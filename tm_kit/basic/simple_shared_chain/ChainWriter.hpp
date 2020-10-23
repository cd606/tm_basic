#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/basic/simple_shared_chain/FolderUsingPartialHistoryInformation.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainPollingPolicy.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class App, class Chain, class ChainItemFolder, class InputHandler, class IdleLogic=void>
    class ChainWriter {};

    template <class IdleLogic>
    struct OffChainUpdateTypeExtractor {
        using T = typename IdleLogic::OffChainUpdateType;
    };
    template <>
    struct OffChainUpdateTypeExtractor<void> {
        using T = basic::VoidStruct;
    };

    template <class Env, class Chain, class ChainItemFolder, class InputHandler, class IdleLogic>
    class ChainWriter<typename infra::RealTimeApp<Env>, Chain, ChainItemFolder, InputHandler, IdleLogic>
        : 
        public virtual infra::RealTimeApp<Env>::IExternalComponent
        , public virtual 
            std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , typename infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>
                , typename infra::RealTimeApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
            >
    {
    private:
        using OOF = typename infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>;
        using IM = typename infra::RealTimeApp<Env>::template AbstractImporter<typename OffChainUpdateTypeExtractor<IdleLogic>::T>;
        void idleWork() {
            static_assert(!std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
            >);

            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
            //This is to give the facility handler a chance to do something
            //with current state, for example to save it. This is not needed
            //when we have IdleLogic.
            //Also, this is specific to real-time mode. Single-pass iteration
            //mode does not really need state-saving in the middle.
            static auto handlerIdleCallbackChecker = boost::hana::is_valid(
                [](auto *h, auto *c, auto const *v) -> decltype((void) h->idleCallback(c, *v)) {}
            );
            if constexpr (std::is_same_v<IdleLogic, void>) {
                //only read one, since this is just a helper
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                if (nextItem) {
                    currentItem_ = std::move(*nextItem);
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (typename Chain::ItemType const *) nullptr
                    )) {
                        folder_.foldInPlace(currentState_, currentItem_);
                    } else {
                        currentState_ = folder_.fold(currentState_, currentItem_);
                    }
                    if constexpr (handlerIdleCallbackChecker(
                        (InputHandler *) nullptr
                        , (Chain *) nullptr
                        , (typename ChainItemFolder::ResultType const *) nullptr
                    )) {
                        inputHandler_.idleCallback(chain_, currentState_);
                    }
                }
            } else {
                //we must read until the end
                while (true) {
                    while (true) {
                        std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                        if (!nextItem) {
                            break;
                        }
                        currentItem_ = std::move(*nextItem);
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (typename Chain::ItemType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, currentItem_);
                        } else {
                            currentState_ = folder_.fold(currentState_, currentItem_);
                        }
                    }
                    std::tuple<
                        std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                        , std::optional<std::tuple<typename Chain::StorageIDType, typename Chain::DataType>>
                    > processResult = idleLogic_.work(env_, chain_, currentState_);
                    if (std::get<1>(processResult)) {    
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                std::get<0>(*std::get<1>(processResult))
                                , std::move(std::get<1>(*std::get<1>(processResult))))
                        )) {
                            if (std::get<0>(processResult)) {
                                this->IM::publish(env_, std::move(*(std::get<0>(processResult))));
                            }
                            break;
                        }
                    } else {
                        if (std::get<0>(processResult)) {
                            this->IM::publish(env_, std::move(*(std::get<0>(processResult))));
                        }
                        break;
                    }
                }
            }
        }
        void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) {
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
            
            auto id = data.timedData.value.id();
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (typename Chain::ItemType const *) nullptr
                    )) {
                        folder_.foldInPlace(currentState_, currentItem_);
                    } else {
                        currentState_ = folder_.fold(currentState_, currentItem_);
                    }
                }
                std::tuple<
                    typename InputHandler::ResponseType
                    , std::optional<std::tuple<typename Chain::StorageIDType, typename Chain::DataType>>
                > processResult = inputHandler_.handleInput(data.environment, chain_, std::move(data.timedData), currentState_);
                if (std::get<1>(processResult)) {   
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItem(
                            std::get<0>(*std::get<1>(processResult))
                            , std::move(std::get<1>(*std::get<1>(processResult))))
                    )) { 
                        this->OOF::publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else {
                    this->OOF::publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        id, std::move(std::get<0>(processResult))
                    }, true);
                    break;
                }
            }
        }
        friend class InnerHandler1;
        class InnerHandler1 : public infra::RealTimeAppComponents<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> {
        private:
            ChainWriter *parent_;
        public:
            InnerHandler1(ChainWriter *parent) : infra::RealTimeAppComponents<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>(), parent_(parent) {
            }
            virtual ~InnerHandler1() {}
            virtual void idleWork() override final {
                parent_->idleWork();
            }
            virtual void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
                parent_->actuallyHandle(std::move(data));
            }
        };
        friend class InnerHandler2;
        class InnerHandler2 : public infra::RealTimeAppComponents<Env>::template BusyLoopThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> {
        private:
            ChainWriter *parent_;
        public:
            InnerHandler2(ChainWriter *parent) : infra::RealTimeAppComponents<Env>::template BusyLoopThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>(parent->noYield_), parent_(parent) {
            }
            virtual ~InnerHandler2() {}
            virtual void idleWork() override final {
                parent_->idleWork();
            }
            virtual void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
                parent_->actuallyHandle(std::move(data));
            }
        };
        bool useBusyLoop_;
        bool noYield_;
        InnerHandler1 *innerHandler1_;
        InnerHandler2 *innerHandler2_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;  
        Env *env_;
        std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , basic::VoidStruct
            , IdleLogic
        > idleLogic_; 

        using ParentInterface = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , typename infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>
            , typename infra::RealTimeApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
        >;
    public:
        using IdleLogicPlaceHolder = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , basic::VoidStruct
            , IdleLogic
        >;
        ChainWriter(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder()) : 
