#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_INTERFACE_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_INTERFACE_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <type_traits>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    enum class RequestDecision : int16_t {
        Success = 0
        , FailurePrecondition = 1
        , FailurePermission = 2
        , FailureConsistency = 3
    };

    template <class KeyType, class DataType>
    struct InsertAction {
        KeyType key;
        DataType data;
    };

    template <class KeyType, class VersionSliceType, class DataSummaryType, class DataDeltaType>
    struct UpdateAction {
        KeyType key;
        std::optional<VersionSliceType> oldVersionSlice;
        std::optional<DataSummaryType> oldDataSummary;
        DataDeltaType dataDelta;
    };

    template <class KeyType, class VersionType, class DataSummaryType>
    struct DeleteAction {
        KeyType key;
        std::optional<VersionType> oldVersion;
        std::optional<DataSummaryType> oldDataSummary;
    };

    template <class GlobalVersionType>
    struct TransactionResponse {
        GlobalVersionType globalVersion;
        RequestDecision requestDecision;
    };

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

        using Transaction = CBOR<std::variant<InsertAction,UpdateAction,DeleteAction>>;
        using TransactionResponse = CBOR<transaction::v2::TransactionResponse<GlobalVersionType>>;

        using Account = std::string;
        using TransactionWithAccountInfo = std::tuple<Account,Transaction>;
    };

} }

namespace bytedata_utils {
    template <class KeyType, class DataType>
    struct RunCBORSerializer<transaction::v2::InsertAction<KeyType,DataType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::InsertAction<KeyType,DataType> const &x) {
            std::tuple<KeyType const *, DataType const *> t {&x.key, &x.data};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *, DataType const *>, 2>
                ::apply(t, {
                    "key", "data"
                });
        }
    };
    template <class KeyType, class DataType>
    struct RunCBORDeserializer<transaction::v2::InsertAction<KeyType,DataType>, void> {
        static std::optional<std::tuple<transaction::v2::InsertAction<KeyType,DataType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType, DataType>,2>
                ::apply(data, start, {
                    "key", "data"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::InsertAction<KeyType,DataType>,size_t> {
                transaction::v2::InsertAction<KeyType,DataType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , std::move(std::get<1>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class KeyType, class VersionSliceType, class DataSummaryType, class DataDeltaType>
    struct RunCBORSerializer<transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType> const &x) {
            std::tuple<KeyType const *, std::optional<VersionSliceType> const *, std::optional<DataSummaryType> const *, DataDeltaType const *> t {&x.key, &x.oldVersionSlice, &x.oldDataSummary, &x.dataDelta};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *, std::optional<VersionSliceType> const *, std::optional<DataSummaryType> const *, DataDeltaType const *>, 4>
                ::apply(t, {
                    "key", "old_version_slice", "old_data_summary", "data_delta"
                });
        }
    };
    template <class KeyType, class VersionSliceType, class DataSummaryType, class DataDeltaType>
    struct RunCBORDeserializer<transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType>, void> {
        static std::optional<std::tuple<transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType, std::optional<VersionSliceType>, std::optional<DataSummaryType>, DataDeltaType>, 4>
                ::apply(data, start, {
                    "key", "old_version_slice", "old_data_summary", "data_delta"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType>,size_t> {
                transaction::v2::UpdateAction<KeyType,VersionSliceType,DataSummaryType,DataDeltaType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , std::move(std::get<1>(std::get<0>(*t)))
                    , std::move(std::get<2>(std::get<0>(*t)))
                    , std::move(std::get<3>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class KeyType, class VersionType, class DataSummaryType>
    struct RunCBORSerializer<transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType> const &x) {
            std::tuple<KeyType const *, std::optional<VersionType> const *, std::optional<DataSummaryType> const *> t {&x.key, &x.oldVersion, &x.oldDataSummary};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *, std::optional<VersionType> const *, std::optional<DataSummaryType> const *>, 3>
                ::apply(t, {
                    "key", "old_version", "old_data_summary"
                });
        }
    };
    template <class KeyType, class VersionType, class DataSummaryType>
    struct RunCBORDeserializer<transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType>, void> {
        static std::optional<std::tuple<transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType, std::optional<VersionType>, std::optional<DataSummaryType>>, 3>
                ::apply(data, start, {
                    "key", "old_version", "old_data_summary"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType>,size_t> {
                transaction::v2::DeleteAction<KeyType,VersionType,DataSummaryType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , std::move(std::get<1>(std::get<0>(*t)))
                    , std::move(std::get<2>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class GlobalVersionType>
    struct RunCBORSerializer<transaction::v2::TransactionResponse<GlobalVersionType>, void> {
        static std::vector<uint8_t> apply(transaction::v2::TransactionResponse<GlobalVersionType> const &x) {
            int16_t d = static_cast<int16_t>(x.requestDecision);
            std::tuple<GlobalVersionType const *, int16_t const *> t {&x.globalVersion, &d};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<GlobalVersionType const *, int16_t const *>, 2>
                ::apply(t, {
                    "global_version", "request_decision"
                });
        }
    };
    template <class GlobalVersionType>
    struct RunCBORDeserializer<transaction::v2::TransactionResponse<GlobalVersionType>, void> {
        static std::optional<std::tuple<transaction::v2::TransactionResponse<GlobalVersionType>,size_t>> apply(std::string_view const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<GlobalVersionType, int16_t>, 2>
                ::apply(data, start, {
                    "global_version", "request_decision"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::v2::TransactionResponse<GlobalVersionType>,size_t> {
                transaction::v2::TransactionResponse<GlobalVersionType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , static_cast<transaction::v2::RequestDecision>(std::get<1>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
}

} } } }


#endif