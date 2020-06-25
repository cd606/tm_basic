#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_DATA_STORE_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_DATA_STORE_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {
    
    template <class DI, class KeyHash = std::hash<typename DI::Key>>
    struct TransactionDataStore {
        using DataMap = std::unordered_map<
            typename DI::Key
            , infra::VersionedData<typename DI::Version, std::optional<typename DI::Data>, typename DI::VersionCmp>
            , KeyHash
        >;
        std::mutex mutex_;
        DataMap dataMap_;
        std::atomic<typename DI::GlobalVersion> globalVersion_;
    };   

    template <class DI, class KeyHash = std::hash<typename T::Key>>
    using TransactionDataStorePtr = std::shared_ptr<TransactionDataStore<DI,KeyHash>>;

} } 

} } } } 

#endif