#ifndef _MSC_VER
            infra::RealTimeApp<Env>::IExternalComponent(), 
            ParentInterface(), 
#endif
            useBusyLoop_(pollingPolicy.busyLoop)
            , noYield_(pollingPolicy.noYield)
            , innerHandler1_(nullptr)
            , innerHandler2_(nullptr)
            , chain_(chain)
            , currentItem_()
            , folder_()
            , inputHandler_(std::move(inputHandler))
            , currentState_() 
            , env_(nullptr)
            , idleLogic_(std::move(idleLogic))
        {}
        virtual ~ChainWriter() {
            if (innerHandler1_) {
                delete innerHandler1_;
            }
            if (innerHandler2_) {
                delete innerHandler2_;
            }
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            env_ = env;
            
            currentState_ = folder_.initialize(env, chain_);
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentState_));
            } else {
                currentItem_ = chain_->head(env);
            }
            inputHandler_.initialize(env, chain_);
            if (useBusyLoop_) {
                innerHandler2_ = new InnerHandler2(this);
            } else {
                innerHandler1_ = new InnerHandler1(this);
            }
        }
        virtual void handle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            if (innerHandler1_) {
                innerHandler1_->handle(std::move(data));
            } else if (innerHandler2_) {
                innerHandler2_->handle(std::move(data));
            }
        }
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacility<
            typename InputHandler::InputType, typename InputHandler::ResponseType
        >> onOrderFacility(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return infra::RealTimeApp<Env>::template fromAbstractOnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >(new ChainWriter(chain, pollingPolicy, std::move(inputHandler), std::move(idleLogic)));
            } else {
                return std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >>();
            }
        }
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacilityWithExternalEffects<
            typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
        >> onOrderFacilityWithExternalEffects(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >>();
            } else {
                return infra::RealTimeApp<Env>::template onOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >(new ChainWriter(chain, pollingPolicy, std::move(inputHandler), std::move(idleLogic)));
            }
        }
    };

    template <class Env, class Chain, class ChainItemFolder, class InputHandler, class IdleLogic>
    class ChainWriter<typename infra::SinglePassIterationApp<Env>, Chain, ChainItemFolder, InputHandler, IdleLogic>
        : 
        public virtual infra::SinglePassIterationApp<Env>::IExternalComponent
        , public virtual std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , typename infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> 
            , typename infra::SinglePassIterationApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
        >
    {
    private:
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;
        Env *env_;
        std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , basic::VoidStruct
            , IdleLogic
        > idleLogic_;
    protected:
        virtual void handle(typename infra::SinglePassIterationApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            static_assert(!std::is_convertible_v<
                ChainItemFolder *, FolderUsingPartialHistoryInformation *
            >);

            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
            auto id = data.timedData.value.id();
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (typename Chain::ItemType const *) nullptr
                    )) {
                        folder_.foldInPlace(currentState_, currentItem_);
                    } else {
                        currentState_ = folder_.fold(currentState_, currentItem_);
                    }
                }
                std::tuple<
                    typename InputHandler::ResponseType
                    , std::optional<std::tuple<typename Chain::StorageIDType, typename Chain::DataType>>
                > processResult = inputHandler_.handleInput(data.environment, chain_, std::move(data.timedData), currentState_);
                if (std::get<1>(processResult)) {    
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItem(
                            std::get<0>(*std::get<1>(processResult))
                            , std::move(std::get<1>(*std::get<1>(processResult))))
                    )) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else {
                    this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        id, std::move(std::get<0>(processResult))
                    }, true);
                    break;
                }
            }
        }
        virtual typename infra::SinglePassIterationApp<Env>::template Data<typename OffChainUpdateTypeExtractor<IdleLogic>::T> generate(
            typename OffChainUpdateTypeExtractor<IdleLogic>::T const *notUsed=nullptr
        ) {
            if constexpr (!std::is_same_v<IdleLogic, void>) {
                static auto foldInPlaceChecker = boost::hana::is_valid(
                    [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
                );
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                if (!nextItem) {
                    return std::nullopt;
                }
                currentItem_ = std::move(*nextItem);
                if constexpr (foldInPlaceChecker(
                    (ChainItemFolder *) nullptr
                    , (typename ChainItemFolder::ResultType *) nullptr
                    , (typename Chain::ItemType const *) nullptr
                )) {
                    folder_.foldInPlace(currentState_, currentItem_);
                } else {
                    currentState_ = folder_.fold(currentState_, currentItem_);
                }
                std::tuple<
                    std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                    , std::optional<std::tuple<typename Chain::StorageIDType, typename Chain::DataType>>
                > processResult = idleLogic_.work(env_, chain_, currentState_);
                if (std::get<1>(processResult)) {    
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItem(
                            std::get<0>(*std::get<1>(processResult))
                            , std::move(std::get<1>(*std::get<1>(processResult))))
                    )) {
                        if (std::get<0>(processResult)) {
                            return typename infra::SinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                                env_
                                , {
                                    env_->resolveTime(folder_.extractTime(currentState_))
                                    , std::move(*(std::get<0>(processResult)))
                                    , false
                                }
                            };
                        } else {
                            return std::nullopt;
                        }
                    } else {
                        return std::nullopt;
                    }
                } else {
                    if (std::get<0>(processResult)) {
                        return typename infra::SinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                            env_
                            , {
                                env_->resolveTime(folder_.extractTime(currentState_))
                                , std::move(*(std::get<0>(processResult)))
                                , false
                            }
                        };
                    } else {
                        return std::nullopt;
                    }
                }
            }
            return std::nullopt;
        }
        using ParentInterface = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , typename infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> 
            , typename infra::SinglePassIterationApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
        >;
    public:
        using IdleLogicPlaceHolder = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , basic::VoidStruct
            , IdleLogic
        >;
        ChainWriter(Chain *chain, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder()) :
#ifndef _MSC_VER
            infra::SinglePassIterationApp<Env>::IExternalComponent(),
            ParentInterface(), 
#endif
            chain_(chain)
            , currentItem_()
            , folder_()
            , inputHandler_(std::move(inputHandler))
            , currentState_() 
            , env_(nullptr)
            , idleLogic_(std::move(idleLogic))
        {}
        virtual ~ChainWriter() {
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            env_ = env;
            currentState_ = folder_.initialize(env, chain_);
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentState_));
            } else {
                currentItem_ = chain_->head(env);
            }
            inputHandler_.initialize(env, chain_);
        }
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacility<
            typename InputHandler::InputType, typename InputHandler::ResponseType
        >> onOrderFacility(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return infra::SinglePassIterationApp<Env>::template fromAbstractOnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >(new ChainWriter(chain, std::move(inputHandler), std::move(idleLogic)));
            } else {
                return std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >>();
            }
        }
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacilityWithExternalEffects<
            typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
        >> onOrderFacilityWithExternalEffects(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >>();
            } else {
                return infra::SinglePassIterationApp<Env>::template onOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >(new ChainWriter(chain, std::move(inputHandler), std::move(idleLogic)));
            }
        }
    };
} } } } }

#endif