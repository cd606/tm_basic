#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/simple_shared_chain/FolderUsingPartialHistoryInformation.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainPollingPolicy.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/NotConstructibleStruct.hpp>
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

    template <class Chain>
    struct ChainLock {
    private:
        Chain *chain_;
    public:
        ChainLock(Chain *c) : chain_(c) {
            if constexpr (chainHasDataLock()) {
                chain_->acquireLock();
            }
        }
        ~ChainLock() {
            if constexpr (chainHasDataLock()) {
                chain_->releaseLock();
            }
        }
        static constexpr bool chainHasDataLock() {
            auto acquireLockChecker = boost::hana::is_valid(
                [](auto *c) -> decltype((void) c->acquireLock()) {}
            );
            auto releaseLockChecker = boost::hana::is_valid(
                [](auto *c) -> decltype((void) c->releaseLock()) {}
            );
            auto lockIsTrivialChecker = boost::hana::is_valid(
                [](auto *c) -> decltype((void) c->DataLockIsTrivial) {}
            );
            if constexpr (acquireLockChecker((Chain *) nullptr)) {
                if constexpr (releaseLockChecker((Chain *) nullptr)) {
                    if constexpr (lockIsTrivialChecker((Chain *) nullptr)) {
                        return !Chain::DataLockIsTrivial;
                    } else {
                        return true;
                    }
                }
            }
            return false;
        }
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

            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            //This is to give the facility handler a chance to do something
            //with current state, for example to save it. This is not needed
            //when we have IdleLogic.
            //Also, this is specific to real-time mode. Single-pass iteration
            //mode does not really need state-saving in the middle.
            static const auto handlerIdleCallbackChecker = boost::hana::is_valid(
                [](auto *h, auto *c, auto const *v) -> decltype((void) h->idleCallback(c, *v)) {}
            );
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(env_, ":idlework");
            if constexpr (std::is_same_v<IdleLogic, void>) {
                //only read one, since this is just a helper
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                if (nextItem) {
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                        if constexpr (handlerIdleCallbackChecker(
                            (InputHandler *) nullptr
                            , (Chain *) nullptr
                            , (typename ChainItemFolder::ResultType const *) nullptr
                        )) {
                            inputHandler_.idleCallback(chain_, currentState_);
                        }
                    }
                }
            } else {
                ChainLock<Chain> _(chain_);
                //we must read until the end
                while (true) {
                    while (true) {
                        std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                        if (!nextItem) {
                            break;
                        }
                        currentItem_ = std::move(*nextItem);
                        auto const *dataPtr = Chain::extractData(currentItem_);
                        if (dataPtr) {
                            if constexpr (foldInPlaceChecker(
                                (ChainItemFolder *) nullptr
                                , (typename ChainItemFolder::ResultType *) nullptr
                                , (std::string_view *) nullptr
                                , (typename Chain::DataType const *) nullptr
                            )) {
                                folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                            } else {
                                currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                            }
                        }
                    }
                    auto processResult = idleLogic_.work(env_, chain_, currentState_);
                    if constexpr (
                        std::is_same_v<
                            decltype(processResult)
                            , std::tuple<
                                std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                                , std::vector<std::tuple<std::string, typename Chain::DataType>>
                            >
                        >
                    ) {
                        for (auto &x : std::get<1>(processResult)) {
                            if (std::get<0>(x) == "") {
                                std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                            }
                        }
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItems(std::move(std::get<1>(processResult)))
                        )) {
                            if (std::get<0>(processResult)) {
                                this->IM::publish(env_, std::move(*(std::get<0>(processResult))));
                            }
                            break;
                        }
                    } else if constexpr (
                        std::is_same_v<
                            decltype(processResult)
                            , std::tuple<
                                std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                                , std::optional<std::tuple<std::string, typename Chain::DataType>>
                            >
                        >
                    ) {
                        if (std::get<1>(processResult)) {
                            std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                            if (newID == "") {
                                newID = Chain::template newStorageIDAsString<Env>();
                            }   
                            if (chain_->appendAfter(
                                currentItem_
                                , chain_->formChainItem(
                                    newID
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
                    } else {
                        throw std::runtime_error("Wrong processResult type");
                    }
                }
            }
        }
        void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(data.environment, ":handle");
            auto id = data.timedData.value.id();
            ChainLock<Chain> _(chain_);
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                    }
                }
                auto processResult = inputHandler_.handleInput(data.environment, chain_, data.timedData, currentState_);
                if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::vector<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    for (auto &x : std::get<1>(processResult)) {
                        if (std::get<0>(x) == "") {
                            std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                        }
                    }
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItems(std::move(std::get<1>(processResult)))
                    )) { 
                        this->OOF::publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::optional<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    if (std::get<1>(processResult)) {   
                        std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                        if (newID == "") {
                            newID = Chain::template newStorageIDAsString<Env>();
                        }
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                newID
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
                } else {
                    throw std::runtime_error("Wrong processResult type");
                }
            }
        }
        void waitForStart() {
            while (true) {
                std::unique_lock<std::mutex> lock(startWaiterStruct_->mutex_);
                startWaiterStruct_->cond_.wait_for(lock, std::chrono::milliseconds(1));
                if (startWaiterStruct_->started_) {
                    lock.unlock();
                    break;
                } else {
                    lock.unlock();
                }
            }
        }
        friend class InnerHandler1;
        class InnerHandler1 : public infra::RealTimeAppComponents<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>, InnerHandler1> {
        private:
            ChainWriter *parent_;
        public:
            InnerHandler1(ChainWriter *parent) : infra::RealTimeAppComponents<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>, InnerHandler1>(), parent_(parent) {
                this->startThread();
            }
            virtual ~InnerHandler1() {}
            void idleWork() {
                parent_->idleWork();
            }
            void waitForStart() {
                parent_->waitForStart();
            }
            void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) {
                parent_->actuallyHandle(std::move(data));
            }
        };
        friend class InnerHandler2;
        class InnerHandler2 : public infra::RealTimeAppComponents<Env>::template BusyLoopThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>, InnerHandler2> {
        private:
            ChainWriter *parent_;
        public:
            InnerHandler2(ChainWriter *parent) : infra::RealTimeAppComponents<Env>::template BusyLoopThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>, InnerHandler2>(parent->noYield_), parent_(parent) {
                this->startThread();
            }
            virtual ~InnerHandler2() {}
            void idleWork() {
                parent_->idleWork();
            }
            void waitForStart() {
                parent_->waitForStart();
            }
            void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) {
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

        struct StartWaiterStruct {
            std::mutex mutex_;
            std::condition_variable cond_;
            bool started_ = false;
        };
        std::unique_ptr<StartWaiterStruct> startWaiterStruct_;

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
        ChainWriter(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder()) : 
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
            , folder_(std::move(folder))
            , inputHandler_(std::move(inputHandler))
            , currentState_() 
            , env_(nullptr)
            , idleLogic_(std::move(idleLogic))
            , startWaiterStruct_(std::make_unique<StartWaiterStruct>())
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
            static const auto checker = boost::hana::is_valid(
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

            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            //for real time writer, if the chain has data lock
            //it might be better to read up to the end in the startup
            //to avoid doing this while holding the data lock
            if constexpr (ChainLock<Chain>::chainHasDataLock()) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                    }
                }
            }
            {
                std::lock_guard<std::mutex> _(startWaiterStruct_->mutex_);
                startWaiterStruct_->started_ = true;
                startWaiterStruct_->cond_.notify_one();
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
        >> onOrderFacility(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return infra::RealTimeApp<Env>::template fromAbstractOnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >(new ChainWriter(chain, pollingPolicy, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            } else {
                return std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >>();
            }
        }
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacilityWithExternalEffects<
            typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
        >> onOrderFacilityWithExternalEffects(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return std::shared_ptr<typename infra::RealTimeApp<Env>::template OnOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >>();
            } else {
                return infra::RealTimeApp<Env>::template onOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >(new ChainWriter(chain, pollingPolicy, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
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
        typename Env::TimePointType lastIdleCheckTime_;
    protected:
        virtual void handle(typename infra::SinglePassIterationApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            static_assert(!std::is_convertible_v<
                ChainItemFolder *, FolderUsingPartialHistoryInformation *
            >);

            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(data.environment, ":handle");
            data.environment->resolveTime(data.timedData.timePoint);
            auto id = data.timedData.value.id();
            ChainLock<Chain> _(chain_);
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                    }
                }
                auto processResult = inputHandler_.handleInput(data.environment, chain_, data.timedData, currentState_);
                if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::vector<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    for (auto &x : std::get<1>(processResult)) {
                        if (std::get<0>(x) == "") {
                            std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                        }
                    }
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItems(std::move(std::get<1>(processResult)))
                    )) {
                        this->publish(data.environment, typename infra::SinglePassIterationApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::optional<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    if (std::get<1>(processResult)) {   
                        std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                        if (newID == "") {
                            newID = Chain::template newStorageIDAsString<Env>();
                        } 
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                newID
                                , std::move(std::get<1>(*std::get<1>(processResult))))
                        )) {
                            this->publish(data.environment, typename infra::SinglePassIterationApp<Env>::template Key<typename InputHandler::ResponseType> {
                                id, std::move(std::get<0>(processResult))
                            }, true);
                            break;
                        }
                    } else {
                        this->publish(data.environment, typename infra::SinglePassIterationApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else {
                    throw std::runtime_error("Wrong processResult type");
                }
            }
        }
        virtual typename infra::SinglePassIterationApp<Env>::template Data<typename OffChainUpdateTypeExtractor<IdleLogic>::T> generate(
            typename OffChainUpdateTypeExtractor<IdleLogic>::T const *notUsed=nullptr
        ) {
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(env_, ":idlework");
            if constexpr (!std::is_same_v<IdleLogic, void>) {
                static const auto foldInPlaceChecker = boost::hana::is_valid(
                    [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
                );
                ChainLock<Chain> _(chain_);
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                bool hasUpdate = false;
                if (nextItem) {
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                        hasUpdate = true;
                    }
                }
                env_->resolveTime(folder_.extractTime(currentState_));
                auto now = env_->now();
                if (!hasUpdate) {
                    if (now <= lastIdleCheckTime_) {
                        return std::nullopt;
                    }
                }
                lastIdleCheckTime_ = now;
                auto processResult = idleLogic_.work(env_, chain_, currentState_);
                if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                            , std::vector<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    for (auto &x : std::get<1>(processResult)) {
                        if (std::get<0>(x) == "") {
                            std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                        }
                    }
                    if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItems(std::move(std::get<1>(processResult)))
                        )) {
                        if (std::get<0>(processResult)) {
                            return typename infra::SinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                                env_
                                , {
                                    now
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
                } else if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                            , std::optional<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    if (std::get<1>(processResult)) {   
                        std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                        if (newID == "") {
                            newID = Chain::template newStorageIDAsString<Env>();
                        } 
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                newID
                                , std::move(std::get<1>(*std::get<1>(processResult))))
                        )) {
                            if (std::get<0>(processResult)) {
                                return typename infra::SinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                                    env_
                                    , {
                                        now
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
                                    now
                                    , std::move(*(std::get<0>(processResult)))
                                    , false
                                }
                            };
                        } else {
                            return std::nullopt;
                        }
                    }
                } else {
                    throw std::runtime_error("Wrong processResult type");
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
        ChainWriter(Chain *chain, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder()) :
#ifndef _MSC_VER
            infra::SinglePassIterationApp<Env>::IExternalComponent(),
            ParentInterface(), 
#endif
            chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , inputHandler_(std::move(inputHandler))
            , currentState_() 
            , env_(nullptr)
            , idleLogic_(std::move(idleLogic))
            , lastIdleCheckTime_()
        {}
        virtual ~ChainWriter() {
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            static const auto checker = boost::hana::is_valid(
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
        >> onOrderFacility(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return infra::SinglePassIterationApp<Env>::template fromAbstractOnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >(new ChainWriter(chain, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            } else {
                return std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >>();
            }
        }
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacilityWithExternalEffects<
            typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
        >> onOrderFacilityWithExternalEffects(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template OnOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >>();
            } else {
                return infra::SinglePassIterationApp<Env>::template onOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >(new ChainWriter(chain, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            }
        }
    };

    //This is just for compilation check, the logic will never be run

    template <class Env, class Chain, class ChainItemFolder, class InputHandler, class IdleLogic>
    class ChainWriter<typename infra::BasicWithTimeApp<Env>, Chain, ChainItemFolder, InputHandler, IdleLogic>
        : 
        public virtual infra::BasicWithTimeApp<Env>::IExternalComponent
        , public virtual 
            std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , typename infra::BasicWithTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>
                , typename infra::BasicWithTimeApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
            >
    {
    private:
        using OOF = typename infra::BasicWithTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>;
        using IM = typename infra::BasicWithTimeApp<Env>::template AbstractImporter<typename OffChainUpdateTypeExtractor<IdleLogic>::T>;
        void idleWork() {
            static_assert(!std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
            >);

            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            //This is to give the facility handler a chance to do something
            //with current state, for example to save it. This is not needed
            //when we have IdleLogic.
            //Also, this is specific to real-time mode. Single-pass iteration
            //mode does not really need state-saving in the middle.
            static const auto handlerIdleCallbackChecker = boost::hana::is_valid(
                [](auto *h, auto *c, auto const *v) -> decltype((void) h->idleCallback(c, *v)) {}
            );
            if constexpr (std::is_same_v<IdleLogic, void>) {
                //only read one, since this is just a helper
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                if (nextItem) {
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                        if constexpr (handlerIdleCallbackChecker(
                            (InputHandler *) nullptr
                            , (Chain *) nullptr
                            , (typename ChainItemFolder::ResultType const *) nullptr
                        )) {
                            inputHandler_.idleCallback(chain_, currentState_);
                        }
                    }
                }
            } else {
                //we must read until the end
                ChainLock<Chain> _(chain_);
                while (true) {
                    while (true) {
                        std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                        if (!nextItem) {
                            break;
                        }
                        currentItem_ = std::move(*nextItem);
                        auto const *dataPtr = Chain::extractData(currentItem_);
                        if (dataPtr) {
                            if constexpr (foldInPlaceChecker(
                                (ChainItemFolder *) nullptr
                                , (typename ChainItemFolder::ResultType *) nullptr
                                , (std::string_view *) nullptr
                                , (typename Chain::DataType const *) nullptr
                            )) {
                                folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                            } else {
                                currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                            }
                        }
                    }
                    auto processResult = idleLogic_.work(env_, chain_, currentState_);
                    if constexpr (
                        std::is_same_v<
                            decltype(processResult)
                            , std::tuple<
                                std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                                , std::vector<std::tuple<std::string, typename Chain::DataType>>
                            >
                        >
                    ) {
                        for (auto &x : std::get<1>(processResult)) {
                            if (std::get<0>(x) == "") {
                                std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                            }
                        }
                        if (chain_->appendAfter(
                                currentItem_
                                , chain_->formChainItems(std::move(std::get<1>(processResult)))
                        )) {
                            if (std::get<0>(processResult)) {
                                this->IM::publish(env_, std::move(*(std::get<0>(processResult))));
                            }
                            break;
                        }
                    } else if constexpr (
                        std::is_same_v<
                            decltype(processResult)
                            , std::tuple<
                                std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                                , std::optional<std::tuple<std::string, typename Chain::DataType>>
                            >
                        >
                    ) {
                        if (std::get<1>(processResult)) {
                            std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                            if (newID == "") {
                                newID = Chain::template newStorageIDAsString<Env>();
                            }   
                            if (chain_->appendAfter(
                                currentItem_
                                , chain_->formChainItem(
                                    newID
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
                    } else {
                        throw std::runtime_error("Wrong processResult type");
                    }
                }
            }
        }
        void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(data.environment, ":handle");
            auto id = data.timedData.value.id();
            ChainLock<Chain> _(chain_);
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                    }
                }
                auto processResult = inputHandler_.handleInput(data.environment, chain_, data.timedData, currentState_);
                if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::vector<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    for (auto &x : std::get<1>(processResult)) {
                        if (std::get<0>(x) == "") {
                            std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                        }
                    }
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItems(std::move(std::get<1>(processResult)))
                    )) { 
                        this->OOF::publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::optional<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    if (std::get<1>(processResult)) {   
                        std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                        if (newID == "") {
                            newID = Chain::template newStorageIDAsString<Env>();
                        }
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                newID
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
                } else {
                    throw std::runtime_error("Wrong processResult type");
                }
            }
        }
        
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
            , typename infra::BasicWithTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>
            , typename infra::BasicWithTimeApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
        >;
    public:
        using IdleLogicPlaceHolder = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , basic::VoidStruct
            , IdleLogic
        >;
        ChainWriter(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder()) : 
#ifndef _MSC_VER
            infra::BasicWithTimeApp<Env>::IExternalComponent(), 
            ParentInterface(), 
#endif
            chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
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
            static const auto checker = boost::hana::is_valid(
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
        virtual void handle(typename infra::BasicWithTimeApp<Env>::template InnerData<typename infra::BasicWithTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
        }
        static std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template OnOrderFacility<
            typename InputHandler::InputType, typename InputHandler::ResponseType
        >> onOrderFacility(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return infra::BasicWithTimeApp<Env>::template fromAbstractOnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >(new ChainWriter(chain, pollingPolicy, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            } else {
                return std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template OnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >>();
            }
        }
        static std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template OnOrderFacilityWithExternalEffects<
            typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
        >> onOrderFacilityWithExternalEffects(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template OnOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >>();
            } else {
                return infra::BasicWithTimeApp<Env>::template onOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >(new ChainWriter(chain, pollingPolicy, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            }
        }
    };

    template <class Env, class Chain, class ChainItemFolder, class InputHandler, class IdleLogic>
    class ChainWriter<typename infra::TopDownSinglePassIterationApp<Env>, Chain, ChainItemFolder, InputHandler, IdleLogic>
        : 
        public virtual infra::TopDownSinglePassIterationApp<Env>::IExternalComponent
        , public virtual std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , typename infra::TopDownSinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> 
            , typename infra::TopDownSinglePassIterationApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
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
        typename Env::TimePointType lastIdleCheckTime_;
    protected:
        using FacilityParentInterface = 
            typename infra::TopDownSinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>;
        virtual void handle(typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            static_assert(!std::is_convertible_v<
                ChainItemFolder *, FolderUsingPartialHistoryInformation *
            >);

            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(data.environment, ":handle");
            data.environment->resolveTime(data.timedData.timePoint);
            auto id = data.timedData.value.id();
            ChainLock<Chain> _(chain_);
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                    }
                }
                auto processResult = inputHandler_.handleInput(data.environment, chain_, data.timedData, currentState_);
                if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::vector<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    for (auto &x : std::get<1>(processResult)) {
                        if (std::get<0>(x) == "") {
                            std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                        }
                    }
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItems(std::move(std::get<1>(processResult)))
                    )) {
                        this->FacilityParentInterface::publish(data.environment, typename infra::TopDownSinglePassIterationApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            typename InputHandler::ResponseType
                            , std::optional<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    if (std::get<1>(processResult)) {   
                        std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                        if (newID == "") {
                            newID = Chain::template newStorageIDAsString<Env>();
                        } 
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                newID
                                , std::move(std::get<1>(*std::get<1>(processResult))))
                        )) {
                            this->FacilityParentInterface::publish(data.environment, typename infra::TopDownSinglePassIterationApp<Env>::template Key<typename InputHandler::ResponseType> {
                                id, std::move(std::get<0>(processResult))
                            }, true);
                            break;
                        }
                    } else {
                        this->FacilityParentInterface::publish(data.environment, typename infra::TopDownSinglePassIterationApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else {
                    throw std::runtime_error("Wrong processResult type");
                }
            }
        }
        virtual std::tuple<bool,typename infra::TopDownSinglePassIterationApp<Env>::template Data<typename OffChainUpdateTypeExtractor<IdleLogic>::T>> generate(
            typename OffChainUpdateTypeExtractor<IdleLogic>::T const *notUsed=nullptr
        ) {
            TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(env_, ":idlework");
            if constexpr (!std::is_same_v<IdleLogic, void>) {
                static const auto foldInPlaceChecker = boost::hana::is_valid(
                    [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
                );
                ChainLock<Chain> _(chain_);
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                bool hasUpdate = false;
                if (nextItem) {
                    currentItem_ = std::move(*nextItem);
                    auto const *dataPtr = Chain::extractData(currentItem_);
                    if (dataPtr) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentState_ = folder_.fold(currentState_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                        hasUpdate = true;
                    }
                }
                env_->resolveTime(folder_.extractTime(currentState_));
                auto now = env_->now();
                if (!hasUpdate) {
                    if (now <= lastIdleCheckTime_) {
                        return {true,std::nullopt};
                    }
                }
                lastIdleCheckTime_ = now;
                auto processResult = idleLogic_.work(env_, chain_, currentState_);
                if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                            , std::vector<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    for (auto &x : std::get<1>(processResult)) {
                        if (std::get<0>(x) == "") {
                            std::get<0>(x) = Chain::template newStorageIDAsString<Env>();
                        }
                    }
                    if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItems(std::move(std::get<1>(processResult)))
                        )) {
                        if (std::get<0>(processResult)) {
                            return {true,{typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                                env_
                                , {
                                    now
                                    , std::move(*(std::get<0>(processResult)))
                                    , false
                                }
                            }}};
                        } else {
                            return {true,std::nullopt};
                        }
                    } else {
                        return {true,std::nullopt};
                    }
                } else if constexpr (
                    std::is_same_v<
                        decltype(processResult)
                        , std::tuple<
                            std::optional<typename OffChainUpdateTypeExtractor<IdleLogic>::T>
                            , std::optional<std::tuple<std::string, typename Chain::DataType>>
                        >
                    >
                ) {
                    if (std::get<1>(processResult)) {   
                        std::string newID = std::move(std::get<0>(*std::get<1>(processResult)));
                        if (newID == "") {
                            newID = Chain::template newStorageIDAsString<Env>();
                        } 
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                newID
                                , std::move(std::get<1>(*std::get<1>(processResult))))
                        )) {
                            if (std::get<0>(processResult)) {
                                return {true,{typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                                    env_
                                    , {
                                        now
                                        , std::move(*(std::get<0>(processResult)))
                                        , false
                                    }
                                }}};
                            } else {
                                return {true,std::nullopt};
                            }
                        } else {
                            return {true,std::nullopt};
                        }
                    } else {
                        if (std::get<0>(processResult)) {
                            return {true,{typename infra::SinglePassIterationApp<Env>::template InnerData<typename OffChainUpdateTypeExtractor<IdleLogic>::T> {
                                env_
                                , {
                                    now
                                    , std::move(*(std::get<0>(processResult)))
                                    , false
                                }
                            }}};
                        } else {
                            return {true,std::nullopt};
                        }
                    }
                } else {
                    throw std::runtime_error("Wrong processResult type");
                }
            }
            return {true,std::nullopt};
        }
        using ParentInterface = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , typename infra::TopDownSinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> 
            , typename infra::TopDownSinglePassIterationApp<Env>::template AbstractIntegratedOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>
        >;
    public:
        using IdleLogicPlaceHolder = std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , basic::VoidStruct
            , IdleLogic
        >;
        ChainWriter(Chain *chain, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder()) :
