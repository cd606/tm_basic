#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_DATA_STORE_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_DATA_STORE_HPP_

#include <unordered_map>
#include <type_traits>
#include <mutex>
#include <thread>
#include <chrono>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {
    
    template <class DI, class KeyHashType = std::hash<typename DI::Key>, bool MutexProtected=true>
    struct TransactionDataStore {
        static constexpr bool IsMutexProtected = MutexProtected;
        using KeyHash = KeyHashType;

        using Mutex = std::conditional_t<MutexProtected,std::mutex,bool>;
        using Lock = std::conditional_t<MutexProtected, std::lock_guard<std::mutex>, bool>;
        using GlobalVersion = std::conditional_t<MutexProtected,std::atomic<typename DI::GlobalVersion>,typename DI::GlobalVersion>;

        using DataMap = std::unordered_map<
            typename DI::Key
            , infra::VersionedData<typename DI::Version, std::optional<typename DI::Data>, typename DI::VersionCmp>
            , KeyHash
        >;
        
        mutable Mutex mutex_;
        DataMap dataMap_;
        GlobalVersion globalVersion_;

        void waitForGlobalVersion(typename DI::GlobalVersion const &v) {
            if constexpr (IsMutexProtected) {
                while (globalVersion_ < v) {
                    ;//std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        }

        typename DI::Update createFullUpdateNotification(std::vector<typename DI::Key> const &keys) const {
            Lock _(mutex_);
            std::vector<typename DI::OneUpdateItem> updates;  
            for (auto const &key : keys) {
                auto iter = dataMap_.find(key);
                if (iter == dataMap_.end()) {
                    updates.push_back(typename DI::OneFullUpdateItem {
                        infra::withtime_utils::makeValueCopy(key)
                        , typename DI::Version {}
                        , std::nullopt
                    });
                } else {
                    updates.push_back(typename DI::OneFullUpdateItem {
                        infra::withtime_utils::makeValueCopy(key)
                        , infra::withtime_utils::makeValueCopy(iter->second.version)
                        , infra::withtime_utils::makeValueCopy(iter->second.data)
                    });
                }
            }
            return typename DI::Update {
                globalVersion_
                , std::move(updates)
            };
        }
        typename DI::FullUpdate createFullUpdateNotificationWithoutVariant(std::vector<typename DI::Key> const &keys) const {
            Lock _(mutex_);
            std::vector<typename DI::OneFullUpdateItem> updates;  
            for (auto const &key : keys) {
                auto iter = dataMap_.find(key);
                if (iter == dataMap_.end()) {
                    updates.push_back(typename DI::OneFullUpdateItem {
                        infra::withtime_utils::makeValueCopy(key)
                        , typename DI::Version {}
                        , std::nullopt
                    });
                } else {
                    updates.push_back(typename DI::OneFullUpdateItem {
                        infra::withtime_utils::makeValueCopy(key)
                        , infra::withtime_utils::makeValueCopy(iter->second.version)
                        , infra::withtime_utils::makeValueCopy(iter->second.data)
                    });
                }
            }
            return typename DI::FullUpdate {
                globalVersion_
                , std::move(updates)
            };
        }
    };   

    template <class DI, class KeyHashType = std::hash<typename DI::Key>, bool MutexProtected=true>
    using TransactionDataStorePtr = std::shared_ptr<TransactionDataStore<DI,KeyHashType,MutexProtected>>;

    template <class DI, class KeyHashType = std::hash<typename DI::Key>, bool MutexProtected=true>
    using TransactionDataStoreConstPtr = std::shared_ptr<TransactionDataStore<DI,KeyHashType,MutexProtected> const>;

} } 

} } } } 

#endif