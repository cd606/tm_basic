#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_MACHINE_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_MACHINE_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <optional>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    //ChainMachine is somewhat like ChainWriter, but there is no input,
    //instead, it works directly on the current chain state, therefore
    //the ChainMachineLogic class will have a slightly different signature
    //from the InputHandler class in ChainWriter

    template <class App, class Chain, class ChainItemFolder, class ChainMachineLogic>
    class ChainMachine {};

    template <class Env, class Chain, class ChainItemFolder, class ChainMachineLogic>
    class ChainMachine<infra::RealTimeApp<Env>,Chain,ChainItemFolder,ChainMachineLogic> final
        : public infra::RealTimeApp<Env>::template AbstractImporter<typename ChainMachineLogic::OffChainUpdateType> {
    private:
        const bool busyLoop_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentState_;
        ChainMachineLogic logic_;
        bool running_;
        std::thread th_;
        void run(Env *env) {
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
            running_ = true;
            while (running_) {
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
                        std::optional<typename ChainMachineLogic::OffChainUpdateType>
                        , std::optional<std::tuple<typename Chain::StorageIDType, typename Chain::DataType>>
                    > processResult = logic_.work(env, chain_, currentState_);
                    if (std::get<1>(processResult)) {    
                        if (chain_->appendAfter(
                            currentItem_
                            , chain_->formChainItem(
                                std::get<0>(*std::get<1>(processResult))
                                , std::move(std::get<1>(*std::get<1>(processResult))))
                        )) {
                            if (std::get<0>(processResult)) {
                                this->publish(env, std::move(*(std::get<0>(processResult))));
                            }
                            break;
                        }
                    } else {
                        if (std::get<0>(processResult)) {
                            this->publish(env, std::move(*(std::get<0>(processResult))));
                        }
                        break;
                    }
                }
                if (!busyLoop_) {
                    std::this_thread::yield();
                }
            }
        }
    public:
        ChainMachine(Chain *chain, ChainMachineLogic &&logic = ChainMachineLogic(), bool busyLoop=false) :
            busyLoop_(busyLoop)
            , chain_(chain)
            , currentItem_()
            , folder_()
            , currentState_()
            , logic_(std::move(logic))
            , running_(false)
            , th_()
        {}
        ~ChainMachine() {
            running_ = false;
            try {
                th_.join();
            } catch (std::exception const &) {
            }
        }
        ChainMachine(ChainMachine const &) = delete;
        ChainMachine &operator=(ChainMachine const &) = delete;
        ChainMachine(ChainMachine &&) = default;
        ChainMachine &operator=(ChainMachine &&) = delete;
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
            logic_.initialize(env, chain_);
            th_ = std::thread(&ChainMachine::run, this, env);
        }
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<typename ChainItemFolder::ResultType>> importer(Chain *chain, ChainMachineLogic &&logic = ChainMachineLogic(), bool busyLoop=false) {
            return infra::RealTimeApp<Env>::template importer<typename ChainItemFolder::ResultType>>(new ChainMachine(chain, std::move(logic), busyLoop));
        }
    };

    template <class Env, class Chain, class ChainItemFolder, class ChainMachineLogic>
    class ChainMachine<infra::SinglePassIterationApp<Env>,Chain,ChainItemFolder,ChainMachineLogic> final
        : public infra::SinglePassIterationApp<Env>::template AbstractImporter<typename ChainMachineLogic::OffChainUpdateType> {
    private:
        Env *env_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentState_;
        ChainMachineLogic logic_;
    public:
        ChainMachine(Chain *chain, ChainMachineLogic &&logic = ChainMachineLogic(), bool busyLoop=false) :
            env_(nullptr)
            , chain_(chain)
            , currentItem_()
            , folder_()
            , currentState_()
            , logic_(std::move(logic))
        {}
        ~ChainMachine() {}
        ChainMachine(ChainMachine const &) = delete;
        ChainMachine &operator=(ChainMachine const &) = delete;
        ChainMachine(ChainMachine &&) = default;
        ChainMachine &operator=(ChainMachine &&) = delete;
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
            logic_.initialize(env, chain_);
        }
        virtual typename infra::SinglePassIterationApp<Env>::template Data<typename ChainItemFolder::ResultType> generate(typename ChainItemFolder::ResultType const *notUsed=nullptr) override final {
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
            static auto timeExtractionChecker = boost::hana::is_valid(
                [](auto *e, auto *f, auto const *v) -> decltype((void) (e->resolveTime(f->extractTime(*v)))) {}
            );
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
                    std::optional<typename ChainMachineLogic::OffChainUpdateType>
                    , std::optional<std::tuple<typename Chain::StorageIDType, typename Chain::DataType>>
                > processResult = logic_.work(env_, chain_, currentState_);
                if (std::get<1>(processResult)) {    
                    if (chain_->appendAfter(
                        currentItem_
                        , chain_->formChainItem(
                            std::get<0>(*std::get<1>(processResult))
                            , std::move(std::get<1>(*std::get<1>(processResult))))
                    )) {
                        if (std::get<0>(processResult)) {
                            if constexpr (timeExtractionChecker(
                                (Env *) nullptr
                                , (ChainItemFolder *) nullptr
                                , (typename ChainItemFolder::ResultType *) nullptr
                            )) {
                                return typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                                    env_
                                    , {
                                        env_->resolveTime(folder_.extractTime(currentState_))
                                        , std::move(*(std::get<0>(processResult)))
                                        , false
                                    }
                                };
                            } else {
                                return typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                                    env_
                                    , {
                                        env_->resolveTime()
                                        , std::move(*(std::get<0>(processResult)))
                                        , false
                                    }
                                };
                            }
                        } else {
                            return std::nullopt;
                        }
                    }
                } else {
                    if (std::get<0>(processResult)) {
                        if constexpr (timeExtractionChecker(
                            (Env *) nullptr
                            , (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                        )) {
                            return typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                                env_
                                , {
                                    env_->resolveTime(folder_.extractTime(currentState_))
                                    , std::move(*(std::get<0>(processResult)))
                                    , false
                                }
                            };
                        } else {
                            return typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                                env_
                                , {
                                    env_->resolveTime()
                                    , std::move(*(std::get<0>(processResult)))
                                    , false
                                }
                            };
                        }
                    } else {
                        return std::nullopt;
                    }
                }
            }
        }
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Importer<typename ChainItemFolder::ResultType>> importer(Chain *chain, ChainMachineLogic &&logic = ChainMachineLogic()) {
            return infra::SinglePassIterationApp<Env>::template importer<typename ChainItemFolder::ResultType>>(new ChainMachine(chain, std::move(logic)));
        }
    };

} } } } }

#endif