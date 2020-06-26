#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_DATA_STORE_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_DATA_STORE_HPP_

#include <unordered_map>
#include <type_traits>
#include <mutex>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {
    
    template <class DI, class KeyHash = std::hash<typename DI::Key>, bool MutexProtected=true>
    struct TransactionDataStore {
        using DataMap = std::unordered_map<
            typename DI::Key
            , infra::VersionedData<typename DI::Version, std::optional<typename DI::Data>, typename DI::VersionCmp>
            , KeyHash
        >;
        std::conditional_t<MutexProtected,std::mutex,bool> mutex_;
        using Lock = std::conditional_t<MutexProtected, std::lock_guard<std::mutex>, bool>;
        DataMap dataMap_;
        using GlobalVersion = std::conditional_t<MutexProtected,std::atomic<typename DI::GlobalVersion>,typename DI::GlobalVersion>;
        GlobalVersion globalVersion_;
    };   

    template <class DI, class KeyHash = std::hash<typename DI::Key>, bool MutexProtected=true>
    using TransactionDataStorePtr = std::shared_ptr<TransactionDataStore<DI,KeyHash,MutexProtected>>;

} } 

} } } } 

#endif