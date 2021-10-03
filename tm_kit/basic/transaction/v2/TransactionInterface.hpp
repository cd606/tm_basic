#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_INTERFACE_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_INTERFACE_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/SerializationHelperMacros.hpp>
#include <type_traits>
#include <functional>
#include <iostream>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    enum class RequestDecision : int16_t {
        Success = 0
        , FailurePrecondition = 1
        , FailurePermission = 2
        , FailureConsistency = 3
    };

    inline std::ostream &operator<<(std::ostream &os, RequestDecision d) {
        switch (d) {
        case RequestDecision::Success:
            os << "Success";
            break;
        case RequestDecision::FailurePrecondition:
            os << "FailurePrecondition";
            break;
        case RequestDecision::FailurePermission:
            os << "FailurePermission";
            break;
        case RequestDecision::FailureConsistency:
            os << "FailureConsistency";
            break;
        default:
            os << "(Unknown)";
            break;
        }
        return os;
    }

    #define InsertActionFields \
        ((KeyType, key)) \
        ((DataType, data))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, KeyType)) ((typename, DataType)), InsertAction, InsertActionFields);

    #define UpdateActionFields \
        ((KeyType, key)) \
        ((std::optional<VersionSliceType>, oldVersionSlice)) \
        ((std::optional<DataSummaryType>, oldDataSummary)) \
        ((DataDeltaType, dataDelta))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, KeyType)) ((typename, VersionSliceType)) ((typename, DataSummaryType)) ((typename, DataDeltaType)), UpdateAction, UpdateActionFields);

    #define DeleteActionFields \
        ((KeyType, key)) \
        ((std::optional<VersionType>, oldVersion)) \
        ((std::optional<DataSummaryType>, oldDataSummary))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, KeyType)) ((typename, VersionType)) ((typename, DataSummaryType)), DeleteAction, DeleteActionFields);

    #define TransactionResponseFields \
        ((GlobalVersionType, globalVersion)) \
        ((dev::cd606::tm::basic::transaction::v2::RequestDecision, requestDecision))
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(((typename, GlobalVersionType)), TransactionResponse, TransactionResponseFields);

    template <
        class GlobalVersionType
        , class KeyType
        , class VersionType
        , class DataType
        , class DataSummaryType = DataType
        , class VersionSliceType = VersionType
        , class DataDeltaType = DataType
        , class ProcessedUpdateType = DataDeltaType
        , class Enable = void 
    >
    class TransactionInterface {};

    template <
        class GlobalVersionType
        , class KeyType
        , class VersionType
        , class DataType
        , class DataSummaryType
        , class VersionSliceType
        , class DataDeltaType
        , class ProcessedUpdateType
    >
    class TransactionInterface<
        GlobalVersionType
        , KeyType
        , VersionType
        , DataType
        , DataSummaryType
        , VersionSliceType
        , DataDeltaType
        , ProcessedUpdateType
        , std::enable_if_t<
               std::is_default_constructible_v<GlobalVersionType>
            && std::is_copy_constructible_v<GlobalVersionType>
            && std::is_default_constructible_v<KeyType>
            && std::is_copy_constructible_v<KeyType>
            && std::is_default_constructible_v<VersionType>
            && std::is_copy_constructible_v<VersionType>
        >
    > {
    public:
        using GlobalVersion = GlobalVersionType;
        using Key = KeyType;
        using Version = VersionType;
        using Data = DataType;
        using DataSummary = DataSummaryType;
        using VersionSlice = VersionSliceType;
        using DataDelta = DataDeltaType;
        using ProcessedUpdate = ProcessedUpdateType;

        using InsertAction = transaction::v2::InsertAction<KeyType,DataType>;
        using UpdateAction = transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType>;
        using DeleteAction = transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType>;

        using Transaction = SingleLayerWrapper<std::variant<InsertAction,UpdateAction,DeleteAction>>;
        using TransactionResponse = transaction::v2::TransactionResponse<GlobalVersionType>;

        using Account = std::string;
        using TransactionWithAccountInfo = std::tuple<Account,Transaction>;
    };

} }

} } } }

TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, KeyType)) ((typename, DataType)), dev::cd606::tm::basic::transaction::v2::InsertAction, InsertActionFields);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, KeyType)) ((typename, VersionSliceType)) ((typename, DataSummaryType)) ((typename, DataDeltaType)), dev::cd606::tm::basic::transaction::v2::UpdateAction, UpdateActionFields);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, KeyType)) ((typename, VersionType)) ((typename, DataSummaryType)), dev::cd606::tm::basic::transaction::v2::DeleteAction, DeleteActionFields);
TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(((typename, GlobalVersionType)), dev::cd606::tm::basic::transaction::v2::TransactionResponse, TransactionResponseFields);

#endif