#ifndef _MSC_VER
            infra::TopDownSinglePassIterationApp<Env>::IExternalComponent(),
            ParentInterface(), 
#endif
            chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , inputHandler_(std::move(inputHandler))
            , currentState_() 
            , env_(nullptr)
            , idleLogic_(std::move(idleLogic))
            , lastIdleCheckTime_()
        {}
        virtual ~ChainWriter() {
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            static const auto checker = boost::hana::is_valid(
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
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template OnOrderFacility<
            typename InputHandler::InputType, typename InputHandler::ResponseType
        >> onOrderFacility(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return infra::TopDownSinglePassIterationApp<Env>::template fromAbstractOnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >(new ChainWriter(chain, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            } else {
                return std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template OnOrderFacility<
                    typename InputHandler::InputType, typename InputHandler::ResponseType
                >>();
            }
        }
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template OnOrderFacilityWithExternalEffects<
            typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
        >> onOrderFacilityWithExternalEffects(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, ChainItemFolder &&folder = ChainItemFolder {}, InputHandler &&inputHandler=InputHandler(), IdleLogicPlaceHolder &&idleLogic=IdleLogicPlaceHolder())
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template OnOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >>();
            } else {
                return infra::TopDownSinglePassIterationApp<Env>::template onOrderFacilityWithExternalEffects<
                    typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T
                >(new ChainWriter(chain, std::move(folder), std::move(inputHandler), std::move(idleLogic)));
            }
        }
    };

    template <class ChainData>
    class EmptyIdleWorker {
    public:
        using OffChainUpdateType = VoidStruct;
        template <class Env, class ChainState>
        static std::tuple<std::optional<OffChainUpdateType>, std::optional<std::tuple<std::string, ChainData>>>
        work(Env *, void *, ChainState const &) {
            return {std::nullopt, std::nullopt};
        }
    };

    template <class ChainData>
    class EmptyInputHandler {
    public:
        using InputType = VoidStruct;
        using ResponseType = VoidStruct;
        static void initialize(void *, void *) {}
        template <class Env, class ChainState>
        static std::tuple<ResponseType, std::optional<std::tuple<std::string, ChainData>>>
        handleInput(Env *, void *, typename infra::RealTimeApp<Env>::template TimedDataType<typename infra::RealTimeApp<Env>::template Key<VoidStruct>> const &, ChainState const &) {
            return {VoidStruct {}, std::nullopt};
        }
    };

    template <class ChainData>
    class CompletelyEmptyInputHandler {
    public:
        using InputType = NotConstructibleStruct;
        using ResponseType = VoidStruct;
        static void initialize(void *, void *) {}
        template <class Env, class ChainState>
        static std::tuple<std::optional<ResponseType>, std::optional<std::tuple<std::string, ChainData>>>
        handleInput(Env *, void *, typename infra::RealTimeApp<Env>::template TimedDataType<typename infra::RealTimeApp<Env>::template Key<NotConstructibleStruct>> const &, ChainState const &) {
            return {std::nullopt, std::nullopt};
        }
    };

    template <class ChainData>
    class SimplyPlaceOnChainInputHandler {
    public:
        using InputType = ChainData;
        using ResponseType = bool;
        static void initialize(void *, void *) {}
        template <class Env, class ChainState>
        static std::tuple<ResponseType, std::optional<std::tuple<std::string, ChainData>>>
        handleInput(Env *, void *, typename infra::RealTimeApp<Env>::template TimedDataType<typename infra::RealTimeApp<Env>::template Key<ChainData>> const &input, ChainState const &) {
            return {true, {{"", input.value.key()}}};
        }
    };

    template <class ChainData>
    class SimplyPlaceMultipleOnChainInputHandler {
    public:
        using InputType = std::vector<ChainData>;
        using ResponseType = bool;
        static void initialize(void *, void *) {}
        template <class Env, class ChainState>
        static std::tuple<ResponseType, std::vector<std::tuple<std::string, ChainData>>>
        handleInput(Env *, void *, typename infra::RealTimeApp<Env>::template TimedDataType<typename infra::RealTimeApp<Env>::template Key<std::vector<ChainData>>> const &input, ChainState const &) {
            std::tuple<ResponseType, std::vector<std::tuple<std::string, ChainData>>> ret {};
            std::get<0>(ret) = true;
            for (auto const item : input.value.key()) {
                std::get<1>(ret).push_back(
                    std::tuple<std::string, ChainData> {
                        "", item
                    }
                );
            }
            return ret;
        }
    };

    template <class ChainData, class StateData>
    class SimplyReturnStateIdleLogic {
    public:
        using OffChainUpdateType = StateData;
        static std::tuple<
            std::optional<OffChainUpdateType>
            , std::optional<std::tuple<std::string, ChainData>>
        > work(void *env, void *chain, StateData const &state) {
            return {
                {infra::withtime_utils::makeValueCopy(state)}
                , std::nullopt
            };
        }
    };

    template <class App, class ChainItemFolder, class InputHandler, class IdleLogic>
    using ChainWriterOnOrderFacilityWithExternalEffectsFactory = std::function<
        std::shared_ptr<typename App::template OnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>>(
        )
    >;
    template <class App, class ChainItemFolder, class InputHandler>
    using ChainWriterOnOrderFacilityFactory = std::function<
        std::shared_ptr<typename App::template OnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>>(
        )
    >;
} } } } }

#endif