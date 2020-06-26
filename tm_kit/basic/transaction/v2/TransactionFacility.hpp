#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_FACILITY_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_FACILITY_HPP_

#include <tm_kit/basic/transaction/v2/TransactionInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/TransactionEnvComponent.hpp>
#include <tm_kit/basic/transaction/v2/DefaultUtilities.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    template <class M, class TI, class DI, class KeyHash=std::hash<typename DI::Key>, bool MutexProtected=true>
    class ITransactionFacility : public M::template AbstractOnOrderFacility<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
        > {
    public:
        using DataStore = TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded>;
        using DataStorePtr = TransactionDataStorePtr<DI,KeyHash,M::PossiblyMultiThreaded>;
        virtual DataStorePtr const &dataStorePtr() const = 0;
    };

    template <
        class M, class TI, class DI
        , class VersionChecker = std::equal_to<typename TI::Version>
        , class VersionSliceChecker = std::equal_to<typename TI::Version>
        , class DataSummaryChecker = std::equal_to<typename TI::Data>
        , class DeltaProcessor = TriviallyProcess<typename TI::Data, typename TI::DataDelta, typename TI::ProcessedUpdate>
        , class KeyHash = std::hash<typename DI::Key>
        , class Enable=void
    >
    class TransactionFacility {};

    template <
        class M, class TI, class DI
        , class VersionChecker, class VersionSliceChecker, class DataSummaryChecker
        , class DeltaProcessor, class KeyHash
    >
    class TransactionFacility<
        M, TI, DI
        , VersionChecker, VersionSliceChecker, DataSummaryChecker
        , DeltaProcessor, KeyHash
        , std::enable_if_t<
            std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
            && std::is_same_v<typename TI::Key, typename DI::Key>
            && std::is_same_v<typename TI::Version, typename DI::Version>
            && std::is_same_v<typename TI::Data, typename DI::Data>
        >
    >
        : 
        public ITransactionFacility<M, TI, DI, KeyHash, M::PossiblyMultiThreaded> 
    {
    private:
        using TH = TransactionEnvComponent<TI>;
        using Lock = typename TransactionDataStore<DI,KeyHash,M::PossiblyMultiThreaded>::Lock;

        TransactionDataStorePtr<DI,KeyHash,M::PossiblyMultiThreaded> dataStore_;

        VersionChecker versionChecker_;
        VersionSliceChecker versionSliceChecker_;
        DataSummaryChecker dataSummaryChecker_;
        DeltaProcessor deltaProcessor_;

        void publishResponse(typename M::EnvironmentType *env, typename M::EnvironmentType::IDType const &id, typename TI::TransactionResponse &&response) {
            this->publish(
                env
                , typename M::template Key<typename TI::TransactionResponse> {
                    id, std::move(response)
                }
                , true
            );
            //The busy wait is important, since it makes sure that our data store
            //is actually up to date
            dataStore_->waitForGlobalVersion(response.value.globalVersion);
        }

        struct ComponentLock {
            TH *th_;
            std::string const *acct_;
            typename TI::Key const *key_;
            ComponentLock(TH *th, std::string const *acct, typename TI::Key const *k) : th_(th), acct_(acct), key_(k) {
                th_->acquireLock(*acct_, *key_);
            }
            ~ComponentLock() {
                th_->releaseLock(*acct_, *key_);
            }
        };

        void handleInsert(typename M::EnvironmentType *env, std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::InsertAction insertAction) {
            auto *th = static_cast<TH *>(env);
            ComponentLock cl(th, &account, &(insertAction.key));
            {
                Lock _(dataStore_->mutex_);
                auto iter = dataStore_->dataMap_.find(insertAction.key);
                if (iter != dataStore_->dataMap_.end()) {
                    publishResponse(env, requester, typename TI::TransactionResponse {
                        dataStore_->globalVersion_, RequestDecision::FailurePrecondition
                    });
                    return;
                }
                if (iter->second.data) {
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
            ComponentLock cl(th, &account, &(updateAction.key));
            {
                Lock _(dataStore_->mutex_);
                auto iter = dataStore_->dataMap_.find(updateAction.key);
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
            ComponentLock cl(th, &account, &(deleteAction.key));
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
    public:
        TransactionFacility(TransactionDataStorePtr<DI,KeyHash,M::PossiblyMultiThreaded> const &dataStore) 
            : dataStore_(dataStore)
            , versionChecker_()
            , versionSliceChecker_()
            , dataSummaryChecker_()
            , deltaProcessor_()
        {}
        TransactionFacility &setVersionChecker(VersionChecker &&vc) {
            versionChecker_ = std::move(vc);
            return *this;
        }
        TransactionFacility &setVersionSliceChecker(VersionSliceChecker &&vc) {
            versionSliceChecker_ = std::move(vc);
            return *this;
        }
        TransactionFacility &setDataSummaryChecker(DataSummaryChecker &&dc) {
            dataSummaryChecker_ = std::move(dc);
            return *this;
        }
        TransactionFacility &setDeltaProcessor(DeltaProcessor &&dp) {
            deltaProcessor_ = std::move(dp);
            return *this;
        }
        virtual TransactionDataStorePtr<DI,KeyHash> const &dataStorePtr() const override final {
            return dataStore_;
        }
        void handle(typename M::template InnerData<typename M::template Key<
            typename TI::TransactionWithAccountInfo
        >> &&transactionWithAccountInfo) override final {
            auto *env = transactionWithAccountInfo.environment;
            auto account = std::get<0>(transactionWithAccountInfo.timedData.value.key());
            auto requester = transactionWithAccountInfo.timedData.value.id();
            std::visit([this,env,&account,&requester](auto &&tr) {
                using T = std::decay_t<decltype(tr)>;
                if constexpr (std::is_same_v<T, typename TI::InsertAction>) {
                    auto insertAction = std::move(tr);
                    handleInsert(env, account, requester, insertAction);
                } else if constexpr (std::is_same_v<T, typename TI::UpdateAction>) {
                    auto updateAction = std::move(tr);
                    handleUpdate(env, account, requester, updateAction);
                } else if constexpr (std::is_same_v<T, typename TI::DeleteAction>) {
                    auto deleteAction = std::move(tr);
                    handleDelete(env, account, requester, deleteAction);
                }
            }, std::move(std::get<1>(transactionWithAccountInfo.timedData.value.key())).value);
        }
    };

} } 

} } } }

#endif