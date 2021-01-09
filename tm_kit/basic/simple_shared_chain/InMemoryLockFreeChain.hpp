#ifndef TM_KIT_BASIC_IN_MEMORY_LOCK_FREE_CHAIN_HPP_
#define TM_KIT_BASIC_IN_MEMORY_LOCK_FREE_CHAIN_HPP_

#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <atomic>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    class InMemoryLockFreeChainException : public std::runtime_error {
    public:
        InMemoryLockFreeChainException(std::string const &s) : std::runtime_error(s) {}
    };

    template <class T>
    struct InMemoryLockFreeChainStorageItem {
        std::string id;
        T data;
        std::atomic<InMemoryLockFreeChainStorageItem<T> *> next;

        InMemoryLockFreeChainStorageItem() : id(""), data(), next(nullptr) {}
        InMemoryLockFreeChainStorageItem(std::string const &s, T const &d, InMemoryLockFreeChainStorageItem<T> *p) : id(s), data(d), next(p) {}
        InMemoryLockFreeChainStorageItem(std::string const &s, T &&d, InMemoryLockFreeChainStorageItem<T> *p) : id(s), data(std::move(d)), next(p) {}
    };
    
    template <class T>
    using InMemoryLockFreeChainItem = InMemoryLockFreeChainStorageItem<T> *;
    
    template <class T>
    class InMemoryLockFreeChain {
    private:
        InMemoryLockFreeChainStorageItem<T> head_;
    public:
        using StorageIDType = std::string;
        using DataType = T;
        using ItemType = InMemoryLockFreeChainItem<T>;
        static constexpr bool SupportsExtraData = false;
        InMemoryLockFreeChain() : head_() {
        }
        ItemType head(void *) {
            return &head_;
        }
        std::optional<ItemType> fetchNext(ItemType const &current) {
            if (!current) {
                throw InMemoryLockFreeChainException("FetchNext on nullptr");
            }
            InMemoryLockFreeChainStorageItem<T> *p = current->next.load(std::memory_order_acquire);
            if (p) {
                return p;
            } else {
                return std::nullopt;
            }
        }
        bool appendAfter(ItemType const &current, ItemType &&toBeWritten) {
            if (!current) {
                throw InMemoryLockFreeChainException("AppendAfter on nullptr");
            }
            if (!toBeWritten) {
                throw InMemoryLockFreeChainException("AppendAfter trying to append nullptr");
            }
            InMemoryLockFreeChainStorageItem<T> *p = nullptr;
            bool ret = std::atomic_compare_exchange_strong<InMemoryLockFreeChainStorageItem<T> *>(
                &(current->next)
                , &p
                , toBeWritten
            );
            if (!ret) {
                delete toBeWritten;
            }
            return ret;
        }
        template <class Env>
        static StorageIDType newStorageID() {
            return Env::id_to_string(Env::new_id());
        }
        template <class Env>
        static std::string newStorageIDAsString() {
            return newStorageID<Env>();
        }
        static ItemType formChainItem(StorageIDType const &itemID, T &&itemData) {
            return new InMemoryLockFreeChainStorageItem<T> {
                itemID, std::move(itemData), nullptr
            };
        }
        static StorageIDType extractStorageID(ItemType const &p) {
            return p->id;
        }
        static T const *extractData(ItemType const &p) {
            return &(p->data);
        }
        static std::string_view extractStorageIDStringView(ItemType const &p) {
            return std::string_view {p->id};
        }
    };

}}}}}

#endif