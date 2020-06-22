#ifndef TM_KIT_BASIC_TRANSACTION_EXCLUSIVE_SINGLE_KEY_ASYNC_WATCHABLE_STORAGE_TRANSACTION_BROKER_HPP_
#define TM_KIT_BASIC_TRANSACTION_EXCLUSIVE_SINGLE_KEY_ASYNC_WATCHABLE_STORAGE_TRANSACTION_BROKER_HPP_

#include <tm_kit/basic/transaction/ExclusiveSingleKeyAsyncWatchableTransactionHandlerComponent.hpp>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <queue>
#include <array>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {
    
    template <
        class M
        , class KeyType
        , class DataType
        , class VersionType
        , class DataSummaryType = DataType
        , class CheckSummary = std::equal_to<DataType>
        , class DataDeltaType = DataType
        , class Cmp = std::less<VersionType>
        , class KeyHash = std::hash<KeyType>
    >
    class ExclusiveSingleKeyAsyncWatchableStorageTransactionBroker final
        :
        public virtual M::IExternalComponent
        , public M::template AbstractOnOrderFacility<
            typename SingleKeyTransactionInterface<KeyType,DataType,VersionType,typename M::EnvironmentType::IDType,DataSummaryType,DataDeltaType,Cmp>
                    ::FacilityInput
            , typename SingleKeyTransactionInterface<KeyType,DataType,VersionType,typename M::EnvironmentType::IDType,DataSummaryType,DataDeltaType,Cmp>
                    ::FacilityOutput
        >
        , public ExclusiveSingleKeyAsyncWatchableTransactionHandlerComponent<VersionType,KeyType,DataType,DataDeltaType,Cmp>
                ::Callback
    {
    private:
        using IDType = typename M::EnvironmentType::IDType;
        using TI = SingleKeyTransactionInterface<KeyType,DataType,VersionType,IDType,DataSummaryType,DataDeltaType,Cmp>;
        using TH = ExclusiveSingleKeyAsyncWatchableTransactionHandlerComponent<VersionType,KeyType,DataType,DataDeltaType,Cmp>;
            
        typename M::EnvironmentType *env_;
        TH *handler_;
        
        CheckSummary checkSummary_;
        Cmp versionCmp_;

        using VersionedData = infra::VersionedData<VersionType, std::optional<DataType>, Cmp>;
        struct OneKeyInfo {
            VersionedData data;
            std::unordered_set<IDType, typename M::EnvironmentType::IDHash> subscribers;
        };
        std::unordered_map<KeyType, OneKeyInfo, KeyHash> keyMap_;

        struct OneRequestInfo {
            std::string account;
        };
        std::unordered_map<IDType, OneRequestInfo, typename M::EnvironmentType::IDHash> reqInfoMap_;
        
        std::mutex dataMutex_;

        using TransactionQueue = std::deque<
            std::tuple<IDType, std::string, typename TI::Transaction>
        >;
        std::array<TransactionQueue, 2> transactionQueues_;
        size_t transactionQueueIncomingIndex_;

        std::thread transactionThread_;
        std::atomic<bool> running_;
        std::mutex transactionQueueMutex_;
        std::condition_variable transactionQueueCond_;

        void handleSubscription(std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::Subscription const &subscription) {
            bool needToStartWatch = false;
            VersionedData d;
            {
                std::lock_guard<std::mutex> _(dataMutex_);
                
                auto iter = keyMap_.find(subscription.key);
                if (iter == keyMap_.end()) {
                    iter = keyMap_.insert(std::make_pair<KeyType,OneKeyInfo>(
                        KeyType(subscription.key)
                        , OneKeyInfo {
                            {VersionType{}, std::nullopt}
                            , {}
                        }
                    )).first;
                    needToStartWatch = true;
                }
                iter->second.subscribers.insert(requester);
                d = iter->second.data;

                reqInfoMap_.insert(std::make_pair<IDType,OneRequestInfo>(
                    IDType(requester)
                    , OneRequestInfo {
                        account
                    }
                ));
            }

            this->publish(
                env_
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::FacilityOutput { {typename TI::SubscriptionAck {}} }
                }
                , false);
            this->publish(
                env_
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::FacilityOutput { {
                        typename TI::OneValue {
                            subscription.key, std::move(d.version), std::move(d.data)
                        }
                    } }
                }
                , false
            );

            if (needToStartWatch) {
                handler_->startWatchIfNecessary(account, subscription.key);
            }
        }
        void handleUnsubscription(std::string const &account, typename M::EnvironmentType::IDType const &requester, typename TI::Unsubscription const &unsubscription) {
            bool found = false;
            std::optional<KeyType> mayStopWatchKey = std::nullopt;
            {
                std::lock_guard<std::mutex> _(dataMutex_);

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
                                mayStopWatchKey = iter->first;
                                keyMap_.erase(iter);
                            }
                        }
                    }
                }
            }
            if (found) {
                this->publish(
                    env_
                    , typename M::template Key<typename TI::FacilityOutput> {
                        unsubscription.originalSubscriptionID, typename TI::FacilityOutput { {typename TI::UnsubscriptionAck {}} }
                    }
                    , true
                );
            }
            this->publish(
                env_
                , typename M::template Key<typename TI::FacilityOutput> {
                    requester, typename TI::FacilityOutput { {typename TI::UnsubscriptionAck {}} }
                }
                , true
            );
            if (mayStopWatchKey) {
                handler_->discretionallyStopWatch(account, *mayStopWatchKey);
            }          
        }
        void queueTransaction(IDType &&id, std::string &&account, typename TI::Transaction &&transaction) {
            if (running_) {
                {
                    std::lock_guard<std::mutex> _(transactionQueueMutex_);
                    transactionQueues_[transactionQueueIncomingIndex_].push_back({
                        std::move(id), std::move(account), std::move(transaction)
                    });
                }
                transactionQueueCond_.notify_one();
            }
        }
        RequestDecision tryInsert(std::string const &account, typename TI::InsertAction insertAction) {
            VersionedData d;
            {
                std::lock_guard<std::mutex> _(dataMutex_);
                auto iter = keyMap_.find(insertAction.key);
                if (iter != keyMap_.end()) {
                    return RequestDecision::FailurePrecondition;
                }
                if (iter->second.data.data) {
                    return RequestDecision::FailurePrecondition;
                }
            }
            return handler_->handleInsert(account, insertAction.key, insertAction.data, insertAction.ignoreChecks);
        }
        RequestDecision tryUpdate(std::string const &account, typename TI::UpdateAction updateAction) {
            infra::VersionedData<VersionType, DataType, Cmp> d;
            {
                std::lock_guard<std::mutex> _(dataMutex_);
                auto iter = keyMap_.find(updateAction.key);
                if (iter == keyMap_.end()) {
                    return RequestDecision::FailurePrecondition;
                }
                if (!iter->second.data.data) {
                    return RequestDecision::FailurePrecondition;
                }
                if (!updateAction.ignoreChecks) {
                    if (versionCmp_(updateAction.oldVersion, iter->second.data.version)
                        || versionCmp_(iter->second.data.version, updateAction.oldVersion)) {
                        return RequestDecision::FailurePrecondition;
                    }
                    if (!checkSummary_(*(iter->second.data.data), updateAction.oldDataSummary)) {
                        return RequestDecision::FailurePrecondition;
                    }
                }
                d = infra::VersionedData<VersionType, DataType, Cmp> {iter->second.data.version, *(iter->second.data.data)};
            }
            return handler_->handleUpdate(account, updateAction.key, d, updateAction.dataDelta, updateAction.ignoreChecks);
        }
        RequestDecision tryDelete(std::string const &account, typename TI::DeleteAction deleteAction) {
            infra::VersionedData<VersionType, DataType, Cmp> d;
            {
                std::lock_guard<std::mutex> _(dataMutex_);
                auto iter = keyMap_.find(deleteAction.key);
                if (deleteAction.ignoreChecks) {
                    if (iter == keyMap_.end()) {
                        return RequestDecision::Success;
                    }
                    if (!iter->second.data.data) {
                        return RequestDecision::Success;
                    }
                } else {
                    if (iter == keyMap_.end()) {
                        return RequestDecision::FailurePrecondition;
                    }
                    if (!iter->second.data.data) {
                        return RequestDecision::FailurePrecondition;
                    }
                    if (versionCmp_(deleteAction.oldVersion, iter->second.data.version)
                        || versionCmp_(iter->second.data.version, deleteAction.oldVersion)) {
                        return RequestDecision::FailurePrecondition;
                    }
                    if (!checkSummary_(*(iter->second.data.data), deleteAction.oldDataSummary)) {
                        return RequestDecision::FailurePrecondition;
                    }
                }
                d = infra::VersionedData<VersionType, DataType, Cmp> {iter->second.data.version, *(iter->second.data.data)};
            }
            return handler_->handleDelete(account, deleteAction.key, d, deleteAction.ignoreChecks);
        }
        void handleTransaction(IDType const &id, std::string const &account, typename TI::Transaction &&transaction) {
            std::visit([this,&id,&account](auto &&tr) {
                using T = std::decay_t<decltype(tr)>;
                if constexpr (std::is_same_v<T, typename TI::InsertAction>) {
                    auto insertAction = std::move(tr);
                    if (insertAction.ignoreChecks) {
                        basic::transaction::RequestDecision res;
                        while ((res = tryInsert(account, insertAction)) != basic::transaction::RequestDecision::Success) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                        onRequestDecision(id, res);
                    } else {
                        onRequestDecision(id, tryInsert(account, insertAction));
                    }
                } else if constexpr (std::is_same_v<T, typename TI::UpdateAction>) {
                    auto updateAction = std::move(tr);
                    if (updateAction.ignoreChecks) {
                        basic::transaction::RequestDecision res;
                        while ((res = tryUpdate(account, updateAction)) != basic::transaction::RequestDecision::Success) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                        onRequestDecision(id, res);
                    } else {
                        onRequestDecision(id, tryUpdate(account, updateAction));
                    }
                } else if constexpr (std::is_same_v<T, typename TI::DeleteAction>) {  
                    auto deleteAction = std::move(tr); 
                    if (deleteAction.ignoreChecks) {
                        basic::transaction::RequestDecision res;
                        while ((res = tryDelete(account, deleteAction)) != basic::transaction::RequestDecision::Success) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                        onRequestDecision(id, res);
                    } else {
                        onRequestDecision(id, tryDelete(account, deleteAction));
                    }
                } else {
                    onRequestDecision(id, RequestDecision::FailurePermission);
                }
            }, std::move(transaction));
        }
        void runTransactionThread() {
            while (running_) {
                std::unique_lock<std::mutex> lock(transactionQueueMutex_);
                transactionQueueCond_.wait_for(lock, std::chrono::milliseconds(1));
                if (!running_) {
                    lock.unlock();
                    break;
                }
                if (transactionQueues_[transactionQueueIncomingIndex_].empty()) {
                    lock.unlock();
                    continue;
                }
                int processingIndex = transactionQueueIncomingIndex_;
                transactionQueueIncomingIndex_ = 1-transactionQueueIncomingIndex_;
                lock.unlock();

                {
                    while (!transactionQueues_[processingIndex].empty()) {
                        auto &t = transactionQueues_[processingIndex].front();
                        handleTransaction(
                            std::get<0>(t)
                            , std::get<1>(t)
                            , std::move(std::get<2>(t))
                        );
                        transactionQueues_[processingIndex].pop_front();
                    }
                }
            }
        }
        void onRequestDecision(IDType const &id, RequestDecision decision) {
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
    public:
        ExclusiveSingleKeyAsyncWatchableStorageTransactionBroker()
            :
            env_(nullptr), handler_(nullptr)
            , checkSummary_(), versionCmp_()
            , keyMap_(), reqInfoMap_(), dataMutex_()
            , transactionQueues_(), transactionQueueIncomingIndex_(0)
            , transactionThread_(), running_(false)
            , transactionQueueMutex_(), transactionQueueCond_()
        {}
        virtual ~ExclusiveSingleKeyAsyncWatchableStorageTransactionBroker() {
            if (running_) {
                running_ = false;
                transactionThread_.join();
            }
        }
        virtual void start(typename M::EnvironmentType *env) override final {
            env_ = env;
            handler_ = static_cast<TH *>(env);
            if (!handler_->exclusivelyBindTo(this)) {
                throw std::runtime_error("exclusive single key async watchable storage transaction broker binding failure");
            }

            running_ = true;
            transactionThread_ = std::thread(&ExclusiveSingleKeyAsyncWatchableStorageTransactionBroker::runTransactionThread, this);
            transactionThread_.detach();

            handler_->setUpInitialWatching();
        }
        virtual void handle(typename M::template InnerData<typename M::template Key<typename TI::FacilityInput>> &&input) override final {
            auto account = std::get<0>(input.timedData.value.key());
            auto requester = input.timedData.value.id();
            
            std::visit([this,&account,&requester](auto && arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<typename TI::Transaction, T>) {
                    typename TI::TransactionResult res = typename TI::TransactionQueuedAsynchronously {};
                    this->publish(
                        env_
                        , typename M::template Key<typename TI::FacilityOutput> {
                            requester, typename TI::FacilityOutput { {res} }
                        }
                        , false
                    );
                    queueTransaction(std::move(requester), std::move(account), std::move(arg));
                } else if constexpr (std::is_same_v<typename TI::Subscription, T>) {
                    auto subscription = std::move(arg);
                    this->handleSubscription(account, requester, subscription);
                } else if constexpr (std::is_same_v<typename TI::Unsubscription, T>) {
                    auto unsubscription = std::move(arg);
                    this->handleUnsubscription(account, requester, unsubscription);
                }
            }, std::move(std::get<1>(input.timedData.value.key()).value));
        }
        virtual void onValueChange(KeyType const &key, VersionType &&version, std::optional<DataType> &&data) override final {
            std::vector<IDType> ids;
            VersionedData newData;
            {
                std::lock_guard<std::mutex> _(dataMutex_);
                auto iter = keyMap_.find(key);
                if (iter == keyMap_.end()) {
                    iter = keyMap_.insert(std::make_pair<KeyType,OneKeyInfo>(
                        KeyType(key)
                        , OneKeyInfo {
                            VersionedData {
                                std::move(version), std::move(data)
                            }
                            , {}
                        }
                    )).first;
                } else {
                    infra::withtime_utils::updateVersionedData(
                        iter->second.data
                        , VersionedData {
                            std::move(version), std::move(data)
                        }
                    );
                }
                if (iter->second.subscribers.empty()) {
                    return;
                }
                newData = iter->second.data;
                std::copy(iter->second.subscribers.begin(), iter->second.subscribers.end(), std::back_inserter(ids));
            }
           
            if (ids.size() == 1) {
                this->publish(
                    env_
                    , typename M::template Key<typename TI::FacilityOutput> {
                        ids[0], typename TI::FacilityOutput { {
                            typename TI::OneValue {
                                key, std::move(newData.version), std::move(newData.data)
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
                                    , infra::withtime_utils::makeValueCopy(newData.version)
                                    , infra::withtime_utils::makeValueCopy(newData.data)
                                }
                            } }
                        }
                        , false
                    );
                }
            }
        }
    };

} } } } } 

#endif