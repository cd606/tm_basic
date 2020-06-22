#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_ASYNC_WATCHABLE_STORAGE_TRANSACTION_BROKER_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_ASYNC_WATCHABLE_STORAGE_TRANSACTION_BROKER_HPP_

#include <tm_kit/basic/transaction/SingleKeyAsyncWatchableTransactionHandlerComponent.hpp>
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
        , class DataDeltaType = DataType
        , class Cmp = std::less<VersionType>
        , class KeyHash = std::hash<KeyType>
    >
    class SingleKeyAsyncWatchableStorageTransactionBroker final
        :
        public virtual M::IExternalComponent
        , public M::template AbstractOnOrderFacility<
            typename SingleKeyTransactionInterface<KeyType,DataType,VersionType,typename M::EnvironmentType::IDType,DataSummaryType,DataDeltaType,Cmp>
                    ::FacilityInput
            , typename SingleKeyTransactionInterface<KeyType,DataType,VersionType,typename M::EnvironmentType::IDType,DataSummaryType,DataDeltaType,Cmp>
                    ::FacilityOutput
        >
        , public SingleKeyAsyncWatchableTransactionHandlerComponent<typename M::EnvironmentType::IDType,VersionType,KeyType,DataType,DataSummaryType,DataDeltaType>
                ::Callback
    {
    private:
        using IDType = typename M::EnvironmentType::IDType;
        using TI = SingleKeyTransactionInterface<KeyType,DataType,VersionType,IDType,DataSummaryType,DataDeltaType,Cmp>;
        using TH = SingleKeyAsyncWatchableTransactionHandlerComponent<IDType,VersionType,KeyType,DataType,DataSummaryType,DataDeltaType>;

        struct OneKeyInfo {
            std::unordered_set<IDType, typename M::EnvironmentType::IDHash> subscribers;
        };
        std::unordered_map<KeyType, OneKeyInfo, KeyHash> keyMap_;
        struct OneRequestInfo {
            std::string account;
        };
        std::unordered_map<IDType, OneRequestInfo, typename M::EnvironmentType::IDHash> reqInfoMap_;

        typename M::EnvironmentType *env_;

        std::mutex mutex_;

        void handleSubscription(typename M::EnvironmentType *env, TH *handler, std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::Subscription const &subscription) {
            {
                std::lock_guard<std::mutex> _(mutex_);
                
                auto const &key = subscription.key;
                auto iter = keyMap_.find(key);

                if (iter == keyMap_.end()) {
                    iter = keyMap_.insert(std::make_pair<KeyType,OneKeyInfo>(
                        KeyType(key)
                        , OneKeyInfo {}
                    )).first;
                }
                iter->second.subscribers.insert(requester);
                reqInfoMap_.insert(std::make_pair<IDType,OneRequestInfo>(
                    IDType(requester)
                    , OneRequestInfo {
                        account
                    }
                ));
            }

            this->publish(
                env
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::FacilityOutput { {typename TI::SubscriptionAck {}} }
                }
                , false);

            handler->startWatch(account, subscription.key, this);
        }
        void handleUnsubscription(typename M::EnvironmentType *env, TH *handler, std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::Unsubscription const &unsubscription) {
            bool found = false;
            std::optional<KeyType> stopWatchKey = std::nullopt;
            {
                std::lock_guard<std::mutex> _(mutex_);

                auto reqInfoIter = reqInfoMap_.find(unsubscription.originalSubscriptionID);
                if (reqInfoIter != reqInfoMap_.end() && reqInfoIter->second.account == account) {
                    reqInfoMap_.erase(reqInfoIter);
                    auto iter = keyMap_.find(unsubscription.key);
                    if (iter != keyMap_.end()) {
                        auto innerIter = iter->second.subscribers.find(unsubscription.originalSubscriptionID);
                        if (innerIter != iter->second.subscribers.end()) {
                            found = true;
                            iter->second.subscribers.erase(innerIter);
                            if (iter->second.subscribers.empty()) {
                                stopWatchKey = iter->first;
                                keyMap_.erase(iter);
                            }
                        }
                    }
                }
            }
            if (found) {
                this->publish(
                    env
                    , typename M::template Key<typename TI::FacilityOutput> {
                        unsubscription.originalSubscriptionID, typename TI::FacilityOutput { {typename TI::UnsubscriptionAck {}} }
                    }
                    , true
                );
            }
            this->publish(
                env
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::FacilityOutput { {typename TI::UnsubscriptionAck {}} }
                }
                , true
            );
            if (stopWatchKey) {
                handler->stopWatch(*stopWatchKey, this);
            }          
        }
    public:
        virtual void start(typename M::EnvironmentType *env) override final {
            env_ = env;
            auto *handler = static_cast<TH *>(env);
            handler->initialize();
        }
        virtual void handle(typename M::template InnerData<typename M::template Key<typename TI::FacilityInput>> &&input) override final {
            auto *env = input.environment;
            auto *handler = static_cast<TH *>(env);
            auto account = std::move(std::get<0>(input.timedData.value.key()));
            auto requester = std::move(input.timedData.value.id());
            
            std::visit([this,env,handler,&account,&requester](auto && arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<typename TI::Transaction, T>) {
                    auto transaction = std::move(arg);
                    std::visit([this,env,handler,&account,&requester](auto const &trans) {
                        using T1 = std::decay_t<decltype(trans)>;
                        
                        typename TI::TransactionResult res = typename TI::TransactionQueuedAsynchronously {};
                        this->publish(
                            env
                            , typename M::template Key<typename TI::FacilityOutput> {
                                requester, typename TI::FacilityOutput { {res} }
                            }
                            , false
                        );

                        if constexpr (std::is_same_v<typename TI::InsertAction, T1>) {
                            auto const &insertAction = trans;
                            handler->handleInsert(requester, account, insertAction.key, insertAction.data, insertAction.ignoreChecks, this);
                        } else if constexpr (std::is_same_v<typename TI::UpdateAction, T1>) {
                            auto const &updateAction = trans;
                            handler->handleUpdate(requester, account, updateAction.key, updateAction.oldVersion, updateAction.oldDataSummary, updateAction.dataDelta, updateAction.ignoreChecks, this);
                        } else if constexpr (std::is_same_v<typename TI::DeleteAction, T1>) {
                            auto const &deleteAction = trans;
                            handler->handleDelete(requester, account, deleteAction.key, deleteAction.oldVersion, deleteAction.oldDataSummary, deleteAction.ignoreChecks, this);
                        } else {
                            typename TI::TransactionResult res = typename TI::TransactionFailurePermission {};
                            this->publish(
                                env
                                , typename M::template Key<typename TI::FacilityOutput> {
                                    requester, typename TI::FacilityOutput { {res} }
                                }
                                , true
                            );
                        }
                    }, transaction);
                } else if constexpr (std::is_same_v<typename TI::Subscription, T>) {
                    auto subscription = std::move(arg);
                    this->handleSubscription(env, handler, account, requester, subscription);
                } else if constexpr (std::is_same_v<typename TI::Unsubscription, T>) {
                    auto unsubscription = std::move(arg);
                    this->handleUnsubscription(env, handler, account, requester, unsubscription);
                }
            }, std::move(std::get<1>(input.timedData.value.key()).value));
        }
        virtual void onRequestDecision(IDType const &id, RequestDecision decision) override final {
            typename TI::TransactionResult res {typename TI::TransactionFailurePermission {}};
            switch (decision) {
            case RequestDecision::Success:
                res = typename TI::TransactionSuccess {};
                break;
            case RequestDecision::FailurePrecondition:
                res = typename TI::TransactionFailurePrecondition {};
                break;
            case RequestDecision::FailureConsistency:
                res = typename TI::TransactionFailureConsistency {};
                break;
            case RequestDecision::FailurePermission:
            default:
                res = typename TI::TransactionFailurePermission {};
                break;
            }
            this->publish(
                env_
                , typename M::template Key<typename TI::FacilityOutput> {
                    id, typename TI::FacilityOutput { {res} }
                }
                , false
            );
        }
        virtual void onValueChange(KeyType const &key, VersionType &&version, std::optional<DataType> &&data) override final {
            std::vector<IDType> ids;
            {
                std::lock_guard<std::mutex> _(mutex_);
                auto iter = keyMap_.find(key);
                if (iter == keyMap_.end()) {
                    return;
                }
                if (iter->second.subscribers.empty()) {
                    return;
                }
                std::copy(iter->second.subscribers.begin(), iter->second.subscribers.end(), std::back_inserter(ids));
            }
           
            if (ids.size() == 1) {
                this->publish(
                    env_
                    , typename M::template Key<typename TI::FacilityOutput> {
                        ids[0], typename TI::FacilityOutput { {
                            typename TI::OneValue {
                                key, std::move(version), std::move(data)
                            }
                        } }
                    }
                    , false
                );
            } else {
                for (auto subscriberID : ids) {
                    this->publish(
                        env_
                        , typename M::template Key<typename TI::FacilityOutput> {
                            subscriberID, typename TI::FacilityOutput { {
                                typename TI::OneValue {
                                    key
                                    , infra::withtime_utils::makeValueCopy(version)
                                    , infra::withtime_utils::makeValueCopy(data)
                                }
                            } }
                        }
                        , false
                    );
                }
            }
        }
        virtual void onStopWatch(KeyType const &key) override final {
            //currently ignore
        }
    };

} } } } } 

#endif