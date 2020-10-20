#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
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
        void idleWork() {
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
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
                if constexpr (!std::is_same_v<IdleLogic, void>) {
                    while (true) {
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
                                    this->publish(env_, std::move(*(std::get<0>(processResult))));
                                }
                                break;
                            }
                        } else {
                            if (std::get<0>(processResult)) {
                                this->publish(env_, std::move(*(std::get<0>(processResult))));
                            }
                            break;
                        }
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
        ChainWriter(Chain *chain, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder(), bool useBusyLoop=false, bool noYield=false) : 
#ifndef _MSC_VER
            infra::RealTimeApp<Env>::IExternalComponent(), 
            ParentInterface(), 
#endif
            useBusyLoop_(useBusyLoop)
            , noYield_(noYield)
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
    };

    template <class Env, class Chain, class ChainItemFolder, class InputHandler, class IdleLogic>
    class ChainWriter<typename infra::SinglePassIterationApp<Env>, Chain, ChainItemFolder, InputHandler, IdleLogic>
        : 
        public infra::SinglePassIterationApp<Env>::IExternalComponent
        , public infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> {
    private:
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;
    protected:
        virtual void handle(typename infra::SinglePassIterationApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
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
    public:
        ChainWriter(Chain *chain, InputHandler &&inputHandler=InputHandler()) :
            infra::SinglePassIterationApp<Env>::IExternalComponent()
            , infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>()
            , chain_(chain)
            , currentItem_()
            , folder_()
            , inputHandler_(std::move(inputHandler))
            , currentState_() 
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
    };
} } } } }

#endif