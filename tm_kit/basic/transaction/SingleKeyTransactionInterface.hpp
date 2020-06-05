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
        , class CheckSummary = std::equal_to<DataType>
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
        , class CheckSummary
    >
    class SingleKeyTransactionInterface<
        KeyType
        , DataType
        , VersionType
        , IDType
        , DataSummaryType
        , Cmp
        , CheckSummary
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
                std::tuple<KeyType const *, DataType const *> t {&key, &data};
                *s = bytedata_utils::RunSerializer<std::tuple<KeyType const *, DataType const *>>
                    ::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<
                            std::tuple<KeyType,DataType>
                            >::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(std::get<0>(*res));
                data = std::move(std::get<1>(*res));
                return true;
            }
        };
        struct UpdateAction {
            KeyType key;
            VersionType oldVersion;
            DataSummaryType oldData;
            DataType newData;
            void SerializeToString(std::string *s) const {
                std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataType const *> t {&key, &oldVersion, &oldData, &newData};
                *s = bytedata_utils::RunSerializer<
                        std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataType const *>
                    >::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<
                            std::tuple<KeyType,VersionType,DataSummaryType,DataType>
                            >::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(std::get<0>(*res));
                oldVersion = std::move(std::get<1>(*res));
                oldData = std::move(std::get<2>(*res));
                newData = std::move(std::get<3>(*res));
                return true;
            }
        };
        struct DeleteAction {
            KeyType key;
            void SerializeToString(std::string *s) const {
                *s = bytedata_utils::RunSerializer<KeyType>::apply(key);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<KeyType>::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(*res);
                return true;
            }
        };

        using Transaction = std::variant<InsertAction,UpdateAction,DeleteAction>;

        struct Subscription {
            KeyType key;
            void SerializeToString(std::string *s) const {
                *s = bytedata_utils::RunSerializer<KeyType>::apply(key);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<KeyType>::apply(s);
                if (!res) {
                    return false;
                }
                key = std::move(*res);
                return true;
            }
        };
        struct Unsubscription {
            IDType originalSubscriptionID;
            KeyType key;
            void SerializeToString(std::string *s) const {
                std::tuple<IDType const *, KeyType const *> t {&originalSubscriptionID, &key};
                *s = bytedata_utils::RunSerializer<KeyType>::apply(t);
            }
            bool ParseFromString(std::string const &s) {
                auto res = bytedata_utils::RunDeserializer<std::tuple<IDType,KeyType>>::apply(s);
                if (!res) {
                    return false;
                }
                originalSubscriptionID = std::move(std::get<0>(*res));
                key = std::move(std::get<1>(*res));
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
        
        //The std::string is the account information
        using FacilityInput = std::tuple<
                                    std::string
                                    , std::variant<
                                        Transaction
                                        , Subscription
                                        , Unsubscription
                                    >
                                >;
        using FacilityOutput = SingleLayerWrapper<std::variant<
                                    TransactionResult
                                    , SubscriptionAck
                                    , UnsubscriptionAck
                                    , OneValue
                                >>;
    };

} } } } }

#endif