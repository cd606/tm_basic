#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_STORAGE_TRANSACTION_BROKER_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_STORAGE_TRANSACTION_BROKER_HPP_

#include <tm_kit/basic/SingleKeyLocalTransactionHandlerComponent.hpp>
#include <tm_kit/basic/transaction/VersionProviderComponent.hpp>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {
    
    template <
        class M
        , class KeyType
        , class DataType
        , class VersionType
        , class DataSummaryType = DataType
        , class Cmp = std::less<VersionType>
        , class CheckSummary = std::equal_to<DataType>
        , class Enable = void
    >
    class SingleKeyLocalStorageTransactionBroker {
    };

    template <
        class M
        , class KeyType
        , class DataType
        , class VersionType
        , class DataSummaryType
        , class Cmp
        , class CheckSummary
    >
    class SingleKeyLocalStorageTransactionBroker<
        KeyType, DataType, VersionType, Env, DataSummaryType, Cmp, CheckSummary
        std::enable_if_t<
            std::is_base_of_v<
                SingleKeyLocalTransactionHandlerComponent<
                    KeyType,DataType,VersionType,typename M::IDType,DataSummaryType,Cmp,CheckSummary
                >
                , typename M::EnvironmentType
            > 
            &&
            std::is_base_of_v<
                VersionProviderComponent<
                    KeyType,DataType,VersionType
                >
                , typename M::EnvironmentType
            > 
        >
    > final
        :
    public M::IExternalComponent
    , public M::template AbstractOnOrderFacility<
        typename SingleKeyTransactionInterface<KeyType,Datatype,VersionType,typename M::IDType,DataSummaryType,Cmp,CheckSummary>
                ::FacilityInput
        , typename SingleKeyTransactionInterface<KeyType,Datatype,VersionType,typename M::IDType,DataSummaryType,Cmp,CheckSummary>
                ::FacilityOutput
    >
    {
    private:
        using TI = SingleKeyTransactionInterface<KeyType,Datatype,VersionType,typename M::IDType,DataSummaryType,Cmp,CheckSummary>;
        using TH = SingleKeyLocalTransactionHandlerComponent<KeyType,DataType,VersionType,typename M::IDType,DataSummaryType,Cmp,CheckSummary>;
        using VP = VersionProviderComponent<KeyType,DataType,VersionType>;

        struct OneKeyInfo {
            typename TI::OneValue currentValue;
            std::unordered_set<typename M::IDType> subscribers;
        };
        std::unordered_map<KeyType, OneKeyInfo> dataMap_;
        std::mutex mutex_;
        CheckSummary checkSummary_;

        bool checkTransactionPrecondition(typename M::IDType const &requester, typename TI::InsertAction const &insertAction) {
            auto iter = dataMap_.find(insertAction.key);
            if (iter == dataMap_.end()) {
                return true;
            }
            if (!iter->second.currentValue.data) {
                return true;
            }
            return false;
        }
        bool checkTransactionPrecondition(typename M::IDType const &requester, typename TI::UpdateAction const &updateAction) { 
            auto iter = dataMap_.find(updateAction.key);
            if (iter == dataMap_.end()) {
                return false;
            }
            if (!iter->second.currentValue.data) {
                return false;
            }
            if (iter->second.currentValue.version != updateAction.oldVersion) {
                return false;
            }
            if (!checkSummary_(iter->second.currentValue, updateAction.oldData)) {
                return false;
            }
            return true;
        }
        bool checkTransactionPrecondition(typename M::IDType const &requester, typename TI::DeleteAction const &deleteAction) {
            auto iter = dataMap_.find(deleteAction.key);
            if (iter == dataMap_.end()) {
                return false;
            }
            if (!iter->second.currentValue.data) {
                return false;
            }
            return true;
        }

        void handleTransactionResult(typename M::EnvironmentType *env, typename M::IDType const &requester, typename TI::InsertAction const &insertAction, VP *vp) {
            auto iter = dataMap_.find(insertAction.key);
            if (iter == dataMap_.end()) {
                iter = dataMap_.insert({
                    insertAction.key
                    , OneKeyInfo {
                        typename TI::OneValue {
                            insertAction.key
                            , vp->getNextVersionForKey(insertAction.key)
                            , insertAction.data
                        }
                        , std::unordered_set<typename M::IDType> {}
                    }
                }).first;
            } else {
                iter->second.currentValue = typename TI::OneValue {
                    insertAction.key
                    , vp->getNextVersionForKey(insertAction.key)
                    , insertAction.data
                };
            }
            for (auto const &id : iter->second.subscribers) {
                this->publish(
                    env
                    , typename M::template Key<typename TI::FacilityOutput> {
                        requester, iter->second.currentValue
                    }
                    , false
                );
            }
        }
        void handleTransactionResult(typename M::EnvironmentType *env, typename M::IDType const &requester, typename TI::UpdateAction const &updateAction, VP *vp) {
            auto iter = dataMap_.find(updateAction.key);
            if (iter == dataMap_.end()) {
                iter = dataMap_.insert({
                    updateAction.key
                    , OneKeyInfo {
                        typename TI::OneValue {
                            updateAction.key
                            , vp->getNextVersionForKey(updateAction.key)
                            , updateAction.newData
                        }
                        , std::unordered_set<typename M::IDType> {}
                    }
                }).first;
            } else {
                iter->second.currentValue = typename TI::OneValue {
                    updateAction.key
                    , vp->getNextVersionForKey(updateAction.key)
                    , updateAction.newData
                };
            }
            for (auto const &id : iter->second.subscribers) {
                this->publish(
                    env,
                    typename M::template Key<typename TI::FacilityOutput> {
                        requester, iter->second.currentValue
                    }
                    , false
                );
            }
        }
        void handleTransactionResult(typename M::EnvironmentType *env, typename M::IDType const &requester, typename TI::DeleteAction const &deleteAction, VP *vp) {
            auto iter = dataMap_.find(deleteAction.key);
            if (iter == dataMap_.end()) {
                iter = dataMap_.insert({
                    deleteAction.key
                    , OneKeyInfo {
                        typename TI::OneValue {
                            deleteAction.key
                            , vp->getNextVersionForKey(updateAction.key)
                            , std::nullopt
                        }
                        , std::unordered_set<typename M::IDType> {}
                    }
                }).first;
            } else {
                iter->second.currentValue = typename TI::OneValue {
                    deleteAction.key
                    , vp->getNextVersionForKey(deleteAction.key)
                    , std::nullopt
                };
            }
            for (auto const &id : iter->second.subscribers) {
                this->publish(
                    env
                    , typename M::template Key<typename TI::FacilityOutput> {
                        requester, iter->second.currentValue
                    }
                    , false);
            }
        }
        void handleSubscription(typename M::EnvironmentType *env, typename M::IDType const &requester, KeyType const &key, VP *vp) {
            auto iter = dataMap_.find(key);
            if (iter == dataMap_.end()) {
                iter = dataMap_.insert({
                    key
                    , OneKeyInfo {
                        typename TI::OneValue {
                            key
                            , vp->getNextVersionForKey(key)
                            , std::nullopt
                        }
                        , std::unordered_set<typename M::IDType> {}
                    }
                }).first;
            }
            iter->second.subscribers.insert(requester);
            this->publish(
                env
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::SubscriptionAck {}
                }
                , false);
            this->publish(
                env
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, iter->second.currentValue
                }
                , false);
        }
        void handleUnsubscription(typename M::EnvironmentType *env, typename M::IDType const &requester, typename M::IDType const &originalSubscriptionID, KeyType const &key, VP *vp) {
            auto iter = dataMap_.find(key);
            if (iter == dataMap_.end()) {
                this->publish(
                    env
                    , typename M::template Key<typename TI::FacilityOutput> {
                        requester, typename TI::UnsubscriptionAck {}
                    }
                    , true);
                return;
            }
            auto innerIter = iter->second.subscribers.find(originalSubscriptionID);
            if (innerIter == iter->second.end()) {
                this->publish(
                    env
                    , typename M::template Key<typename TI::FacilityOutput> {
                        requester, typename TI::UnsubscriptionAck {}
                    }
                    , true
                );
                return;
            }
            this->publish(
                env
                , typename M::template Key<typename TI::FacilityOutput> {
                    innerIter->second, typename TI::UnsubscriptionAck {}
                }
                , true
            );
            iter->second.erase(innerIter);
            this->publish(
                env
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::UnsubscriptionAck {}
                }
                , true
            );
            return;
        }
    public:
        virtual void start(typename M::EnvironmentType *env) {
            auto *vp = static_cast<VP *>(env);
            auto initialValues = static_cast<TH *>(env)->loadInitialData();
            std::lock_guard<std::mutex> _(mutex_);
            for (auto const &v : initialValues) {
                auto const &key = std::get<0>(v);
                dataMap_.insert({
                    key
                    , OneKeyInfo {
                        typename TI::OneValue {
                            key
                            , vp->getNextVersionForKey(key)
                            , std::get<1>(v)
                        }
                        , std::unordered_set<typename M::IDType> {}
                    }
                });
            }
        }
        virtual void handle(typename M::template InnerData<typename M::template Key<SingleKeyTransactionInterface<KeyType,Datatype,VersionType,DataSummaryType,Cmp>::FacilityInput>> &&input) override final {
            auto *env = input.environment;
            auto *handler = static_cast<TH *>(env);
            auto *versionProvider = static_cast<VP *>(env);
            auto const &account = std::get<0>(input.timedData.value.key());
            auto const &id = input.timedData.value.id();

            std::lock_guard<std::mutex> _(mutex_);
            switch (input.timedData.value.key().which()) {
            case 0:
                {
                    auto const &transaction = std::get<0>(input.timedData.value.key());
                    switch (transaction.which()) {
                    case 0:
                        {
                            auto const &insertAction = std::get<0>(transaction);
                            if (!this->checkTransactionPrecondition(id, insertAction)) {
                                typename T::TransactionResult res = typename TI::TransactionFailurePrecondition {};
                                this->publish(
                                    env
                                    , typename M::template Key<typename TI::FacilityOutput> {
                                        requester, res
                                    }
                                    , true
                                );
                                return;
                            }
                            bool actionRes = handler->handleInsert(insertAction.key, insertAction.data);
                            if (!actionRes) {
                                typename T::TransactionResult res = typename TI::TransactionFailurePermission {};
                                this->publish(
                                    env
                                    , typename M::template Key<typename TI::FacilityOutput> {
                                        requester, res
                                    }
                                    , true
                                );
                                return;
                            } 
                            this->handleTransactionResult(id, insertAction, versionProvider);
                            typename T::TransactionResult res = typename TI::TransactionSuccess {};
                            this->publish(
                                env
                                , typename M::template Key<typename TI::FacilityOutput> {
                                    requester, res
                                }
                                , true
                            );
                        }
                        break;
                    case 1:
                        {
                            auto const &updateAction = std::get<1>(transaction);
                            if (!this->checkTransactionPrecondition(id, updateAction)) {
                                typename T::TransactionResult res = typename TI::TransactionFailurePrecondition {};
                                this->publish(
                                    env
                                    , typename M::template Key<typename TI::FacilityOutput> {
                                        requester, res
                                    }
                                    , true);
                                return;
                            }
                            bool actionRes = handler->handleUpdate(updateAction.key, updateAction.newData);
                            if (!actionRes) {
                                typename T::TransactionResult res = typename TI::TransactionFailurePermission {};
                                this->publish(
                                    env
                                    , typename M::template Key<typename TI::FacilityOutput> {
                                        requester, res
                                    }
                                    , true);
                                return;
                            } 
                            this->handleTransactionResult(id, updateAction, versionProvider);
                            typename T::TransactionResult res = typename TI::TransactionSuccess {};
                            this->publish(
                                env
                                , typename M::template Key<typename TI::FacilityOutput> {
                                    requester, res
                                }
                                , true
                            );
                        }
                        break;
                    case 2:
                        {
                            auto const &deleteAction = std::get<2>(transaction);
                            if (!this->checkTransactionPrecondition(id, deleteAction)) {
                                typename T::TransactionResult res = typename TI::TransactionFailurePrecondition {};
                                this->publish(
                                    env
                                    , typename M::template Key<typename TI::FacilityOutput> {
                                        requester, res
                                    }
                                    , true);
                                return;
                            }
                            bool actionRes = handler->handleDelete(deleteAction.key);
                            if (!actionRes) {
                                typename T::TransactionResult res = typename TI::TransactionFailurePermission {};
                                this->publish(
                                    env
                                    , typename M::template Key<typename TI::FacilityOutput> {
                                        requester, res
                                    }
                                    , true);
                                return;
                            } 
                            this->handleTransactionResult(id, deleteAction, versionProvider);
                            typename T::TransactionResult res = typename TI::TransactionSuccess {};
                            this->publish(
                                env
                                , typename M::template Key<typename TI::FacilityOutput> {
                                    requester, res
                                }
                                , true
                            );                        
                        }
                        break;
                    default:
                        {
                            typename T::TransactionResult res = typename TI::TransactionFailurePermission {};
                            this->publish(
                                env
                                , typename M::template Key<typename TI::FacilityOutput> {
                                    requester, res
                                }
                                , true
                            );
                        }
                        break;
                    }
                }
                break;
            case 1:
                {
                    auto const &subscription = std::get<1>(input.timedData.value.key());
                    this->handleSubscription(id, subscription.key, versionProvider);
                }
                break;
            case 2:
                {
                    auto const &unsubscription = std::get<2>(input.timedData.value.key());
                    this->handleUnsubscription(id, unsubscription.originalSubscriptionID, unsubscription.key, versionProvider);
                }
                break;
            default:
                break;
            }
        }
    };

} } } } } 

#endif