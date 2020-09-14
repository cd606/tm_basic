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

#include <queue>
#include <array>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    template <class KeyType>
    struct Subscription {
        std::vector<KeyType> keys;
        void SerializeToString(std::string *s) const {
            *s = bytedata_utils::RunSerializer<std::vector<KeyType> const *>::apply(&keys);
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<std::vector<KeyType>>::apply(s);
            if (!res) {
                return false;
            }
            keys = std::move(*res);
            return true;
        }
    };
    template <class IDType>
    struct Unsubscription {
        IDType originalSubscriptionID;
        void SerializeToString(std::string *s) const {
            std::tuple<IDType const *> t {&originalSubscriptionID};
            *s = bytedata_utils::RunSerializer<std::tuple<IDType const *>>::apply(t);
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<std::tuple<IDType>>::apply(s);
            if (!res) {
                return false;
            }
            originalSubscriptionID = std::move(std::get<0>(*res));
            return true;
        }
    };
    struct ListSubscriptions {
        void SerializeToString(std::string *s) const {
            *s = bytedata_utils::RunSerializer<VoidStruct>::apply(VoidStruct {});
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<VoidStruct>::apply(s);
            if (!res) {
                return false;
            }
            return true;
        }
    };
    template <class IDType, class KeyType>
    struct SubscriptionInfo {
        using Subscriptions = std::vector<std::tuple<IDType,std::vector<KeyType>>>;
        Subscriptions subscriptions;
        void SerializeToString(std::string *s) const {
            std::tuple<Subscriptions const *> t {&subscriptions};
            *s = bytedata_utils::RunSerializer<std::tuple<Subscriptions const *>>::apply(t);
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<std::tuple<Subscriptions>>::apply(s);
            if (!res) {
                return false;
            }
            subscriptions = std::move(std::get<0>(*res));
            return true;
        }
    };
    struct UnsubscribeAll {
        void SerializeToString(std::string *s) const {
            *s = bytedata_utils::RunSerializer<VoidStruct>::apply(VoidStruct {});
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<VoidStruct>::apply(s);
            if (!res) {
                return false;
            }
            return true;
        }
    };
    template <class KeyType>
    struct SnapshotRequest {
        std::vector<KeyType> keys;
        void SerializeToString(std::string *s) const {
            *s = bytedata_utils::RunSerializer<std::vector<KeyType> const *>::apply(&keys);
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<std::vector<KeyType>>::apply(s);
            if (!res) {
                return false;
            }
            keys = std::move(*res);
            return true;
        }
    };

    template <class KeyType>
    inline std::ostream &operator<<(std::ostream &os, Subscription<KeyType> const &x) {
        os << "Subscription{keys=[";
        for (size_t ii = 0; ii < x.keys.size(); ++ii) {
            if (ii > 0) {
                os << ',';
            }
            os << x.keys[ii];
        }
        os << "]}";
        return os;
    }
    template <class IDType, class KeyType>
    inline std::ostream &operator<<(std::ostream &os, Unsubscription<IDType> const &x) {
        os << "Unsubscription{id=" << x.id << "}";
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, ListSubscriptions const &x) {
        os << "ListSubscriptions{}";
        return os;
    }
    template <class IDType, class KeyType>
    inline std::ostream &operator<<(std::ostream &os, SubscriptionInfo<IDType,KeyType> const &x) {
        os << "SubscriptionInfo{subscriptions=[";
        size_t ii = 0;
        for (auto const &item : x.subscriptions) {
            if (ii > 0) {
                os << ',';
            }
            ++ii;
            os << "{id=" << std::get<0>(item);
            os << ",keys=[";
            size_t jj = 0;
            for (auto const &key : std::get<1>(item)) {
                if (jj > 0) {
                    os << ',';
                }
                ++jj;
                os << key;
            }
            os << "]}";
        }
        os << "]}";
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, UnsubscribeAll const &x) {
        os << "UnsubscribeAll{}";
        return os;
    }
    template <class KeyType>
    inline std::ostream &operator<<(std::ostream &os, SnapshotRequest<KeyType> const &x) {
        os << "SnapshotRequest{keys=[";
        for (size_t ii = 0; ii < x.keys.size(); ++ii) {
            if (ii > 0) {
                os << ',';
            }
            os << x.keys[ii];
        }
        os << "]}";
        return os;
    }
    template <class KeyType>
    inline bool operator==(Subscription<KeyType> const &x, Subscription<KeyType> const &y) {
        return (x.keys == y.keys);
    }
    template <class IDType>
    inline bool operator==(Unsubscription<IDType> const &x, Unsubscription<IDType> const &y) {
        return (x.id == y.id);
    }
    inline bool operator==(ListSubscriptions const &, ListSubscriptions const &) {
        return true;
    }
    template <class IDType, class KeyType>
    inline bool operator==(SubscriptionInfo<IDType,KeyType> const &x, SubscriptionInfo<IDType,KeyType> const &y) {
        return (x.subscriptions == y.subscriptions);
    }
    inline bool operator==(UnsubscribeAll const &, UnsubscribeAll const &) {
        return true;
    }
    template <class KeyType>
    inline bool operator==(SnapshotRequest<KeyType> const &x, SnapshotRequest<KeyType> const &y) {
        return (x.keys == y.keys);
    }

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

        using Input = CBOR<
            std::variant<Subscription, Unsubscription, ListSubscriptions, UnsubscribeAll, SnapshotRequest>
        >;
        using Output = CBOR<
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

    template <class M, class DI, class KeyHash = std::hash<typename DI::Key>>
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
                }, item);
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
    
