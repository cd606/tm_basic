#ifndef TM_KIT_BASIC_TRANSACTION_V2_GENERAL_SUBSCRIBER_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_GENERAL_SUBSCRIBER_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <functional>

#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/SerializationHelperMacros.hpp>

#include <queue>
#include <array>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    #define SubscriptionFields \
        ((std::vector<KeyType>, keys))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, KeyType)), Subscription, SubscriptionFields);
    
    #define UnsubscriptionFields \
        ((IDType, originalSubscriptionID))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, IDType)), Unsubscription, UnsubscriptionFields);
    
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT(ListSubscriptions);

    #ifdef _MSC_VER
        #define SubscriptionInfoFields \
            ((TM_BASIC_CBOR_CAPABLE_STRUCT_PROTECT_TYPE(std::vector<std::tuple<IDType,std::vector<KeyType>>>), subscriptions))
    #else
        #define SubscriptionInfoFields \
            (((std::vector<std::tuple<IDType,std::vector<KeyType>>>), subscriptions))
    #endif
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, IDType)) ((typename, KeyType)), SubscriptionInfo, SubscriptionInfoFields);
    
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT(UnsubscribeAll);

    #define SnapshotRequestFields \
        ((std::vector<KeyType>, keys))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, KeyType)), SnapshotRequest, SnapshotRequestFields);

    //subscription and unsubscription ack's just use the exact same data types

    template <class IDType, class DI>
    struct GeneralSubscriberTypes {
        using Key = typename DI::Key;
        using ID = IDType;

        using Subscription = transaction::v2::Subscription<Key>;
        using Unsubscription = transaction::v2::Unsubscription<ID>;
        using SubscriptionUpdate = typename DI::Update;

        using ListSubscriptions = transaction::v2::ListSubscriptions;
        using SubscriptionInfo = transaction::v2::SubscriptionInfo<ID,Key>;
        using UnsubscribeAll = transaction::v2::UnsubscribeAll;

        using SnapshotRequest = transaction::v2::SnapshotRequest<Key>;

        using Input = SingleLayerWrapper<
            std::variant<Subscription, Unsubscription, ListSubscriptions, UnsubscribeAll, SnapshotRequest>
        >;
        using Output = SingleLayerWrapper<
            std::variant<Subscription, Unsubscription, SubscriptionUpdate, SubscriptionInfo, UnsubscribeAll>
        >;
        using InputWithAccountInfo = std::tuple<std::string, Input>;
    };

    template <class M, class DI>
    using IGeneralSubscriber = typename M::template AbstractIntegratedLocalOnOrderFacility<
        typename GeneralSubscriberTypes<typename M::EnvironmentType::IDType, DI>::InputWithAccountInfo
        , typename GeneralSubscriberTypes<typename M::EnvironmentType::IDType, DI>::Output
        , typename GeneralSubscriberTypes<typename M::EnvironmentType::IDType, DI>::SubscriptionUpdate
    >;

    template <class M, class DI, class KeyHash = std::hash<typename DI::Key>, bool VerboseLogging=true>
    class GeneralSubscriber 
        : public IGeneralSubscriber<M,DI>
    {       
    private:
        using Types = GeneralSubscriberTypes<typename M::EnvironmentType::IDType, DI>;
        
        using SubscriptionMap = std::unordered_map<typename Types::Key, std::vector<typename Types::ID>, KeyHash>;
        SubscriptionMap subscriptionMap_;
        
        struct IDInfo {
            std::vector<typename Types::Key> keys;
            std::string account;
        };
        using IDInfoMap = std::unordered_map<typename Types::ID, IDInfo, typename M::EnvironmentType::IDHash>;
        IDInfoMap idInfoMap_;

        struct AccountInfo {
            std::unordered_set<typename Types::ID, typename M::EnvironmentType::IDHash> ids;
        };
        using AccountInfoMap = std::unordered_map<std::string, AccountInfo>;
        AccountInfoMap accountInfoMap_;

        using DataStorePtr = transaction::v2::TransactionDataStoreConstPtr<
            DI, KeyHash, M::PossiblyMultiThreaded
        >;
        using DataStore = std::decay_t<typename DataStorePtr::element_type>;
        using Mutex = typename DataStore::Mutex;

        Mutex mutex_;

        DataStorePtr dataStorePtr_;

        struct ThreadDataImpl {
            using QueueItem = std::variant<
                std::tuple<typename M::EnvironmentType::IDType, typename Types::InputWithAccountInfo>
                , typename Types::SubscriptionUpdate
            >;
            using InputQueue = std::deque<
                std::tuple<typename M::EnvironmentType *, QueueItem>
            >;
            std::array<InputQueue, 2> inputQueues_;
            size_t inputQueueIncomingIndex_ = 0;

            std::thread workThread_;
            std::atomic<bool> running_ = false;
            std::mutex inputQueueMutex_;
            std::condition_variable inputQueueCond_;
        };
        using ThreadData = std::conditional_t<
            M::PossiblyMultiThreaded
            , ThreadDataImpl
            , bool
        >;
        ThreadData threadData_;

        std::vector<typename Types::Key> addSubscription(typename M::EnvironmentType *env, std::string const &account, typename Types::ID id, typename Types::Subscription const &subscription) {
            std::vector<typename Types::Key> ret;
            for (auto const &key : subscription.keys) {
                auto iter = subscriptionMap_.find(key);
                if (iter == subscriptionMap_.end()) {
                    ret.push_back(key);
                    subscriptionMap_.insert(std::make_pair(
                        key
                        , std::vector<typename Types::ID> {id}
                    ));
                } else {
                    if (std::find(iter->second.begin(), iter->second.end(), id) == iter->second.end()) {
                        iter->second.push_back(id);
                        ret.push_back(key);
                    }
                }
            }
            idInfoMap_.insert(std::make_pair(id, IDInfo {ret, account}));
            accountInfoMap_[account].ids.insert(id);
            return ret;
        }
        bool removeSubscription(typename M::EnvironmentType *env, std::string const &account, typename Types::Unsubscription const &unsubscription) {
            auto idInfoIter = idInfoMap_.find(unsubscription.originalSubscriptionID);
            if (idInfoIter == idInfoMap_.end()) {
                return false;
            }
            if (idInfoIter->second.account != account) {
                return false;
            }
            for (auto const &key : idInfoIter->second.keys) {
                auto iter = subscriptionMap_.find(key);
                if (iter != subscriptionMap_.end()) {
                    auto vecIter = std::find(iter->second.begin(), iter->second.end(), unsubscription.originalSubscriptionID);
                    if (vecIter != iter->second.end()) {
                        iter->second.erase(vecIter);
                        if (iter->second.empty()) {
                            subscriptionMap_.erase(iter);
                        }
                    }
                }
            }
            idInfoMap_.erase(idInfoIter);
            auto acctIter = accountInfoMap_.find(account);
            if (acctIter != accountInfoMap_.end()) {
                acctIter->second.ids.erase(unsubscription.originalSubscriptionID);
                if (acctIter->second.ids.empty()) {
                    accountInfoMap_.erase(acctIter);
                }
            }
            return true;
        }
        std::vector<typename M::EnvironmentType::IDType> removeAllSubscriptions(typename M::EnvironmentType *env, std::string const &account) {
            auto acctIter = accountInfoMap_.find(account);
            if (acctIter == accountInfoMap_.end()) {
                return std::vector<typename M::EnvironmentType::IDType> {};
            }
            std::vector<typename M::EnvironmentType::IDType> ret;
            for (auto const &id : acctIter->second.ids) {
                auto idInfoIter = idInfoMap_.find(id);
                if (idInfoIter == idInfoMap_.end()) {
                    continue;
                }
                if (idInfoIter->second.account != account) {
                    continue;
                }
                for (auto const &key : idInfoIter->second.keys) {
                    auto iter = subscriptionMap_.find(key);
                    if (iter != subscriptionMap_.end()) {
                        auto vecIter = std::find(iter->second.begin(), iter->second.end(), id);
                        if (vecIter != iter->second.end()) {
                            iter->second.erase(vecIter);
                            if (iter->second.empty()) {
                                subscriptionMap_.erase(iter);
                            }
                        }
                    }
                }
                idInfoMap_.erase(idInfoIter);
                ret.push_back(id);
            }
            accountInfoMap_.erase(acctIter);
            return ret;
        }
        void reallyHandleUserInput(typename M::EnvironmentType *env, typename M::EnvironmentType::IDType &&inputID, typename Types::InputWithAccountInfo &&input) {
            std::string account = std::move(std::get<0>(input));
            typename Types::ID id = std::move(inputID);
            if constexpr (VerboseLogging) {
                std::ostringstream oss;
                oss << "[GeneralSubscriber::reallyHandleUserInput] Got input from '" << account << "', id is '" << env->id_to_string(id) << "', content is ";
                PrintHelper<typename Types::Input>::print(oss, std::get<1>(input));
                env->log(infra::LogLevel::Info, oss.str());
            }
            std::visit([this,env,id,&account](auto &&d) {
                using T = std::decay_t<decltype(d)>;
                if constexpr (std::is_same_v<T, typename Types::Subscription>) {
                    typename Types::Subscription subscription {std::move(d)};
                    std::vector<typename Types::Key> newKeys;
                    {
                        typename DataStore::Lock _(mutex_);
                        newKeys = addSubscription(env, account, id, subscription); 
                    }
                    this->publish(
                        env
                        , typename M::template Key<typename Types::Output> {
                            id
                            , typename Types::Output {
                                { typename Types::Subscription { newKeys } }
                            }
                        }
                        , false
                    );
                    this->publish(
                        env
                        , typename M::template Key<typename Types::Output> {
                            id
                            , typename Types::Output {
                                { dataStorePtr_->createFullUpdateNotification(newKeys) }
                            }
                        }
                        , false
                    );
                } else if constexpr (std::is_same_v<T, typename Types::Unsubscription>) {
                    typename Types::Unsubscription unsubscription {std::move(d)};
                    bool success = false;
                    {
                        typename DataStore::Lock _(mutex_);
                        success = removeSubscription(env, account, unsubscription);
                    }
                    if (success) {
                        this->publish(
                            env
                            , typename M::template Key<typename Types::Output> {
                                unsubscription.originalSubscriptionID
                                , typename Types::Output {
                                    { typename Types::Unsubscription { unsubscription.originalSubscriptionID } }
                                }
                            }
                            , true
                        );
                    }
                    this->publish(
                        env
                        , typename M::template Key<typename Types::Output> {
                            id
                            , typename Types::Output {
                                { typename Types::Unsubscription { unsubscription.originalSubscriptionID } }
                            }
                        }
                        , true
                    );
                } else if constexpr (std::is_same_v<T, typename Types::ListSubscriptions>) {
                    typename Types::SubscriptionInfo info;
                    {
                        typename DataStore::Lock _(mutex_);
                        auto acctIter = accountInfoMap_.find(account);
                        if (acctIter != accountInfoMap_.end()) {
                            for (auto const &existingID : acctIter->second.ids) {
                                auto idIter = idInfoMap_.find(existingID);
                                if (idIter != idInfoMap_.end()) {
                                    info.subscriptions.push_back({
                                        existingID
                                        , idIter->second.keys
                                    });
                                }
                            }
                        }
                    }
                    this->publish(
                        env
                        , typename M::template Key<typename Types::Output> {
                            id
                            , typename Types::Output {
                                { std::move(info) }
                            }
                        }
                        , true
                    );
                } else if constexpr (std::is_same_v<T, typename Types::UnsubscribeAll>) {
                    std::vector<typename M::EnvironmentType::IDType> removedIDs;
                    {
                        typename DataStore::Lock _(mutex_);
                        removedIDs = removeAllSubscriptions(env, account);
                    }
                    for (auto const &removedID : removedIDs) {
                        this->publish(
                            env
                            , typename M::template Key<typename Types::Output> {
                                removedID
                                , typename Types::Output {
                                    { typename Types::Unsubscription { removedID } }
                                }
                            }
                            , true
                        );
                    }
                    this->publish(
                        env
                        , typename M::template Key<typename Types::Output> {
                            id
                            , typename Types::Output {
                                { typename Types::UnsubscribeAll {} }
                            }
                        }
                        , true
                    );
                } else if constexpr (std::is_same_v<T, typename Types::SnapshotRequest>) {
                    this->publish(
                        env
                        , typename M::template Key<typename Types::Output> {
                            id
                            , typename Types::Output {
                                { dataStorePtr_->createFullUpdateNotification(d.keys) }
                            }
                        }
                        , true
                    );
                }
            }, std::move(std::get<1>(input).value));
        }
        void reallyHandleDataUpdate(typename M::EnvironmentType *env, typename Types::SubscriptionUpdate &&update) {
            auto globalVersion = update.version;
            std::unordered_map<typename Types::ID, std::vector<typename DI::OneUpdateItem>, typename M::EnvironmentType::IDHash> updateMap;
            for (auto &x : update.data) {
                typename DI::OneUpdateItem item = std::move(x);
                std::vector<typename Types::ID> *affected = nullptr;
                std::visit([this,env,&affected](auto const &u) {
                    using T = std::decay_t<decltype(u)>;
                    typename Types::Key k;
                    if constexpr (std::is_same_v<T, typename DI::OneFullUpdateItem>) {
                        k = u.groupID;
                    } else if constexpr (std::is_same_v<T, typename DI::OneDeltaUpdateItem>) {
                        k = std::get<0>(u);
                    }
                    typename DataStorePtr::element_type::Lock _(mutex_);
                    auto iter = subscriptionMap_.find(k);
                    if (iter != subscriptionMap_.end()) {
                        affected = &(iter->second);
                    }
                }, item.value);
                if (affected == nullptr || affected->empty()) {
                    continue;
                } else if (affected->size() == 1) {
                    updateMap[(*affected)[0]].push_back(std::move(item));
                } else {
                    for (auto const &id : *affected) {
                        updateMap[id].push_back(infra::withtime_utils::makeValueCopy(item));
                    }
                }
            }
            for (auto &item : updateMap) {
                this->publish(
                    env
                    , typename M::template Key<typename Types::Output> {
                        item.first
                        , typename Types::Output {
                            typename Types::SubscriptionUpdate { globalVersion, std::move(item.second) }
                        }
                    }
                    , false
                );
            }
        }
        void runWorkThread() {
            if constexpr (M::PossiblyMultiThreaded) {
                while (threadData_.running_) {
                    std::unique_lock<std::mutex> lock(threadData_.inputQueueMutex_);
                    threadData_.inputQueueCond_.wait_for(lock, std::chrono::milliseconds(1));
                    if (!threadData_.running_) {
                        lock.unlock();
                        break;
                    }
                    if (threadData_.inputQueues_[threadData_.inputQueueIncomingIndex_].empty()) {
                        lock.unlock();
                        continue;
                    }
                    int processingIndex = threadData_.inputQueueIncomingIndex_;
                    threadData_.inputQueueIncomingIndex_ = 1-threadData_.inputQueueIncomingIndex_;
                    lock.unlock();

                    {
                        while (!threadData_.inputQueues_[processingIndex].empty()) {
                            auto &t = threadData_.inputQueues_[processingIndex].front();
                            auto *env = std::get<0>(t);
                            std::visit([this,env](auto &&item) {
                                using T = std::decay_t<decltype(item)>;
                                if constexpr (std::is_same_v<T, std::tuple<typename M::EnvironmentType::IDType, typename Types::InputWithAccountInfo>>) {
                                    reallyHandleUserInput(env, std::move(std::get<0>(item)), std::move(std::get<1>(item)));
                                } else if constexpr (std::is_same_v<T, typename Types::SubscriptionUpdate>) {
                                    reallyHandleDataUpdate(env, std::move(item));
                                }
                            }, std::move(std::get<1>(t)));
                            threadData_.inputQueues_[processingIndex].pop_front();
                        }
                    }
                }
            }            
        }
    public:
        using Input = typename Types::Input;
        using Output = typename Types::Output;
        using Key = typename Types::Key;
        using Subscription = typename Types::Subscription;
        using Unsubscription = typename Types::Unsubscription;
        using SubscriptionUpdate = typename Types::SubscriptionUpdate;
        using ListSubscriptions = typename Types::ListSubscriptions;
        using SubscriptionInfo = typename Types::SubscriptionInfo;
        using InputWithAccountInfo = typename Types::InputWithAccountInfo;

        GeneralSubscriber(DataStorePtr const &dataStorePtr) 
            : subscriptionMap_(), idInfoMap_(), accountInfoMap_(), mutex_(), dataStorePtr_(dataStorePtr), threadData_() {}
        virtual ~GeneralSubscriber() {
            if constexpr (M::PossiblyMultiThreaded) {
                if (threadData_.running_) {
                    threadData_.running_ = false;
                    threadData_.workThread_.join();
                }
            }
        }       
        
        void start(typename M::EnvironmentType *) override final {
            if constexpr (M::PossiblyMultiThreaded) {
                threadData_.running_ = true;
                threadData_.workThread_ = std::thread(&GeneralSubscriber::runWorkThread, this);
                threadData_.workThread_.detach();
            }
        }
        void handle(typename M::template InnerData<typename M::template Key<typename Types::InputWithAccountInfo>> &&input) override final {
            auto *env = input.environment;
            if constexpr (M::PossiblyMultiThreaded) {
                if (threadData_.running_) {
                    {
                        std::lock_guard<std::mutex> _(threadData_.inputQueueMutex_);
                        threadData_.inputQueues_[threadData_.inputQueueIncomingIndex_].push_back({
                            env
                            , std::tuple<typename M::EnvironmentType::IDType, typename Types::InputWithAccountInfo> {
                                input.timedData.value.id()
                                , input.timedData.value.key()
                            }
                        });
                    }
                    threadData_.inputQueueCond_.notify_one();
                }
            } else {
                reallyHandleUserInput(env
                    , input.timedData.value.id()
                    , input.timedData.value.key()
                );
            }
        }
        void handle(typename M::template InnerData<typename Types::SubscriptionUpdate> &&update) override final {
            auto *env = update.environment;
            //std::chrono::steady_clock::time_point t1, t2, t3, t4;
            if constexpr (M::PossiblyMultiThreaded) {
                //t1 = std::chrono::steady_clock::now();
                if (threadData_.running_) {
                    {
                        std::lock_guard<std::mutex> _(threadData_.inputQueueMutex_);
                        //t2 = std::chrono::steady_clock::now();
                        threadData_.inputQueues_[threadData_.inputQueueIncomingIndex_].push_back({
                            env
                            , std::move(update.timedData.value)
                        });
                        //t3 = std::chrono::steady_clock::now();
                    }
                    threadData_.inputQueueCond_.notify_one();
                }
                /*
                t4 = std::chrono::steady_clock::now();
                env->log(infra::LogLevel::Info
                    , std::string("From entrance to obtain lock: ")
                        +std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count())
                        +std::string(", push queue: ")
                        +std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(t3-t2).count())
                        +std::string(", until end: ")
                        +std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count())
                );*/
            } else {
                reallyHandleDataUpdate(env, std::move(update.timedData.value));
            }
        }
    };

} } 
    
} } } }

TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, KeyType)), dev::cd606::tm::basic::transaction::v2::Subscription, SubscriptionFields);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, IDType)), dev::cd606::tm::basic::transaction::v2::Unsubscription, UnsubscriptionFields);
TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_SERIALIZE(dev::cd606::tm::basic::transaction::v2::ListSubscriptions);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, IDType)) ((typename, KeyType)), dev::cd606::tm::basic::transaction::v2::SubscriptionInfo, SubscriptionInfoFields);
TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_SERIALIZE(dev::cd606::tm::basic::transaction::v2::UnsubscribeAll);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, KeyType)), dev::cd606::tm::basic::transaction::v2::SnapshotRequest, SnapshotRequestFields);

#endif
