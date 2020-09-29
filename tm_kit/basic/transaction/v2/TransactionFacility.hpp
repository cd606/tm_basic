#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_FACILITY_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_FACILITY_HPP_

#include <tm_kit/basic/transaction/v2/TransactionInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/TransactionEnvComponent.hpp>
#include <tm_kit/basic/transaction/v2/DefaultUtilities.hpp>

#include <queue>
#include <array>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    enum class TransactionLoggingLevel {
        None, Verbose
    };

    template <class M, class TI, class DI, class KeyHash=std::hash<typename DI::Key>>
    class ITransactionProcessorBase {
    public:
        using DataStore = TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded>;
        using DataStorePtr = TransactionDataStorePtr<DI,KeyHash,M::PossiblyMultiThreaded>;
        virtual DataStorePtr const &dataStorePtr() const = 0;
    };

    template <class M, class TI, class DI, TransactionLoggingLevel TLogging=TransactionLoggingLevel::Verbose, class KeyHash=std::hash<typename DI::Key>>
    class ITransactionFacility : public M::template AbstractOnOrderFacility<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
        >, public ITransactionProcessorBase<M, TI, DI, KeyHash>
    {};

    template <class M, class TI, class DI, TransactionLoggingLevel TLogging=TransactionLoggingLevel::Verbose, class KeyHash=std::hash<typename DI::Key>>
    class ISilentTransactionHandler : public M::template AbstractExporter<
            typename TI::TransactionWithAccountInfo
        >, public ITransactionProcessorBase<M, TI, DI, KeyHash>
    {};

    template <
        class M, class TI, class DI
        , TransactionLoggingLevel TLogging=TransactionLoggingLevel::Verbose
        , class VersionChecker = std::equal_to<typename TI::Version>
        , class VersionSliceChecker = std::equal_to<typename TI::Version>
        , class DataSummaryChecker = std::equal_to<typename TI::Data>
        , class DeltaProcessor = TriviallyProcess<typename TI::Data, typename TI::DataDelta, typename TI::ProcessedUpdate>
        , class KeyHash = std::hash<typename DI::Key>
        , bool Silent = false
        , class Enable = void
    >
    class TransactionProcessor {};

    template <
        class M, class TI, class DI
        , TransactionLoggingLevel TLogging
        , class VersionChecker, class VersionSliceChecker, class DataSummaryChecker
        , class DeltaProcessor, class KeyHash, bool Silent
    >
    class TransactionProcessor<
        M, TI, DI
        , TLogging
        , VersionChecker, VersionSliceChecker, DataSummaryChecker
        , DeltaProcessor, KeyHash, Silent
        , std::enable_if_t<
            std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
            && std::is_same_v<typename TI::Key, typename DI::Key>
            && std::is_same_v<typename TI::Version, typename DI::Version>
            && std::is_same_v<typename TI::Data, typename DI::Data>
        >
    >
        : 
        public std::conditional_t<
            Silent
            , ISilentTransactionHandler<M, TI, DI, TLogging, KeyHash>
            , ITransactionFacility<M, TI, DI, TLogging, KeyHash> 
        >
        , public virtual M::IExternalComponent
    {
    
    private:
        using TH = TransactionEnvComponent<TI>;
        using Lock = typename TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded>::Lock;

        TransactionDataStorePtr<DI,KeyHash,M::PossiblyMultiThreaded> dataStore_;

        VersionChecker versionChecker_;
        VersionSliceChecker versionSliceChecker_;
        DataSummaryChecker dataSummaryChecker_;
        DeltaProcessor deltaProcessor_;

        struct ThreadDataImpl {
            using TransactionQueue = std::deque<
                std::tuple<typename M::EnvironmentType *, typename M::EnvironmentType::IDType, typename TI::TransactionWithAccountInfo>
            >;
            std::array<TransactionQueue, 2> transactionQueues_;
            size_t transactionQueueIncomingIndex_ = 0;

            std::thread transactionThread_;
            std::atomic<bool> running_ = false;
            std::mutex transactionQueueMutex_;
            std::condition_variable transactionQueueCond_;
        };
        using ThreadData = std::conditional_t<
            M::PossiblyMultiThreaded
            , ThreadDataImpl
            , bool
        >;
        ThreadData threadData_;

        void publishResponse(typename M::EnvironmentType *env, typename M::EnvironmentType::IDType const &id, typename TI::TransactionResponse &&response) {
            if constexpr (!Silent) {
                this->publish(
                    env
                    , typename M::template Key<typename TI::TransactionResponse> {
                        id, std::move(response)
                    }
                    , true
                );
            }
            //The busy wait is important, since it makes sure that our data store
            //is actually up to date
            dataStore_->waitForGlobalVersion(response.value.globalVersion);
        }

        struct ComponentLock {
            TH *th_;
            std::string const *acct_;
            typename TI::Key const *key_;
            typename TI::DataDelta const *dataDelta_; 
            TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded> *dataStorePtr_;
            ComponentLock(TH *th, std::string const *acct, typename TI::Key const *k, typename TI::DataDelta const *dd, TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded> *dataStorePtr) : th_(th), acct_(acct), key_(k), dataDelta_(dd), dataStorePtr_(dataStorePtr) {
                dataStorePtr_->waitForGlobalVersion(th_->acquireLock(*acct_, *key_, dataDelta_));
            }
            ~ComponentLock() {
                dataStorePtr_->waitForGlobalVersion(th_->releaseLock(*acct_, *key_, dataDelta_));
            }
        };

        void handleInsert(typename M::EnvironmentType *env, std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::InsertAction insertAction) {
            auto *th = static_cast<TH *>(env);
            ComponentLock cl(th, &account, &(insertAction.key), nullptr, dataStore_.get());
            {
                Lock _(dataStore_->mutex_);
                auto iter = dataStore_->dataMap_.find(insertAction.key);
                if (iter != dataStore_->dataMap_.end() && iter->second.data) {
                    publishResponse(env, requester, typename TI::TransactionResponse {
                        dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                    });
                    return;
                }
            }
            publishResponse(env, requester, th->handleInsert(account, insertAction.key, insertAction.data));
        }
        void handleUpdate(typename M::EnvironmentType *env, std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::UpdateAction updateAction) {
            std::optional<typename TI::ProcessedUpdate> processed = std::nullopt;
            auto *th = static_cast<TH *>(env);
            ComponentLock cl(th, &account, &(updateAction.key), &(updateAction.dataDelta), dataStore_.get());
            {
                Lock _(dataStore_->mutex_);
                auto iter = const_cast<
                    typename TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded>::DataMap const &
                >(dataStore_->dataMap_).find(updateAction.key);
                if (iter == dataStore_->dataMap_.end()) {
                    publishResponse(env, requester, typename TI::TransactionResponse {
                        dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                    });
                    return;
                }
                if (!iter->second.data) {
                    publishResponse(env, requester, typename TI::TransactionResponse {
                        dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                    });
                    return;
                }
                if (updateAction.oldVersionSlice) {
                    if (!versionSliceChecker_(iter->second.version, *(updateAction.oldVersionSlice))) {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                        });
                        return;
                    }
                }
                if (updateAction.oldDataSummary) {
                    if (!dataSummaryChecker_(*(iter->second.data), *(updateAction.oldDataSummary))) {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                        });
                        return;
                    }
                }
                processed = deltaProcessor_(*(iter->second.data), updateAction.dataDelta);
                if (!processed) {
                    publishResponse(env, requester, typename TI::TransactionResponse {
                        dataStore_->globalVersion_, RequestDecision::FailureConsistency
                    });
                    return;
                }
            }
            publishResponse(env, requester, th->handleUpdate(account, updateAction.key, updateAction.oldVersionSlice, *processed));
        }
        void handleDelete(typename M::EnvironmentType *env, std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::DeleteAction deleteAction) {
            auto *th = static_cast<TH *>(env);
            ComponentLock cl(th, &account, &(deleteAction.key), nullptr, dataStore_.get());
            {
                Lock _(dataStore_->mutex_);
                auto iter = dataStore_->dataMap_.find(deleteAction.key);
                if (iter == dataStore_->dataMap_.end()) {
                    if (!deleteAction.oldVersion) {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::Success
                        });
                    } else {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                        });
                    }
                    return;
                }
                if (!iter->second.data) {
                    if (!deleteAction.oldVersion) {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::Success
                        });
                    } else {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                        });
                    }
                    return;
                }
                if (deleteAction.oldVersion) {
                    if (!versionChecker_(iter->second.version, *(deleteAction.oldVersion))) {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                        });
                        return;
                    }
                }
                if (deleteAction.oldDataSummary) {
                    if (!dataSummaryChecker_(*(iter->second.data), *(deleteAction.oldDataSummary))) {
                        publishResponse(env, requester, typename TI::TransactionResponse {
                            dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                        });
                        return;
                    }
                }
            }
            publishResponse(env, requester, th->handleDelete(account, deleteAction.key, deleteAction.oldVersion));
        }
        void reallyHandle(typename M::EnvironmentType *env, typename M::EnvironmentType::IDType &&id, typename TI::TransactionWithAccountInfo &&transactionWithAccountInfo) {
            auto account = std::move(std::get<0>(transactionWithAccountInfo));
            auto requester = std::move(id);
            if constexpr (TLogging == TransactionLoggingLevel::Verbose) {
                std::ostringstream oss;
                oss << "[TransactionProcessor::reallyHandle] Got input from '" << account << "', id is '" << env->id_to_string(requester) << "', content is ";
                PrintHelper<typename TI::Transaction>::print(oss, std::get<1>(transactionWithAccountInfo));
                env->log(infra::LogLevel::Info, oss.str());
            }
            std::visit([this,env,&account,&requester](auto &&tr) {
                using T = std::decay_t<decltype(tr)>;
                if constexpr (std::is_same_v<T, typename TI::InsertAction>) {
                    auto insertAction = std::move(tr);
                    if (!checkInsertPermission(env, account, insertAction)) {
                        if constexpr (!Silent) {
                            this->publish(
                                env
                                , typename M::template Key<typename TI::TransactionResponse> {
                                    requester
                                    , typename TI::TransactionResponse { {
                                        typename TI::GlobalVersion {}
                                        , RequestDecision::FailurePermission
                                    } }
                                }
                                , true
                            );
                        }
                        return;
                    }
                    handleInsert(env, account, requester, insertAction);
                } else if constexpr (std::is_same_v<T, typename TI::UpdateAction>) {
                    auto updateAction = std::move(tr);
                    if (!checkUpdatePermission(env, account, updateAction)) {
                        if constexpr (!Silent) {
                            this->publish(
                                env
                                , typename M::template Key<typename TI::TransactionResponse> {
                                    requester
                                    , typename TI::TransactionResponse { {
                                        typename TI::GlobalVersion {}
                                        , RequestDecision::FailurePermission
                                    } }
                                }
                                , true
                            );
                        }
                        return;
                    }
                    handleUpdate(env, account, requester, updateAction);
                } else if constexpr (std::is_same_v<T, typename TI::DeleteAction>) {
                    auto deleteAction = std::move(tr);
                    if (!checkDeletePermission(env, account, deleteAction)) {
                        if constexpr (!Silent) {
                            this->publish(
                                env
                                , typename M::template Key<typename TI::TransactionResponse> {
                                    requester
                                    , typename TI::TransactionResponse { {
                                        typename TI::GlobalVersion {}
                                        , RequestDecision::FailurePermission
                                    } }
                                }
                                , true
                            );
                        }
                        return;
                    }
                    handleDelete(env, account, requester, deleteAction);
                }
            }, std::move(std::get<1>(transactionWithAccountInfo)).value);
            if constexpr (TLogging == TransactionLoggingLevel::Verbose) {
                std::ostringstream oss;
                oss << "[TransactionProcessor::reallyHandle] Finished handling input with id '" << env->id_to_string(requester) << "'";
                env->log(infra::LogLevel::Info, oss.str());
            }
        }
        void runTransactionThread() {
            if constexpr (M::PossiblyMultiThreaded) {
                while (threadData_.running_) {
                    std::unique_lock<std::mutex> lock(threadData_.transactionQueueMutex_);
                    threadData_.transactionQueueCond_.wait_for(lock, std::chrono::milliseconds(1));
                    if (!threadData_.running_) {
                        lock.unlock();
                        break;
                    }
                    if (threadData_.transactionQueues_[threadData_.transactionQueueIncomingIndex_].empty()) {
                        lock.unlock();
                        continue;
                    }
                    int processingIndex = threadData_.transactionQueueIncomingIndex_;
                    threadData_.transactionQueueIncomingIndex_ = 1-threadData_.transactionQueueIncomingIndex_;
                    lock.unlock();

                    {
                        while (!threadData_.transactionQueues_[processingIndex].empty()) {
                            auto &t = threadData_.transactionQueues_[processingIndex].front();
                            reallyHandle(
                                std::get<0>(t)
                                , std::move(std::get<1>(t))
                                , std::move(std::get<2>(t))
                            );
                            threadData_.transactionQueues_[processingIndex].pop_front();
                        }
                    }
                }
            }
        }
    public:
        TransactionProcessor(TransactionDataStorePtr<DI,KeyHash,M::PossiblyMultiThreaded> const &dataStore) 
            : dataStore_(dataStore)
            , versionChecker_()
            , versionSliceChecker_()
            , dataSummaryChecker_()
            , deltaProcessor_()
            , threadData_()
        {
        }
        TransactionProcessor &setVersionChecker(VersionChecker &&vc) {
            versionChecker_ = std::move(vc);
            return *this;
        }
        TransactionProcessor &setVersionSliceChecker(VersionSliceChecker &&vc) {
            versionSliceChecker_ = std::move(vc);
            return *this;
        }
        TransactionProcessor &setDataSummaryChecker(DataSummaryChecker &&dc) {
            dataSummaryChecker_ = std::move(dc);
            return *this;
        }
        TransactionProcessor &setDeltaProcessor(DeltaProcessor &&dp) {
            deltaProcessor_ = std::move(dp);
            return *this;
        }
        ~TransactionProcessor() {
            if constexpr (M::PossiblyMultiThreaded) {
                if (threadData_.running_) {
                    threadData_.running_ = false;
                    threadData_.transactionThread_.join();
                }
            }
        }
        virtual TransactionDataStorePtr<DI,KeyHash> const &dataStorePtr() const override final {
            return dataStore_;
        }
        void start(typename M::EnvironmentType *) override final {
            if constexpr (M::PossiblyMultiThreaded) {
                threadData_.running_ = true;
                threadData_.transactionThread_ = std::thread(&TransactionProcessor::runTransactionThread, this);
                threadData_.transactionThread_.detach();
            }
        }
        void handle(typename M::template InnerData<
            std::conditional_t<
                Silent
                , typename TI::TransactionWithAccountInfo
                , typename M::template Key<
                    typename TI::TransactionWithAccountInfo
                >
            >
        > &&transactionWithAccountInfo) override final {
            if constexpr (!Silent) {
                auto *env = transactionWithAccountInfo.environment;
                if constexpr (M::PossiblyMultiThreaded) {
                    if (threadData_.running_) {
                        {
                            std::lock_guard<std::mutex> _(threadData_.transactionQueueMutex_);
                            threadData_.transactionQueues_[threadData_.transactionQueueIncomingIndex_].push_back({
                                env
                                , std::move(transactionWithAccountInfo.timedData.value.id())
                                , std::move(transactionWithAccountInfo.timedData.value.key())
                            });
                        }
                        threadData_.transactionQueueCond_.notify_one();
                    }
                } else {
                    reallyHandle(
                        env
                        , std::move(transactionWithAccountInfo.timedData.value.id())
                        , std::move(transactionWithAccountInfo.timedData.value.key())
                    );
                }
            } else {
                static typename M::EnvironmentType::IDType emptyID;

                auto *env = transactionWithAccountInfo.environment;
                if constexpr (M::PossiblyMultiThreaded) {
                    if (threadData_.running_) {
                        {
                            std::lock_guard<std::mutex> _(threadData_.transactionQueueMutex_);
                            threadData_.transactionQueues_[threadData_.transactionQueueIncomingIndex_].push_back({
                                env
                                , emptyID
                                , std::move(transactionWithAccountInfo.timedData.value)
                            });
                        }
                        threadData_.transactionQueueCond_.notify_one();
                    }
                } else {
                    reallyHandle(
                        env
                        , emptyID
                        , std::move(transactionWithAccountInfo.timedData.value)
                    );
                }
            }
        }
        virtual bool checkInsertPermission(typename M::EnvironmentType *env, std::string const &account, typename TI::InsertAction const &) {
            return true;
        }
        virtual bool checkUpdatePermission(typename M::EnvironmentType *env, std::string const &account, typename TI::UpdateAction const &) {
            return true;
        }
        virtual bool checkDeletePermission(typename M::EnvironmentType *env, std::string const &account, typename TI::DeleteAction const &) {
            return true;
        }
    };

    template <
        class M, class TI, class DI
        , TransactionLoggingLevel TLogging=TransactionLoggingLevel::Verbose
        , class VersionChecker = std::equal_to<typename TI::Version>
        , class VersionSliceChecker = std::equal_to<typename TI::Version>
        , class DataSummaryChecker = std::equal_to<typename TI::Data>
        , class DeltaProcessor = TriviallyProcess<typename TI::Data, typename TI::DataDelta, typename TI::ProcessedUpdate>
        , class KeyHash = std::hash<typename DI::Key>
    >
    using TransactionFacility = 
        TransactionProcessor<
            M, TI, DI, TLogging
            , VersionChecker, VersionSliceChecker, DataSummaryChecker
            , DeltaProcessor, KeyHash
            , false
            , std::enable_if_t<
                std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
                && std::is_same_v<typename TI::Key, typename DI::Key>
                && std::is_same_v<typename TI::Version, typename DI::Version>
                && std::is_same_v<typename TI::Data, typename DI::Data>
            >
        >;

    template <
        class M, class TI, class DI
        , TransactionLoggingLevel TLogging=TransactionLoggingLevel::Verbose
        , class VersionChecker = std::equal_to<typename TI::Version>
        , class VersionSliceChecker = std::equal_to<typename TI::Version>
        , class DataSummaryChecker = std::equal_to<typename TI::Data>
        , class DeltaProcessor = TriviallyProcess<typename TI::Data, typename TI::DataDelta, typename TI::ProcessedUpdate>
        , class KeyHash = std::hash<typename DI::Key>
    >
    using TransactionHandlingExporter = 
        TransactionProcessor<
            M, TI, DI, TLogging
            , VersionChecker, VersionSliceChecker, DataSummaryChecker
            , DeltaProcessor, KeyHash
            , true
            , std::enable_if_t<
                std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
                && std::is_same_v<typename TI::Key, typename DI::Key>
                && std::is_same_v<typename TI::Version, typename DI::Version>
                && std::is_same_v<typename TI::Data, typename DI::Data>
            >
        >;

} } 

} } } }

#endif