namespace bytedata_utils {
    template <class KeyType>
    struct RunCBORSerializer<transaction::v2::Subscription<KeyType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::Subscription<KeyType> const &x) {
            std::tuple<std::vector<KeyType> const *> t {&x.keys};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<std::vector<KeyType> const *>, 1>
                ::apply(t, {
                    "keys"
                });
        }
        static std::size_t apply(transaction::v2::Subscription<KeyType> const &x, char *output) {
            std::tuple<std::vector<KeyType> const *> t {&x.keys};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<std::vector<KeyType> const *>, 1>
                ::apply(t, {
                    "keys"
                }, output);
        }
        static std::size_t calculateSize(transaction::v2::Subscription<KeyType> const &x) {
            std::tuple<std::vector<KeyType> const *> t {&x.keys};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<std::vector<KeyType> const *>, 1>
                ::calculateSize(t, {
                    "keys"
                });
        }
    };
    template <class KeyType>
    struct RunCBORDeserializer<transaction::v2::Subscription<KeyType>, void> {
        static std::optional<std::tuple<transaction::v2::Subscription<KeyType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<std::vector<KeyType>>, 1>
                ::apply(data, start, {
                    "keys"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::Subscription<KeyType>,size_t> {
                transaction::v2::Subscription<KeyType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class IDType>
    struct RunCBORSerializer<transaction::v2::Unsubscription<IDType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::Unsubscription<IDType> const &x) {
            std::tuple<IDType const *> t {&x.originalSubscriptionID};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<IDType const *>, 1>
                ::apply(t, {
                    "original_subscription_id"
                });
        }
        static std::size_t apply(transaction::v2::Unsubscription<IDType> const &x, char *output) {
            std::tuple<IDType const *> t {&x.originalSubscriptionID};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<IDType const *>, 1>
                ::apply(t, {
                    "original_subscription_id"
                }, output);
        }
        static std::size_t calculateSize(transaction::v2::Unsubscription<IDType> const &x) {
            std::tuple<IDType const *> t {&x.originalSubscriptionID};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<IDType const *>, 1>
                ::calculateSize(t, {
                    "original_subscription_id"
                });
        }
    };
    template <class IDType>
    struct RunCBORDeserializer<transaction::v2::Unsubscription<IDType>, void> {
        static std::optional<std::tuple<transaction::v2::Unsubscription<IDType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<IDType>, 1>
                ::apply(data, start, {
                    "original_subscription_id"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::Unsubscription<IDType>,size_t> {
                transaction::v2::Unsubscription<IDType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <>
    struct RunCBORSerializer<transaction::v2::ListSubscriptions, void> {
        static std::vector<uint8_t> apply(transaction::v2::ListSubscriptions const &x) {
            return bytedata_utils::RunCBORSerializer<VoidStruct>
                ::apply(VoidStruct {});
        }
        static std::size_t apply(transaction::v2::ListSubscriptions const &x, char *output) {
            return bytedata_utils::RunCBORSerializer<VoidStruct>
                ::apply(VoidStruct {}, output);
        }
        static std::size_t calculateSize(transaction::v2::ListSubscriptions const &x) {
            return bytedata_utils::RunCBORSerializer<VoidStruct>
                ::calculateSize(VoidStruct {});
        }
    };
    template <>
    struct RunCBORDeserializer<transaction::v2::ListSubscriptions, void> {
        static std::optional<std::tuple<transaction::v2::ListSubscriptions,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializer<VoidStruct>
                ::apply(data, start);
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::ListSubscriptions,size_t> {
                transaction::v2::ListSubscriptions {}
                , std::get<1>(*t)
            };
        }
    };
    template <class IDType, class KeyType>
    struct RunCBORSerializer<transaction::v2::SubscriptionInfo<IDType,KeyType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::SubscriptionInfo<IDType,KeyType> const &x) {
            std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions const *> t {&x.subscriptions};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions const *>, 1>
                ::apply(t, {
                    "subscriptions"
                });
        }
        static std::size_t apply(transaction::v2::SubscriptionInfo<IDType,KeyType> const &x, char *output) {
            std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions const *> t {&x.subscriptions};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions const *>, 1>
                ::apply(t, {
                    "subscriptions"
                }, output);
        }
        static std::size_t calculateSize(transaction::v2::SubscriptionInfo<IDType,KeyType> const &x) {
            std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions const *> t {&x.subscriptions};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions const *>, 1>
                ::calculateSize(t, {
                    "subscriptions"
                });
        }
    };
    template <class IDType, class KeyType>
    struct RunCBORDeserializer<transaction::v2::SubscriptionInfo<IDType,KeyType>, void> {
        static std::optional<std::tuple<transaction::v2::SubscriptionInfo<IDType,KeyType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<typename transaction::v2::SubscriptionInfo<IDType,KeyType>::Subscriptions>, 1>
                ::apply(data, start, {
                    "subscriptions"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::SubscriptionInfo<IDType,KeyType>,size_t> {
                transaction::v2::SubscriptionInfo<IDType,KeyType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <>
    struct RunCBORSerializer<transaction::v2::UnsubscribeAll, void> {
        static std::vector<uint8_t> apply(transaction::v2::UnsubscribeAll const &x) {
            return bytedata_utils::RunCBORSerializer<VoidStruct>
                ::apply(VoidStruct {});
        }
        static std::size_t apply(transaction::v2::UnsubscribeAll const &x, char *output) {
            return bytedata_utils::RunCBORSerializer<VoidStruct>
                ::apply(VoidStruct {}, output);
        }
        static std::size_t calculateSize(transaction::v2::UnsubscribeAll const &x) {
            return bytedata_utils::RunCBORSerializer<VoidStruct>
                ::calculateSize(VoidStruct {});
        }
    };
    template <>
    struct RunCBORDeserializer<transaction::v2::UnsubscribeAll, void> {
        static std::optional<std::tuple<transaction::v2::UnsubscribeAll,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializer<VoidStruct>
                ::apply(data, start);
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::UnsubscribeAll,size_t> {
                transaction::v2::UnsubscribeAll {}
                , std::get<1>(*t)
            };
        }
    };
    template <class KeyType>
    struct RunCBORSerializer<transaction::v2::SnapshotRequest<KeyType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::SnapshotRequest<KeyType> const &x) {
            std::tuple<std::vector<KeyType> const *> t {&x.keys};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<std::vector<KeyType> const *>, 1>
                ::apply(t, {
                    "keys"
                });
        }
        static std::size_t apply(transaction::v2::SnapshotRequest<KeyType> const &x, char *output) {
            std::tuple<std::vector<KeyType> const *> t {&x.keys};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<std::vector<KeyType> const *>, 1>
                ::apply(t, {
                    "keys"
                }, output);
        }
        static std::size_t calculateSize(transaction::v2::SnapshotRequest<KeyType> const &x) {
            std::tuple<std::vector<KeyType> const *> t {&x.keys};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<std::vector<KeyType> const *>, 1>
                ::calculateSize(t, {
                    "keys"
                });
        }
    };
    template <class KeyType>
    struct RunCBORDeserializer<transaction::v2::SnapshotRequest<KeyType>, void> {
        static std::optional<std::tuple<transaction::v2::SnapshotRequest<KeyType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<std::vector<KeyType>>, 1>
                ::apply(data, start, {
                    "keys"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::SnapshotRequest<KeyType>,size_t> {
                transaction::v2::SnapshotRequest<KeyType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
}

} } } }

#endif
