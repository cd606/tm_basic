#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/simple_shared_chain/FolderUsingPartialHistoryInformation.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainPollingPolicy.hpp>
#include <optional>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class App, class Chain, class ChainItemFolder, class TriggerT=VoidStruct>
    class ChainReader {
    private:
        using Env = typename App::EnvironmentType;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
    public:
        ChainReader(Env *env, Chain *chain) : 
            chain_(chain)
            , currentItem_()
            , folder_()
            , currentValue_(folder_.initialize(env, chain)) 
        {
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );
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
        typename std::optional<typename ChainItemFolder::ResultType> operator()(TriggerT &&) {
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            bool hasNew = false;
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
                    hasNew = true;
                }
            }
            if (hasNew) {
                return {currentValue_};
            } else {
                return std::nullopt;
            }
        }
        static std::shared_ptr<typename App::template Action<TriggerT, typename ChainItemFolder::ResultType>> action(Env *env, Chain *chain) {
            return App::template liftMaybe<TriggerT>(ChainReader(env, chain));
        }
    };

    template <class Env, class Chain, class ChainItemFolder>
    class ChainReader<infra::RealTimeApp<Env>,Chain,ChainItemFolder,void> final
        : public infra::RealTimeApp<Env>::template AbstractImporter<typename ChainItemFolder::ResultType> {
    private:
        const bool callbackPerUpdate_;
        const bool busyLoop_;
        const bool noYield_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
        bool running_;
        std::thread th_;
        void run(Env *env) {
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
            static constexpr bool UsesPartialHistory = 
                std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
                >;
            running_ = true;
            while (running_) {
                std::optional<typename Chain::ItemType> nextItem;
                bool hasData = false;
                do {
                    nextItem = chain_->fetchNext(currentItem_);
                    if (nextItem) {
                        currentItem_ = std::move(*nextItem);
                        auto const *dataPtr = Chain::extractData(currentItem_);
                        if (dataPtr) {
                            hasData = true;
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
                                this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                            } else {
                                if (callbackPerUpdate_) {
                                    this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                                }
                            }
                        }
                    }
                } while (nextItem);
                if constexpr (!UsesPartialHistory) {
                    if (!callbackPerUpdate_) {
                        if (hasData) {
                            this->publish(env, infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_));
                        }
                    }
                }
                if (busyLoop_) {
                    if (!noYield_) {
                        std::this_thread::yield();
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    public:
        ChainReader(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}) :
            callbackPerUpdate_(pollingPolicy.callbackPerUpdate)
            , busyLoop_(pollingPolicy.busyLoop)
            , noYield_(pollingPolicy.noYield)
            , chain_(chain)
            , currentItem_()
            , folder_()
            , currentValue_()
            , running_(false)
            , th_()
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
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            currentValue_ = folder_.initialize(env, chain_); 
            
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
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<typename ChainItemFolder::ResultType>> importer(Chain *chain, ChainPollingPolicy const &pollingPolicy=ChainPollingPolicy {}) {
            return infra::RealTimeApp<Env>::template importer<typename ChainItemFolder::ResultType>(new ChainReader(chain, pollingPolicy));
        }
    };

    //in SinglePassIteration, we have to check every update, therefore we don't
    //have the callbackPerUpdate option
    //also, in SinglePassIteration, the extractTime call must be there to provide
    //the timestamp of update
    template <class Env, class Chain, class ChainItemFolder>
    class ChainReader<infra::SinglePassIterationApp<Env>,Chain,ChainItemFolder,void> final
        : public infra::SinglePassIterationApp<Env>::template AbstractImporter<typename ChainItemFolder::ResultType> {
    private:
        Env *env_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
    public:
        ChainReader(Chain *chain) :
            env_(nullptr)
            , chain_(chain)
            , currentItem_()
            , folder_()
            , currentValue_()
        {}
        ~ChainReader() {}
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = delete;
        virtual void start(Env *env) override final {
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForState(*v)))) {}
            );

            env_ = env;
            currentValue_ = folder_.initialize(env, chain_); 
            
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
        virtual typename infra::SinglePassIterationApp<Env>::template Data<typename ChainItemFolder::ResultType> generate(typename ChainItemFolder::ResultType const *notUsed=nullptr) override final {
            static constexpr bool UsesPartialHistory = 
                std::is_convertible_v<
                    ChainItemFolder *, FolderUsingPartialHistoryInformation *
                >;
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *id, auto const *data) -> decltype((void) (f->foldInPlace(*v, *id, *data))) {}
            );
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
                        folder_.foldInPlace(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    } else {
                        currentValue_ = folder_.fold(currentValue_, Chain::extractStorageIDStringView(currentItem_), *dataPtr);
                    }
                    return typename infra::SinglePassIterationApp<Env>::template InnerData<typename ChainItemFolder::ResultType> {
                        env_
                        , {
                            env_->resolveTime(folder_.extractTime(currentValue_))
                            , infra::withtime_utils::ValueCopier<typename ChainItemFolder::ResultType>::copy(currentValue_)
                            , false
                        }
                    };
                } else {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
        }
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Importer<typename ChainItemFolder::ResultType>> importer(Chain *chain, ChainPollingPolicy const &notUsed=ChainPollingPolicy {}) {
            return infra::SinglePassIterationApp<Env>::template importer<typename ChainItemFolder::ResultType>(new ChainReader(chain));
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

    template <class App, class ChainItemFolder, class TriggerT>
    using ChainReaderActionFactory = std::function<
        std::shared_ptr<typename App::template Action<TriggerT, typename ChainItemFolder::ResultType>>(
        )
    >;
    template <class App, class ChainItemFolder>
    using ChainReaderImporterFactory = std::function<
        std::shared_ptr<typename App::template Importer<typename ChainItemFolder::ResultType>>(
        )
    >;
} } } } }

#endif