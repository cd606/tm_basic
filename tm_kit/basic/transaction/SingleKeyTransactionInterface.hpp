#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_TRANSACTION_INTERFACE_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_TRANSACTION_INTERFACE_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <type_traits>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class IDType
        , class DataSummaryType = DataType
        , class Cmp = std::less<VersionType>
        , class Enable = void 
    >
    class SingleKeyTransactionInterface {};

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class IDType
        , class DataSummaryType
        , class Cmp
    >
    class SingleKeyTransactionInterface<
        KeyType
        , DataType
        , VersionType
        , IDType
        , DataSummaryType
        , Cmp
        , std::enable_if_t<
            std::is_default_constructible_v<KeyType>
            && std::is_copy_constructible_v<KeyType>
            && std::is_default_constructible_v<VersionType>
            && std::is_copy_constructible_v<VersionType>
            && std::is_default_constructible_v<DataSummaryType>
            && std::is_copy_constructible_v<DataSummaryType>
        >
    > {
    public:
        struct InsertAction {
            KeyType key;
            DataType data;
            void SerializeToString(std::string *s) const {
                CBOR<std::tuple<KeyType const *, DataType const *>> t {{&key, &data}};
                *s = bytedata_utils::RunSerializer<CBOR<std::tuple<KeyType const *, DataType const *>>>
                    ::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<
                            CBOR<std::tuple<KeyType,DataType>>
                            >::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(std::get<0>(res->value));
                data = std::move(std::get<1>(res->value));
                return true;
            }
        };
        struct UpdateAction {
            KeyType key;
            VersionType oldVersion;
            DataSummaryType oldDataSummary;
            DataType newData;
            void SerializeToString(std::string *s) const {
                CBOR<std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataType const *>> t {{&key, &oldVersion, &oldDataSummary, &newData}};
                *s = bytedata_utils::RunSerializer<
                        CBOR<std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataType const *>>
                    >::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<
                            CBOR<std::tuple<KeyType,VersionType,DataSummaryType,DataType>>
                            >::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(std::get<0>(res->value));
                oldVersion = std::move(std::get<1>(res->value));
                oldDataSummary = std::move(std::get<2>(res->value));
                newData = std::move(std::get<3>(res->value));
                return true;
            }
        };
        struct DeleteAction {
            KeyType key;
            VersionType oldVersion;
            DataSummaryType oldDataSummary;
            void SerializeToString(std::string *s) const {
                CBOR<std::tuple<KeyType const *, VersionType const *, DataSummaryType const *>> t {{&key, &oldVersion, &oldDataSummary}};
                *s = bytedata_utils::RunSerializer<
                        CBOR<std::tuple<KeyType const *, VersionType const *, DataSummaryType const *>>
                    >::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<
                            CBOR<std::tuple<KeyType,VersionType,DataSummaryType>>
                            >::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(std::get<0>(res->value));
                oldVersion = std::move(std::get<1>(res->value));
                oldDataSummary = std::move(std::get<2>(res->value));
                return true;
            }
        };

        using Transaction = std::variant<InsertAction,UpdateAction,DeleteAction>;

        struct Subscription {
            KeyType key;
            void SerializeToString(std::string *s) const {
                *s = bytedata_utils::RunSerializer<CBOR<KeyType const *>>::apply(CBOR<KeyType const *> {&key});
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<CBOR<KeyType>>::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(res->value);
                return true;
            }
        };
        struct Unsubscription {
            IDType originalSubscriptionID;
            KeyType key;
            void SerializeToString(std::string *s) const {
                CBOR<std::tuple<IDType const *, KeyType const *>> t {{&originalSubscriptionID, &key}};
                *s = bytedata_utils::RunSerializer<CBOR<std::tuple<IDType const *, KeyType const *>>>::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<CBOR<std::tuple<IDType,KeyType>>>::apply(s);
                if (!res) {
                    return false;
                }
                originalSubscriptionID = std::move(std::get<0>(res->value));
                key = std::move(std::get<1>(res->value));
                return true;
            }
        };
        using OneValue = infra::GroupedVersionedData<
                            KeyType
                            , VersionType
                            , std::optional<DataType>
                            , Cmp
                        >;
      
        using TransactionSuccess = ConstType<100>;
        using TransactionFailurePermission = ConstType<101>;
        using TransactionFailurePrecondition = ConstType<102>;
        using TransactionResult = std::variant<TransactionSuccess,TransactionFailurePermission,TransactionFailurePrecondition>;

        using SubscriptionAck = ConstType<103>;
        using UnsubscriptionAck = ConstType<104>;

        using BasicFacilityInput = CBOR<std::variant<
                                    Transaction
                                    , Subscription
                                    , Unsubscription
                                >>;
        
        //The std::string is the account information
        using FacilityInput = std::tuple<
                                    std::string
                                    , BasicFacilityInput
                                >;
        using FacilityOutput = CBOR<std::variant<
                                    TransactionResult
                                    , SubscriptionAck
                                    , UnsubscriptionAck
                                    , OneValue
                                >>;
    };

} } } } }

#endif