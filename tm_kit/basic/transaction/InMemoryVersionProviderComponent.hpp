#ifndef TM_KIT_BASIC_TRANSACTION_IN_MEMORY_VERSION_PROVIDER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_IN_MEMORY_VERSION_PROVIDER_COMPONENT_HPP_

#include <mutex>
#include <unordered_map>

#include <tm_kit/basic/transaction/VersionProviderComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    //This implementation is purely in memory and cannot preserve version
    //across runs or across simultaneous processes
    template <class KeyType, class DataType>
    class InMemoryVersionProviderComponent final
        : public VersionProviderComponent<KeyType,DataType,int64_t> {
    private:
        std::mutex mutex_;
        std::unordered_map<KeyType,int64_t> version_;
    public:
        InMemoryVersionProviderComponent(std::string const &filePath) 
            : mutex_(), version_()
        {

        }
        virtual ~InMemoryVersionProviderComponent() {
        }
        virtual int64_t getNextVersionForKey(KeyType const &k) {
            std::lock_guard<std::mutex> _(mutex_);
            auto iter = version_.find(k);
            if (iter == version_.end()) {
                iter = version_.insert({k, 0}).first;
            }
            ++iter->second;
            return iter->second;
        }
    };

} } } } }
#endif