#ifndef TM_KIT_BASIC_TRANSACTION_NAMED_VALUE_STORE_DATA_MODEL_HPP_
#define TM_KIT_BASIC_TRANSACTION_NAMED_VALUE_STORE_DATA_MODEL_HPP_

#include <tm_kit/basic/SerializationHelperMacros.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/transaction/TransactionServer.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction { namespace named_value_store {

    template <class Data>
    using Item = std::tuple<std::string, Data>;

    template <class Data>
    using Collection = std::map<std::string, Data>;

    using CollectionSummary = std::size_t;

    using DeleteKeys = std::vector<std::string>;

    template <class Data>
    using InsertOrUpdateItems = std::vector<Item<Data>>;

    #define NamedValueStoreCollectionDeltaFields \
        ((dev::cd606::tm::basic::transaction::named_value_store::DeleteKeys, deletes)) \
        ((dev::cd606::tm::basic::transaction::named_value_store::InsertOrUpdateItems<Data>, inserts_updates))

    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, Data)), CollectionDelta, NamedValueStoreCollectionDeltaFields);

    template <class Data>
    struct CheckSummary {
        bool operator()(Collection<Data> const &c, CollectionSummary const &s) const {
            return (c.size() == s);
        }
    };

    template <class Data>
    struct ApplyDelta {
        CollectionDelta<Data> operator()(Collection<Data> &c, CollectionDelta<Data> const &delta) const {
            for (auto const &key : delta.deletes) {
                c.erase(key);
            }
            for (auto const &item : delta.inserts_updates) {
                c[std::get<0>(item)] = std::get<1>(item);
            }
            return delta;
        }
    };

    template <class Data>
    using TI = transaction::v2::TransactionInterface<
        int64_t
        , VoidStruct
        , int64_t
        , Collection<Data>
        , CollectionSummary
        , int64_t
        , CollectionDelta<Data>
    >;

    template <class Data>
    using DI = transaction::v2::DataStreamInterface<
        int64_t
        , VoidStruct
        , int64_t
        , Collection<Data>
        , int64_t 
        , CollectionDelta<Data>
    >;

    template <class IDType, class Data>
    using GS = transaction::v2::GeneralSubscriberTypes<
        IDType, DI<Data>
    >;

    template <class M, class Data>
    inline typename GS<typename M::EnvironmentType::IDType,Data>::Input subscription() {
        return typename GS<typename M::EnvironmentType::IDType,Data>::Input {
            typename GS<typename M::EnvironmentType::IDType,Data>::Subscription { std::vector<VoidStruct> {VoidStruct {}} }
        };
    }
    template <class M, class Data>
    inline typename GS<typename M::EnvironmentType::IDType,Data>::Input snapshotRequest() {
        return typename GS<typename M::EnvironmentType::IDType,Data>::Input {
            typename GS<typename M::EnvironmentType::IDType,Data>::SnapshotRequest { std::vector<VoidStruct> {VoidStruct {}} }
        };
    }

    using VersionChecker = std::equal_to<int64_t>;
    using VersionMerger = transaction::v2::TriviallyMerge<int64_t, int64_t>;

    template <class M, class Data>
    using TF = transaction::v2::TransactionFacility<
        M, TI<Data>, DI<Data>, transaction::v2::TransactionLoggingLevel::Verbose
        , VersionChecker
        , VersionChecker
        , CheckSummary<Data>
    >;

    template <class M, class Data, bool NeedOutput=false>
    using DM = transaction::v2::TransactionDeltaMerger<
        DI<Data>, NeedOutput, M::PossiblyMultiThreaded
        , VersionMerger
        , ApplyDelta<Data>
    >;

} } } } } }

TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, Data)), dev::cd606::tm::basic::transaction::named_value_store::CollectionDelta, NamedValueStoreCollectionDeltaFields);

#endif