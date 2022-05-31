#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/simple_shared_chain/FolderUsingPartialHistoryInformation.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainPollingPolicy.hpp>
#include <optional>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    template <class T>
    class ResultTypeExtractor {
    public:
        using TheType = typename T::ResultType;
    };
    template <>
    class ResultTypeExtractor<void> {
    public:
        using TheType = VoidStruct;
    };

    template <class Chain, class T>
    class SaveDataOnChain {
    private:
        Chain *chain_;
    public:
        SaveDataOnChain(Chain *chain) : chain_(chain) {}
        SaveDataOnChain(SaveDataOnChain const &) = default;
        SaveDataOnChain(SaveDataOnChain &&) = default;
        SaveDataOnChain &operator=(SaveDataOnChain const &) = default;
        SaveDataOnChain &operator=(SaveDataOnChain &&) = default;
        ~SaveDataOnChain() = default;
        void operator()(std::string const &key, T const &data) const {
            if constexpr (Chain::SupportsExtraData) {
                chain_->template saveExtraData<T>(key, data);
            }
        }
    };

    template <class App, class Chain, class ChainItemFolder, class TriggerT=VoidStruct, class ResultTransformer=void>
    class ChainReader {
    private:
        using Env = typename App::EnvironmentType;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
        std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> resultTransformer_;
        using OutputType = std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>;

        SaveDataOnChain<Chain, typename ChainItemFolder::ResultType> stateSaver_;
        void realTimeIdleWork() {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
            if (!nextItem) {
                return;
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
                    folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                } else {
                    currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                }
            }
        }
    public:
        ChainReader(Env *env, Chain *chain, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) : 
            chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , currentValue_(folder_.initialize(env, chain)) 
            , resultTransformer_(std::move(resultTransformer))
            , stateSaver_(chain)
        {
            static const auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            currentValue_ = folder_.initialize(env, chain_); 
            if constexpr (!std::is_same_v<ResultTransformer, void>) {
                resultTransformer_.initialize(env);
            }
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentValue_));
            } else {
                currentItem_ = chain_->head(env);
            }
        }
        ~ChainReader() = default;
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = default;
        typename std::optional<OutputType> operator()(TriggerT &&triggerData) {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            static const auto threeParamTransformChecker = boost::hana::is_valid(
                [](auto *f, auto const *s, auto const *v, auto *data) -> decltype((void) (f->transform(*s, *v, std::move(*data)))) {}
            );
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
                        folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    } else {
                        currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    }
                }
            }
            if constexpr (std::is_same_v<ResultTransformer, void>) {
                return {currentValue_};
            } else {
                if constexpr (threeParamTransformChecker(
                    (ResultTransformer *) nullptr
                    , (SaveDataOnChain<Chain, typename ChainItemFolder::ResultType> const *) nullptr
                    , (typename ChainItemFolder::ResultType const *) nullptr
                    , (TriggerT *) nullptr
                )) {
                    auto r = resultTransformer_.transform(stateSaver_, currentValue_, std::move(triggerData));
                    if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                        return {std::move(r)};
                    } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                        return r;
                    } else {
                        return std::nullopt;
                    }
                } else {
                    auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                    if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                        return {std::move(r)};
                    } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                        return r;
                    } else {
                        return std::nullopt;
                    }
                }
            }
        }
        static std::shared_ptr<typename App::template Action<TriggerT, OutputType>> action(Env *env, Chain *chain, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) {
            auto ret = App::template liftMaybe<TriggerT>(ChainReader(env, chain, std::move(folder), std::move(resultTransformer)), infra::LiftParameters<typename Env::TimePointType>().SuggestThreaded(true));
            if constexpr (std::is_same_v<App, infra::RealTimeApp<Env>>) {
                App::setIdleWorkerForAction(ret, [](void *p) {
                    if (p) {
                        ((ChainReader *) p)->realTimeIdleWork();
                    }
                });
            }
            return ret;
        }
    };

    template <class Env, class Chain, class ChainItemFolder, class ResultTransformer>
    class ChainReader<infra::RealTimeApp<Env>,Chain,ChainItemFolder,void,ResultTransformer> final
        : public infra::IRealTimeAppPossiblyThreadedNode
          , public infra::RealTimeApp<Env>::template AbstractImporter<
            std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>
          >
          , public virtual infra::IControllableNode<Env> {
    private:
        using OutputType = std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>;
        
        const bool callbackPerUpdate_;
        const bool busyLoop_;
        const bool noYield_;
        const std::chrono::system_clock::duration pollingWaitDuration_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
        std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> resultTransformer_;
        bool running_;
        std::thread th_;
        SaveDataOnChain<Chain, typename ChainItemFolder::ResultType> stateSaver_;
        void run(Env *env) {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            static const auto filterUpdateChecker = boost::hana::is_valid(
                [](auto *f, auto const *id, auto const *data) -> decltype((void) (f->filterUpdate(*id, *data))) {}
            );
            static const auto waitFuncChecker = boost::hana::is_valid(
                [](auto *c, auto const *duration) -> decltype((void) (c->waitForUpdate(*duration))) {}
            );
            static constexpr bool UsesPartialHistory = 
                std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
                >;
            running_ = true;
            while (running_) {
                TM_INFRA_IMPORTER_TRACER(env);
                std::optional<typename Chain::ItemType> nextItem;
                bool hasData = false;
                do {
                    nextItem = chain_->fetchNext(currentItem_);
                    if (nextItem) {
                        currentItem_ = std::move(*nextItem);
                        auto const *dataPtr = Chain::extractData(currentItem_);
                        if (dataPtr) {
                            hasData = true;
                            if constexpr (filterUpdateChecker(
                                (ChainItemFolder *) nullptr
                                , (std::string_view *) nullptr
                                , (typename Chain::DataType const *) nullptr
                            )) {
                                if (!folder_.filterUpdate(Chain::extractStorageIDStringView(currentItem_), *dataPtr)) {
                                    continue;
                                }
                            }
                            if constexpr (foldInPlaceChecker(
                                (ChainItemFolder *) nullptr
                                , (typename ChainItemFolder::ResultType *) nullptr
                                , (std::string_view *) nullptr
                                , (typename Chain::DataType const *) nullptr
                            )) {
                                folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                            } else {
                                currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                            }
                            if constexpr (UsesPartialHistory) {
                                if constexpr (std::is_same_v<ResultTransformer, void>) {
                                    this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                                } else {
                                    auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                                    if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                                        this->publish(env, std::move(r));
                                    } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                                        if (r) {
                                            this->publish(env, std::move(*r));
                                        }
                                    }
                                }
                            } else {
                                if (callbackPerUpdate_) {
                                    if constexpr (std::is_same_v<ResultTransformer, void>) {
                                        this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                                    } else {
                                        auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                                        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                                            this->publish(env, std::move(r));
                                        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                                            if (r) {
                                                this->publish(env, std::move(*r));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } while (nextItem);
                if constexpr (!UsesPartialHistory) {
                    if (!callbackPerUpdate_) {
                        if (hasData) {
                            if constexpr (std::is_same_v<ResultTransformer, void>) {
                                this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                            } else {
                                auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                                if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                                    this->publish(env, std::move(r));
                                } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                                    if (r) {
                                        this->publish(env, std::move(*r));
                                    }
                                }
                            }
                        }
                    }
                }
                if (busyLoop_) {
                    if (!noYield_) {
                        std::this_thread::yield();
                    }
                } else {
                    if constexpr (waitFuncChecker(
                        (Chain *) nullptr 
                        , (std::chrono::system_clock::duration const *) nullptr
                    )) {
                        chain_->waitForUpdate(pollingWaitDuration_);
                    } else {
                        std::this_thread::sleep_for(pollingWaitDuration_);
                    }
                }
            }
        }
    public:
        ChainReader(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) :
            callbackPerUpdate_(pollingPolicy.callbackPerUpdate)
            , busyLoop_(pollingPolicy.busyLoop)
            , noYield_(pollingPolicy.noYield)
            , pollingWaitDuration_(pollingPolicy.readerPollingWaitDuration)
            , chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , currentValue_()
            , resultTransformer_(std::move(resultTransformer))
            , running_(false)
            , th_()
            , stateSaver_(chain)
        {}
        ~ChainReader() {
            running_ = false;
            try {
                th_.join();
            } catch (std::exception const &) {
            }
        }
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = delete;
        virtual void start(Env *env) override final {
            static const auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            currentValue_ = folder_.initialize(env, chain_); 
            if constexpr (!std::is_same_v<ResultTransformer, void>) {
                resultTransformer_.initialize(env);
            }
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentValue_));
            } else {
                currentItem_ = chain_->head(env);
            }
            th_ = std::thread(&ChainReader::run, this, env);
        }
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<OutputType>> importer(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) {
            return infra::RealTimeApp<Env>::template importer<OutputType>(new ChainReader(chain, pollingPolicy, std::move(folder), std::move(resultTransformer)));
        }
        virtual std::optional<std::thread::native_handle_type> threadHandle() override final {
            return th_.native_handle();
        }
        void control(Env */*env*/, std::string const &command, std::vector<std::string> const &params) override final {
            if (command == "stop") {
                running_ = false;
            }
        }
    };

    //in SinglePassIteration, we have to check every update, therefore we don't
    //have the callbackPerUpdate option
    //also, in SinglePassIteration, the extractTime call must be there to provide
    //the timestamp of update
    template <class Env, class Chain, class ChainItemFolder, class ResultTransformer>
    class ChainReader<infra::SinglePassIterationApp<Env>,Chain,ChainItemFolder,void,ResultTransformer> final
        : public infra::SinglePassIterationApp<Env>::template AbstractImporter<
            std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>
          > {
    private:
        using OutputType = std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>;

        Env *env_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
        std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> resultTransformer_;
        SaveDataOnChain<Chain, typename ChainItemFolder::ResultType> stateSaver_;
    public:
        ChainReader(Chain *chain, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) :
            env_(nullptr)
            , chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , currentValue_()
            , resultTransformer_(std::move(resultTransformer))
            , stateSaver_(chain)
        {}
        ~ChainReader() {}
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = delete;
        virtual void start(Env *env) override final {
            static const auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            env_ = env;
            currentValue_ = folder_.initialize(env, chain_); 
            if constexpr (!std::is_same_v<ResultTransformer, void>) {
                resultTransformer_.initialize(env);
            }
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentValue_));
            } else {
                currentItem_ = chain_->head(env);
            }
        }
        virtual typename infra::SinglePassIterationApp<Env>::template Data<OutputType> generate(OutputType const *notUsed=nullptr) override final {
            static constexpr bool UsesPartialHistory = 
                std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
                >;
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            static const auto filterUpdateChecker = boost::hana::is_valid(
                [](auto *f, auto const *id, auto const *data) -> decltype((void) (f->filterUpdate(*id, *data))) {}
            );
            TM_INFRA_IMPORTER_TRACER(env_);
            std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
            if (nextItem) {
                currentItem_ = std::move(*nextItem);
                auto const *dataPtr = Chain::extractData(currentItem_);
                if (dataPtr) {
                    if constexpr (filterUpdateChecker(
                        (ChainItemFolder *) nullptr
                        , (std::string_view *) nullptr
                        , (typename Chain::DataType const *) nullptr
                    )) {
                        if (!folder_.filterUpdate(Chain::extractStorageIDStringView(currentItem_), *dataPtr)) {
                            return std::nullopt;
                        }
                    }
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (std::string_view *) nullptr
                        , (typename Chain::DataType const *) nullptr
                    )) {
                        folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    } else {
                        currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    }
                    if constexpr (std::is_same_v<ResultTransformer, void>) {
                        return typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                            env_
                            , {
                                env_->resolveTime(folder_.extractTime(currentValue_))
                                , infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_)
                                , false
                            }
                        };
                    } else {
                        auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                            return typename infra::SinglePassIterationApp<Env>::template InnerData<OutputType> {
                                env_
                                , {
                                    env_->resolveTime(folder_.extractTime(currentValue_))
                                    , std::move(r)
                                    , false
                                }
                            };
                        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                            if (r) {
                                return typename infra::SinglePassIterationApp<Env>::template InnerData<OutputType> {
                                    env_
                                    , {
                                        env_->resolveTime(folder_.extractTime(currentValue_))
                                        , std::move(*r)
                                        , false
                                    }
                                };
                            } else {
                                return std::nullopt;
                            }
                        }
                    }
                } else {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
        }
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Importer<OutputType>> importer(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) {
            return infra::SinglePassIterationApp<Env>::template importer<OutputType>(new ChainReader(chain, std::move(folder), std::move(resultTransformer)));
        }
    };

    template <class Env, class Chain, class ChainItemFolder, class ResultTransformer>
    class ChainReader<infra::TopDownSinglePassIterationApp<Env>,Chain,ChainItemFolder,void,ResultTransformer> final
        : public infra::TopDownSinglePassIterationApp<Env>::template AbstractImporter<
            std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>
          > {
    private:
        using OutputType = std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>;

        Env *env_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
        std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> resultTransformer_;
        SaveDataOnChain<Chain, typename ChainItemFolder::ResultType> stateSaver_;
    public:
        ChainReader(Chain *chain, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) :
            env_(nullptr)
            , chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , currentValue_()
            , resultTransformer_(std::move(resultTransformer))
            , stateSaver_(chain)
        {}
        ~ChainReader() {}
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = delete;
        virtual void start(Env *env) override final {
            static const auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            env_ = env;
            currentValue_ = folder_.initialize(env, chain_); 
            if constexpr (!std::is_same_v<ResultTransformer, void>) {
                resultTransformer_.initialize(env);
            }
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentValue_));
            } else {
                currentItem_ = chain_->head(env);
            }
        }
        virtual std::tuple<bool, typename infra::TopDownSinglePassIterationApp<Env>::template Data<OutputType>> generate(OutputType const *notUsed=nullptr) override final {
            static constexpr bool UsesPartialHistory = 
                std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
                >;
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            static const auto filterUpdateChecker = boost::hana::is_valid(
                [](auto *f, auto const *id, auto const *data) -> decltype((void) (f->filterUpdate(*id, *data))) {}
            );
            TM_INFRA_IMPORTER_TRACER(env_);
            std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
            if (nextItem) {
                currentItem_ = std::move(*nextItem);
                auto const *dataPtr = Chain::extractData(currentItem_);
                if (dataPtr) {
                    if constexpr (filterUpdateChecker(
                        (ChainItemFolder *) nullptr
                        , (std::string_view *) nullptr
                        , (typename Chain::DataType const *) nullptr
                    )) {
                        if (!folder_.filterUpdate(Chain::extractStorageIDStringView(currentItem_), *dataPtr)) {
                            return {true, std::nullopt};
                        }
                    }
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (std::string_view *) nullptr
                        , (typename Chain::DataType const *) nullptr
                    )) {
                        folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    } else {
                        currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    }
                    if constexpr (std::is_same_v<ResultTransformer, void>) {
                        return {true, {typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                            env_
                            , {
                                env_->resolveTime(folder_.extractTime(currentValue_))
                                , infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_)
                                , false
                            }
                        }}};
                    } else {
                        auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                            return {true, {typename infra::SinglePassIterationApp<Env>::template InnerData<OutputType> {
                                env_
                                , {
                                    env_->resolveTime(folder_.extractTime(currentValue_))
                                    , std::move(r)
                                    , false
                                }
                            }}};
                        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                            if (r) {
                                return {true, {typename infra::SinglePassIterationApp<Env>::template InnerData<OutputType> {
                                    env_
                                    , {
                                        env_->resolveTime(folder_.extractTime(currentValue_))
                                        , std::move(*r)
                                        , false
                                    }
                                }}};
                            } else {
                                return {true,std::nullopt};
                            }
                        }
                    }
                } else {
                    return {true,std::nullopt};
                }
            } else {
                return {true,std::nullopt};
            }
        }
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template Importer<OutputType>> importer(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) {
            return infra::TopDownSinglePassIterationApp<Env>::template importer<OutputType>(new ChainReader(chain, std::move(folder), std::move(resultTransformer)));
        }
    };

    //This whole thing is just for compilation check, the logic will never be run
    template <class Env, class Chain, class ChainItemFolder, class ResultTransformer>
    class ChainReader<infra::BasicWithTimeApp<Env>,Chain,ChainItemFolder,void,ResultTransformer> final
        : public infra::BasicWithTimeApp<Env>::template AbstractImporter<
            std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>
          > {
    private:
        using OutputType = std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>;

        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
        std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> resultTransformer_;
        SaveDataOnChain<Chain, typename ChainItemFolder::ResultType> stateSaver_;
        void run(Env *env) {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            static const auto filterUpdateChecker = boost::hana::is_valid(
                [](auto *f, auto const *id, auto const *data) -> decltype((void) (f->filterUpdate(*id, *data))) {}
            );
            static constexpr bool UsesPartialHistory = 
                std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
                >;
            
            bool hasData = false;
            std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
            if (nextItem) {
                currentItem_ = std::move(*nextItem);
                auto const *dataPtr = Chain::extractData(currentItem_);
                if (dataPtr) {
                    hasData = true;
                    if constexpr (filterUpdateChecker(
                        (ChainItemFolder *) nullptr
                        , (std::string_view *) nullptr
                        , (typename Chain::DataType const *) nullptr
                    )) {
                        if (!folder_.filterUpdate(Chain::extractStorageIDStringView(currentItem_), *dataPtr)) {
                            hasData = false;
                        }
                    }
                    if (hasData) {
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (std::string_view *) nullptr
                            , (typename Chain::DataType const *) nullptr
                        )) {
                            folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        } else {
                            currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                        }
                    }
                }
            }
            if (hasData) {
                if constexpr (std::is_same_v<ResultTransformer, void>) {
                    this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                } else {
                    auto r = resultTransformer_.transform(stateSaver_, currentValue_);
                    if constexpr (std::is_same_v<std::decay_t<decltype(r)>, OutputType>) {
                        this->publish(env, std::move(r));
                    } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, std::optional<OutputType>>) {
                        if (r) {
                            this->publish(env, std::move(*r));
                        }
                    }
                }   
            }
        }
    public:
        ChainReader(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) :
            chain_(chain)
            , currentItem_()
            , folder_(std::move(folder))
            , currentValue_()
            , resultTransformer_(std::move(resultTransformer))
            , stateSaver_(chain)
        {}
        ~ChainReader() {
        }
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = delete;
        virtual void start(Env *env) override final {
            static const auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            currentValue_ = folder_.initialize(env, chain_); 
            if constexpr (!std::is_same_v<ResultTransformer, void>) {
                resultTransformer_.initialize(env);
            }
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForState(currentValue_));
            } else {
                currentItem_ = chain_->head(env);
            }
        }
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<OutputType>> importer(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}, ChainItemFolder &&folder=ChainItemFolder {}, std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()) {
            return infra::BasicWithTimeApp<Env>::template importer<OutputType>(new ChainReader(chain, pollingPolicy, std::move(folder), std::move(resultTransformer)));
        }
    };

    template <class ChainDataType>
    class TrivialChainIDAndDataFetchingFolder : public FolderUsingPartialHistoryInformation {
    public:
        using ResultType = std::optional<std::tuple<std::string, ChainDataType>>;
        static ResultType initialize(void *, void *) {
            return std::nullopt;
        }
        static ResultType fold(ResultType const &, std::string_view const &id, ChainDataType const &item) {
            return std::tuple<std::string, ChainDataType> {
                std::string {id}, item
            };
        }
    };
    template <class ChainDataType>
    class TrivialChainDataFetchingFolder : public FolderUsingPartialHistoryInformation {
    public:
        using ResultType = std::optional<ChainDataType>;
        static ResultType initialize(void *, void *) {
            return std::nullopt;
        }
        static ResultType fold(ResultType const &, std::string_view const &, ChainDataType const &item) {
            return {item};
        }
    };
    template <class ChainDataType, class Env>
    class TrivialChainDataFetchingFolderWithTime : public FolderUsingPartialHistoryInformation {
    private:
        std::function<typename Env::TimePointType(std::optional<ChainDataType> const &)> timeExtractor_;
    public:
        using ResultType = std::optional<ChainDataType>;
        TrivialChainDataFetchingFolderWithTime(std::function<typename Env::TimePointType(std::optional<ChainDataType> const &)> const &timeExtractor) 
            : timeExtractor_(timeExtractor) {}
        static ResultType initialize(void *, void *) {
            return std::nullopt;
        }
        static ResultType fold(ResultType const &, std::string_view const &, ChainDataType const &item) {
            return {item};
        }
        typename Env::TimePointType extractTime(ResultType const &r) {
            return timeExtractor_(r);
        }
    };
    class EmptyStateChainFolder {
    public:
        using ResultType = VoidStruct;
        static ResultType initialize(void *, void *) {
            return VoidStruct {};
        }
        template <class ChainDataType>
        static void foldInPlace(ResultType &, std::string_view const &, ChainDataType const &) {
        }
    };

    template <
        class FolderA
        , class FolderB
        , bool usesPartialHistory=
            std::is_convertible_v<FolderA *, FolderUsingPartialHistoryInformation *>
            ||
            std::is_convertible_v<FolderB *, FolderUsingPartialHistoryInformation *>
    >
    class CombinedFolder {};
    template <class FolderA, class FolderB>
    class CombinedFolder<FolderA, FolderB, false> {
    private:
        FolderA folderA_;
        FolderB folderB_;
    public:
        using ResultType = std::tuple<typename FolderA::ResultType, typename FolderB::ResultType>;
        CombinedFolder(FolderA &&folderA, FolderB &&folderB) 
            : folderA_(std::move(folderA)), folderB_(std::move(folderB))
        {}
        template <class Env, class Chain>
        ResultType initialize(Env *env, Chain *chain) {
            return {
                folderA_.initialize(env, chain)
                , folderB_.initialize(env, chain)
            };
        }
        template <class ChainDataType>
        void foldInPlace(ResultType &res, std::string_view const &id, ChainDataType const &data) {
            static const auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            if constexpr (foldInPlaceChecker(
                (FolderA *) nullptr
                , (typename FolderA::ResultType *) nullptr
                , (std::string_view *) nullptr
                , (ChainDataType const *) nullptr
            )) {
                folderA_.foldInPlace(std::get<0>(res), id, data);
            } else {
                std::get<0>(res) = folderA_.fold(std::get<0>(res), id, data);
            }
            if constexpr (foldInPlaceChecker(
                (FolderB *) nullptr
                , (typename FolderB::ResultType *) nullptr
                , (std::string_view *) nullptr
                , (ChainDataType const *) nullptr
            )) {
                folderB_.foldInPlace(std::get<1>(res), id, data);
            } else {
                std::get<1>(res) = folderB_.fold(std::get<1>(res), id, data);
            }
        }
    };
    template <class FolderA, class FolderB>
    class CombinedFolder<FolderA, FolderB, true> : public FolderUsingPartialHistoryInformation, public CombinedFolder<FolderA, FolderB, false> {
    public:
        CombinedFolder(FolderA &&folderA, FolderB &&folderB) 
            : FolderUsingPartialHistoryInformation()
                , CombinedFolder<FolderA, FolderB, false>(std::move(folderA), std::move(folderB))
        {}
    };

    template <class App, class ChainItemFolder, class TriggerT, class ResultTransformer>
    using ChainReaderActionFactory = std::function<
        std::shared_ptr<typename App::template Action<TriggerT, std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>>>(
        )
    >;
    template <class App, class ChainItemFolder, class ResultTransformer>
    using ChainReaderImporterFactory = std::function<
        std::shared_ptr<typename App::template Importer<std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>>>(
        )
    >;
} } } } }

#endif
