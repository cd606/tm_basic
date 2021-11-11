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
        std::mutex mutex_; //for extra data only
        std::unordered_map<std::string, void *> extraData_;

        std::condition_variable notificationCond_;
    public:
        using StorageIDType = std::string;
        using DataType = T;
        using ItemType = InMemoryLockFreeChainItem<T>;
        static constexpr bool SupportsExtraData = true;
        InMemoryLockFreeChain() : head_(), mutex_(), extraData_(), notificationCond_() {
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
            notificationCond_.notify_all();
            return ret;
        }
        bool appendAfter(ItemType const &current, std::vector<ItemType> &&toBeWritten) {
            if (toBeWritten.empty()) {
                return true;
            }
            if (toBeWritten.size() == 1) {
                return appendAfter(current, std::move(toBeWritten[0]));
            }
            if (!current) {
                throw InMemoryLockFreeChainException("AppendAfter on nullptr");
            }
            auto *first = toBeWritten[0];
            if (!first) {
                throw InMemoryLockFreeChainException("AppendAfter trying to append nullptr");
            }
            InMemoryLockFreeChainStorageItem<T> *p = nullptr;
            bool ret = std::atomic_compare_exchange_strong<InMemoryLockFreeChainStorageItem<T> *>(
                &(current->next)
                , &p
                , first
            );
            if (!ret) {
                for (auto *p : toBeWritten) {
                    delete p;
                }
            }
            notificationCond_.notify_all();
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
        static std::vector<ItemType> formChainItems(std::vector<std::tuple<StorageIDType, T>> &&itemDatas) {
            std::vector<ItemType> ret;
            bool first = true;
            for (auto &&x : itemDatas) {
                auto *p = (first?nullptr:ret.back());
                auto *q = formChainItem(std::get<0>(x), std::move(std::get<1>(x)));
                if (p) {
                    p->next = q;
                }
                ret.push_back(q);
                first = false;
            }
            return ret;
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
        template <class ExtraData>
        void saveExtraData(std::string const &key, ExtraData const &data) {
            std::lock_guard<std::mutex> _(mutex_);
            extraData_[key] = new ExtraData(data);
        }
        template <class ExtraData>
        std::optional<ExtraData> loadExtraData(std::string const &key) {
            std::lock_guard<std::mutex> _(mutex_);
            auto iter = extraData_.find(key);
            if (iter == extraData_.end()) {
                return std::nullopt;
            }
            return ExtraData(*((ExtraData *) (iter->second)));
        }
        void waitForUpdate(std::chrono::system_clock::duration const &d) {
            std::mutex mut;
            std::unique_lock<std::mutex> lock(mut);
            notificationCond_.wait_for(lock, d);
            lock.unlock();
        }
    };

}}}}}

#endif