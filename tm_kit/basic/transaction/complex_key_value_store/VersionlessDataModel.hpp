#ifndef TM_KIT_BASIC_TRANSACTION_NAMED_VALUE_STORE_VERSION_LESS_DATA_MODEL_HPP_
#define TM_KIT_BASIC_TRANSACTION_NAMED_VALUE_STORE_VERSION_LESS_DATA_MODEL_HPP_

#include <tm_kit/basic/SerializationHelperMacros.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/ConstType.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/transaction/TransactionServer.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction { namespace complex_key_value_store {

    template <class ItemKey, class ItemData>
    using Item = std::tuple<ItemKey, ItemData>;

    template <class ItemKey, class ItemData>
    using Collection = std::unordered_map<ItemKey, ItemData, struct_field_info_utils::StructFieldInfoBasedHash<ItemKey>>;

    using CollectionSummary = std::size_t;

    template <class ItemKey>
    using DeleteKeys = std::vector<ItemKey>;

    template <class ItemKey, class ItemData>
    using InsertOrUpdateItems = std::vector<Item<ItemKey, ItemData>>;

    #ifdef _MSC_VER
    #define VersionlessComplexKeyValueStoreCollectionDeltaFields \
        ((dev::cd606::tm::basic::transaction::complex_key_value_store::DeleteKeys<ItemKey>, deletes)) \
        ((TM_BASIC_CBOR_CAPABLE_STRUCT_PROTECT_TYPE(dev::cd606::tm::basic::transaction::complex_key_value_store::InsertOrUpdateItems<ItemKey, ItemData>), inserts_updates))
    #else
    #define VersionlessComplexKeyValueStoreCollectionDeltaFields \
        ((dev::cd606::tm::basic::transaction::complex_key_value_store::DeleteKeys<ItemKey>, deletes)) \
        (((dev::cd606::tm::basic::transaction::complex_key_value_store::InsertOrUpdateItems<ItemKey, ItemData>), inserts_updates))
    #endif
    #define VersionlessComplexKeyValueStoreParams \
        ((typename, ItemKey)) \
        ((typename, ItemData))

    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(VersionlessComplexKeyValueStoreParams, CollectionDelta, VersionlessComplexKeyValueStoreCollectionDeltaFields);

    template <class ItemKey, class ItemData>
    struct CheckSummary {
        bool operator()(Collection<ItemKey, ItemData> const &c, CollectionSummary const &s) const {
            return (c.size() == s);
        }
    };

    template <class ItemKey, class ItemData>
    struct ApplyDelta {
        CollectionDelta<ItemKey, ItemData> operator()(Collection<ItemKey, ItemData> &c, CollectionDelta<ItemKey, ItemData> const &delta) const {
            for (auto const &key : delta.deletes) {
                c.erase(key);
            }
            for (auto const &item : delta.inserts_updates) {
                c[std::get<0>(item)] = std::get<1>(item);
            }
            return delta;
        }
    };

    template <class ItemKey, class ItemData>
    using TI = transaction::v2::TransactionInterface<
        ConstType<0>
        , VoidStruct
        , ConstType<0>
        , Collection<ItemKey, ItemData>
        , CollectionSummary
        , ConstType<0>
        , CollectionDelta<ItemKey, ItemData>
    >;

    template <class ItemKey, class ItemData>
    using DI = transaction::v2::DataStreamInterface<
        ConstType<0>
        , VoidStruct
        , ConstType<0>
        , Collection<ItemKey, ItemData>
        , ConstType<0>
        , CollectionDelta<ItemKey, ItemData>
    >;

    template <class IDType, class ItemKey, class ItemData>
    using GS = transaction::v2::GeneralSubscriberTypes<
        IDType, DI<ItemKey, ItemData>
    >;

    template <class M, class ItemKey, class ItemData>
    inline typename GS<typename M::EnvironmentType::IDType,ItemKey,ItemData>::Input subscription() {
        return typename GS<typename M::EnvironmentType::IDType,ItemKey,ItemData>::Input {
            typename GS<typename M::EnvironmentType::IDType,ItemKey,ItemData>::Subscription { std::vector<VoidStruct> {VoidStruct {}} }
        };
    }
    template <class M, class ItemKey, class ItemData>
    inline typename GS<typename M::EnvironmentType::IDType,ItemKey,ItemData>::Input snapshotRequest() {
        return typename GS<typename M::EnvironmentType::IDType,ItemKey,ItemData>::Input {
            typename GS<typename M::EnvironmentType::IDType,ItemKey,ItemData>::SnapshotRequest { std::vector<VoidStruct> {VoidStruct {}} }
        };
    }

    using VersionChecker = std::equal_to<ConstType<0>>;
    class VersionMerger {
        void operator()(ConstType<0> &d, ConstType<0> const &) const {}
    };

    template <class M, class ItemKey, class ItemData>
    using TF = transaction::v2::TransactionFacility<
        M, TI<ItemKey, ItemData>, DI<ItemKey, ItemData>, transaction::v2::TransactionLoggingLevel::Verbose
        , VersionChecker
        , VersionChecker
        , CheckSummary<ItemKey, ItemData>
    >;

    template <class M, class ItemKey, class ItemData, bool NeedOutput=false>
    using DM = transaction::v2::TransactionDeltaMerger<
        DI<ItemKey, ItemData>, NeedOutput, M::PossiblyMultiThreaded
        , VersionMerger
        , ApplyDelta<ItemKey, ItemData>
    >;

    template <class ItemData>
    using KeyBasedQueryResult = basic::CBOR<std::optional<ItemData>>;

    template <class ItemKey, class ItemData>
    using FullDataResult = basic::CBOR<Collection<ItemKey,ItemData>>;

} } } } } }

TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(VersionlessComplexKeyValueStoreParams, dev::cd606::tm::basic::transaction::complex_key_value_store::CollectionDelta, VersionlessComplexKeyValueStoreCollectionDeltaFields);

#endif