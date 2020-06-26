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
    template <class KeyType>
    inline bool operator==(Subscription<KeyType> const &x, Subscription<KeyType> const &y) {
        return (x.keys == y.keys);
    }
    template <class IDType, class KeyType>
    inline bool operator==(Unsubscription<IDType> const &x, Unsubscription<IDType> const &y) {
        return (x.id == y.id);
    }

    //subscription and unsubscription ack's just use the exact same data types

    template <
        class M
        , class DI
        >
    struct GeneralSubscriberTypes {
        using Monad = M;
        using Key = typename DI::Key;
        using ID = typename M::EnvironmentType::IDType;

        using Subscription = transaction::v2::Subscription<Key>;
        using Unsubscription = transaction::v2::Unsubscription<ID>;
        using SubscriptionUpdate = typename DI::Update;

        using Input = CBOR<
            std::variant<Subscription, Unsubscription>
        >;
        using Output = CBOR<
            std::variant<Subscription, Unsubscription, SubscriptionUpdate>
        >;

        using IGeneralSubscriber = typename M::template AbstractIntegratedLocalOnOrderFacility<
            Input
            , Output
            , SubscriptionUpdate
        >;
    };

    template <class M, class DI, class KeyHash = std::hash<typename DI::Key>>
    class GeneralSubscriber 
        : public GeneralSubscriberTypes<M,DI>::IGeneralSubscriber
    {       
    private:
        using Types = GeneralSubscriberTypes<M,DI>;
        using SubscriptionMap = std::unordered_map<typename Types::Key, std::vector<typename Types::ID>, KeyHash>;
        SubscriptionMap subscriptionMap_;
        using IDInfoMap = std::unordered_map<typename Types::ID, std::vector<typename Types::Key>, typename M::EnvironmentType::IDHash>;
        IDInfoMap idInfoMap_;

        using DataStorePtr = transaction::v2::TransactionDataStorePtr<
            DI, KeyHash, M::PossiblyMultiThreaded
        >;
        using DataStore = typename DataStorePtr::element_type;
        using Mutex = typename DataStore::Mutex;

        Mutex mutex_;

        DataStorePtr dataStorePtr_;

        std::vector<typename Types::Key> addSubscription(typename M::EnvironmentType *env, typename Types::ID id, typename Types::Subscription const &subscription) {
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
            idInfoMap_.insert(std::make_pair(id, ret));
            return ret;
        }
        void removeSubscription(typename M::EnvironmentType *env, typename Types::Unsubscription const &unsubscription) {
            auto idInfoIter = idInfoMap_.find(unsubscription.originalSubscriptionID);
            if (idInfoIter == idInfoMap_.end()) {
                return;
            }
            for (auto const &key : idInfoIter->second) {
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
        }
    public:
        GeneralSubscriber(DataStorePtr const &dataStorePtr) 
            : subscriptionMap_(), idInfoMap_(), mutex_(), dataStorePtr_(dataStorePtr) {}
        virtual ~GeneralSubscriber() {}       
        
        void start(typename M::EnvironmentType *) override final {
        }
        void handle(typename M::template InnerData<typename M::template Key<typename Types::Input>> &&input) override final {
            auto *env = input.environment;
            typename Types::ID id = input.timedData.value.id();
            std::visit([this,env,id](auto &&d) {
                using T = std::decay_t<decltype(d)>;
                if constexpr (std::is_same_v<T, typename Types::Subscription>) {
                    typename Types::Subscription subscription {std::move(d)};
                    std::vector<typename Types::Key> newKeys;
                    {
                        typename DataStore::Lock _(mutex_);
                        newKeys = addSubscription(env, id, subscription); 
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
                    std::vector<typename Types::Key> removedKeys;
                    {
                        typename DataStore::Lock _(mutex_);
                        removeSubscription(env, unsubscription);
                    }
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
                }
            }, std::move(input.timedData.value.key().value));
        }
        void handle(typename M::template InnerData<typename Types::SubscriptionUpdate> &&update) override final {
            auto *env = update.environment;
            auto globalVersion = update.timedData.value.version;
            std::unordered_map<typename Types::ID, std::vector<typename DI::OneUpdateItem>, typename M::EnvironmentType::IDHash> updateMap;
            for (auto &x : update.timedData.value.data) {
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
}

} } } }

#endif
