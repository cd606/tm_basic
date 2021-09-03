#ifndef TM_KIT_BASIC_EMPTY_CHAIN_HPP_
#define TM_KIT_BASIC_EMPTY_CHAIN_HPP_

#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/ByteData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
  
    template <class T, bool AllowExtraData=false>
    class EmptyChain {};

    template <class T>
    class EmptyChain<T,false> {
    public:
        using StorageIDType = std::string;
        using DataType = T;
        using ItemType = std::tuple<StorageIDType,T>;
        static constexpr bool SupportsExtraData = false;
        ItemType head(void *) {
            return {"", {}};
        }
        std::optional<ItemType> fetchNext(ItemType const &current) {
            return std::nullopt;
        }
        bool appendAfter(ItemType const &current, ItemType &&toBeWritten) {
            return true;
        }
        bool appendAfter(ItemType const &current, std::vector<ItemType> &&toBeWritten) {
            return true;
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
            return {itemID, std::move(itemData)};
        }
        static std::vector<ItemType> formChainItems(std::vector<std::tuple<StorageIDType, T>> &&itemDatas) {
            return std::move(itemDatas);
        }
        static StorageIDType extractStorageID(ItemType const &p) {
            return std::get<0>(p);
        }
        static T const *extractData(ItemType const &p) {
            return &(std::get<1>(p));
        }
        static std::string_view extractStorageIDStringView(ItemType const &p) {
            return std::string_view {std::get<0>(p)};
        }
    };

    template <class T>
    class EmptyChain<T,true> {
    public:
        using StorageIDType = std::string;
        using DataType = T;
        using ItemType = std::tuple<StorageIDType,T>;
        static constexpr bool SupportsExtraData = true;
        ItemType head(void *) {
            return {"", {}};
        }
        std::optional<ItemType> fetchNext(ItemType const &current) {
            return std::nullopt;
        }
        bool appendAfter(ItemType const &current, ItemType &&toBeWritten) {
            return true;
        }
        bool appendAfter(ItemType const &current, std::vector<ItemType> &&toBeWritten) {
            return true;
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
            return {itemID, std::move(itemData)};
        }
        static std::vector<ItemType> formChainItems(std::vector<std::tuple<StorageIDType, T>> &&itemDatas) {
            return std::move(itemDatas);
        }
        static StorageIDType extractStorageID(ItemType const &p) {
            return std::get<0>(p);
        }
        static T const *extractData(ItemType const &p) {
            return &(std::get<1>(p));
        }
        static std::string_view extractStorageIDStringView(ItemType const &p) {
            return std::string_view {std::get<0>(p)};
        }
        template <class ExtraData>
        void saveExtraData(std::string const &key, ExtraData const &data) {
        }
        template <class ExtraData>
        std::optional<ExtraData> loadExtraData(std::string const &key) {
            return std::nullopt;
        }
    };

}}}}}

#endif