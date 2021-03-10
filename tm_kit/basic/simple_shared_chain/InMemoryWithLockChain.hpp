#ifndef TM_KIT_BASIC_IN_MEMORY_WITH_LOCK_CHAIN_HPP_
#define TM_KIT_BASIC_IN_MEMORY_WITH_LOCK_CHAIN_HPP_

#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/SerializationHelperMacros.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    #define InMemoryWithLockChainItemFields \
        ((int64_t, revision)) \
        ((std::string, id)) \
        ((T, data)) \
        ((std::string, nextID)) 
    
    #define InMemoryWithLockChainStorageFields \
        ((T, data)) \
        ((std::string, nextID)) 

    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, T)), InMemoryWithLockChainItem, InMemoryWithLockChainItemFields);
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, T)), InMemoryWithLockChainStorage, InMemoryWithLockChainStorageFields);
}}}}} 

TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE_NO_FIELD_NAMES(((typename, T)), dev::cd606::tm::basic::simple_shared_chain::InMemoryWithLockChainItem, InMemoryWithLockChainItemFields);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE_NO_FIELD_NAMES(((typename, T)), dev::cd606::tm::basic::simple_shared_chain::InMemoryWithLockChainStorage, InMemoryWithLockChainStorageFields);

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    template <class T>
    class InMemoryWithLockChain {
    public:
        using StorageIDType = std::string;
        using DataType = T;
        using MapData = InMemoryWithLockChainStorage<T>;
        using TheMap = std::unordered_map<std::string, MapData>;
        using ItemType = InMemoryWithLockChainItem<T>;
        static constexpr bool SupportsExtraData = true;
    private:
        std::function<void()> updateTriggerFunc_;
        TheMap theMap_;
        std::unordered_map<std::string, std::string> extraData_;
        std::mutex mutex_;
        std::mutex chainActionMutex_;
    public:
        InMemoryWithLockChain() : updateTriggerFunc_(), theMap_({{"", MapData {}}}), extraData_(), mutex_(), chainActionMutex_() {
        }
        void setUpdateTriggerFunc(std::function<void()> f) {
            if (updateTriggerFunc_) {
                throw std::runtime_error("Duplicate attempt to set update trigger function for InMemoryWithLockChain");
            }
            updateTriggerFunc_ = f;
            if (updateTriggerFunc_) {
                updateTriggerFunc_();
            }
        }
        ItemType head(void *) {
            std::lock_guard<std::mutex> _(mutex_);
            auto &x = theMap_[""];
            return ItemType {0, "", x.data, x.nextID};
        }
        ItemType loadUntil(void *, StorageIDType const &id) {
            std::lock_guard<std::mutex> _(mutex_);
            auto &x = theMap_[id];
            return ItemType {0, id, x.data, x.nextID};
        }
        std::optional<ItemType> fetchNext(ItemType const &current) {
            std::lock_guard<std::mutex> _(mutex_);
            auto iter = theMap_.find(current.id);
            if (iter == theMap_.end()) {
                return std::nullopt;
            }
            if (iter->second.nextID == "") {
                return std::nullopt;
            }
            iter = theMap_.find(iter->second.nextID);
            if (iter == theMap_.end()) {
                return std::nullopt;
            }
            return ItemType {0, iter->first, iter->second.data, iter->second.nextID};
        }
        bool idIsAlreadyOnChain(std::string const &id) {
            std::lock_guard<std::mutex> _(mutex_);
            return (theMap_.find(id) != theMap_.end());
        }
        bool appendAfter(ItemType const &current, ItemType &&toBeWritten) {
            std::lock_guard<std::mutex> _(mutex_);
            if (theMap_.find(toBeWritten.id) != theMap_.end()) {
                return false;
            }
            auto iter = theMap_.find(current.id);
            if (iter == theMap_.end()) {
                return false;
            }
            if (iter->second.nextID != "") {
                return false;
            }
            iter->second.nextID = toBeWritten.id;
            theMap_.insert({iter->second.nextID, MapData {std::move(toBeWritten.data), ""}}).first;
            if (updateTriggerFunc_) {
                updateTriggerFunc_();
            }
            return true;
        }
        bool appendAfter(ItemType const &current, std::vector<ItemType> &&toBeWritten) {
            if (toBeWritten.empty()) {
                return true;
            }
            if (toBeWritten.size() == 1) {
                return appendAfter(current, std::move(toBeWritten[0]));
            }
            std::lock_guard<std::mutex> _(mutex_);
            auto const &firstID = toBeWritten[0].id;
            if (theMap_.find(firstID) != theMap_.end()) {
                return false;
            }
            auto iter = theMap_.find(current.id);
            if (iter == theMap_.end()) {
                return false;
            }
            if (iter->second.nextID != "") {
                return false;
            }
            iter->second.nextID = firstID;
            for (auto &&x : toBeWritten) {
                theMap_.insert({x.id, MapData {std::move(x.data), std::move(x.nextID)}});
            }
            
            if (updateTriggerFunc_) {
                updateTriggerFunc_();
            }
            return true;
        }
        template <class ExtraData>
        void saveExtraData(std::string const &key, ExtraData const &data) {
            std::lock_guard<std::mutex> _(mutex_);
            extraData_[key] = basic::bytedata_utils::RunSerializer<basic::CBOR<ExtraData>>::apply(
                basic::CBOR<ExtraData> {data}
            );
        }
        template <class ExtraData>
        std::optional<ExtraData> loadExtraData(std::string const &key) {
            std::lock_guard<std::mutex> _(mutex_);
            auto iter = extraData_.find(key);
            if (iter != extraData_.end()) {
                auto d = basic::bytedata_utils::RunDeserializer<basic::CBOR<ExtraData>>::apply(
                    iter->second
                );
                if (d) {
                    return d->value;
                } else {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
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
            return ItemType {
                0, itemID, std::move(itemData), ""
            };
        }
        static std::vector<ItemType> formChainItems(std::vector<std::tuple<StorageIDType, T>> &&itemDatas) {
            std::vector<ItemType> ret;
            bool first = true;
            for (auto &&x: itemDatas) {
                auto *p = (first?nullptr:&(ret.back()));
                if (p) {
                    p->nextID = std::get<0>(x);
                }
                ret.push_back(formChainItem(std::get<0>(x), std::move(std::get<1>(x))));
                first = false;
            }
            return ret;
        }
        static StorageIDType extractStorageID(ItemType const &p) {
            return p.id;
        }
        static T const *extractData(ItemType const &p) {
            return &(p.data);
        }
        static std::string_view extractStorageIDStringView(ItemType const &p) {
            return std::string_view {p.id};
        }
        void acquireLock() {
            chainActionMutex_.lock();
        }
        void releaseLock() {
            chainActionMutex_.unlock();
        }
    };

}}}}}

#endif
