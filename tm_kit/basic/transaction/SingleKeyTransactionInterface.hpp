#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_TRANSACTION_INTERFACE_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_TRANSACTION_INTERFACE_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <type_traits>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction {

    template <class KeyType, class DataType>
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
    template <class KeyType, class VersionType, class DataSummaryType, class DataDeltaType>
    struct UpdateAction {
        KeyType key;
        VersionType oldVersion;
        DataSummaryType oldDataSummary;
        DataDeltaType dataDelta;
        void SerializeToString(std::string *s) const {
            std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataDeltaType const *> t {&key, &oldVersion, &oldDataSummary, &dataDelta};
            *s = bytedata_utils::RunSerializer<
                    std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataDeltaType const *>
                >::apply(t);
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<
                        std::tuple<KeyType,VersionType,DataSummaryType,DataDeltaType>
                        >::apply(s);
            if (!res) {
                return false;
            }
            key = std::move(std::get<0>(*res));
            oldVersion = std::move(std::get<1>(*res));
            oldDataSummary = std::move(std::get<2>(*res));
            dataDelta = std::move(std::get<3>(*res));
            return true;
        }
    };
    template <class KeyType, class VersionType, class DataSummaryType>
    struct DeleteAction {
        KeyType key;
        VersionType oldVersion;
        DataSummaryType oldDataSummary;
        void SerializeToString(std::string *s) const {
            std::tuple<KeyType const *, VersionType const *, DataSummaryType const *> t {&key, &oldVersion, &oldDataSummary};
            *s = bytedata_utils::RunSerializer<
                    std::tuple<KeyType const *, VersionType const *, DataSummaryType const *>
                >::apply(t);
        }
        bool ParseFromString(std::string const &s) {
            auto res = bytedata_utils::RunDeserializer<
                        std::tuple<KeyType,VersionType,DataSummaryType>
                        >::apply(s);
            if (!res) {
                return false;
            }
            key = std::move(std::get<0>(*res));
            oldVersion = std::move(std::get<1>(*res));
            oldDataSummary = std::move(std::get<2>(*res));
            return true;
        }
    };
    template <class KeyType>
    struct Subscription {
        KeyType key;
        void SerializeToString(std::string *s) const {
            *s = bytedata_utils::RunSerializer<KeyType const *>::apply(&key);
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
    template <class IDType, class KeyType>
    struct Unsubscription {
        IDType originalSubscriptionID;
        KeyType key;
        void SerializeToString(std::string *s) const {
            std::tuple<IDType const *, KeyType const *> t {&originalSubscriptionID, &key};
            *s = bytedata_utils::RunSerializer<std::tuple<IDType const *, KeyType const *>>::apply(t);
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

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class IDType
        , class DataSummaryType = DataType
        , class DataDeltaType = DataType
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
        , class DataDeltaType
        , class Cmp
    >
    class SingleKeyTransactionInterface<
        KeyType
        , DataType
        , VersionType
        , IDType
        , DataSummaryType
        , DataDeltaType
        , Cmp
        , std::enable_if_t<
            std::is_default_constructible_v<KeyType>
            && std::is_copy_constructible_v<KeyType>
            && std::is_default_constructible_v<VersionType>
            && std::is_copy_constructible_v<VersionType>
            && std::is_default_constructible_v<DataSummaryType>
            && std::is_copy_constructible_v<DataSummaryType>
            && std::is_default_constructible_v<DataDeltaType>
            && std::is_copy_constructible_v<DataDeltaType>
        >
    > {
    public:
        using InsertAction = transaction::InsertAction<KeyType,DataType>;
        using UpdateAction = transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType>;
        using DeleteAction = transaction::DeleteAction<KeyType,VersionType,DataSummaryType>;

        using Transaction = std::variant<InsertAction,UpdateAction,DeleteAction>;

        using Subscription = transaction::Subscription<KeyType>;
        using Unsubscription = transaction::Unsubscription<IDType,KeyType>;
        
        using OneValue = infra::GroupedVersionedData<
                            KeyType
                            , VersionType
                            , std::optional<DataType>
                            , Cmp
                        >;
        using OneDelta = infra::GroupedVersionedData<
                            KeyType
                            , VersionType
                            , DataDeltaType
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
                                    , OneDelta
                                >>;
    };

} 

namespace bytedata_utils {
    template <class KeyType, class DataType>
    struct RunCBORSerializer<transaction::InsertAction<KeyType,DataType>, void> {
        static std::vector<uint8_t> apply(transaction::InsertAction<KeyType,DataType> const &x) {
            std::tuple<KeyType const *, DataType const *> t {&x.key, &x.data};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *, DataType const *>, 2>
                ::apply(t, {
                    "key", "data"
                });
        }
    };
    template <class KeyType, class DataType>
    struct RunCBORDeserializer<transaction::InsertAction<KeyType,DataType>, void> {
        static std::optional<std::tuple<transaction::InsertAction<KeyType,DataType>,size_t>> apply(std::string const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType, DataType>,2>
                ::apply(data, start, {
                    "key", "data"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::InsertAction<KeyType,DataType>,size_t> {
                transaction::InsertAction<KeyType,DataType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , std::move(std::get<1>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class KeyType, class VersionType, class DataSummaryType, class DataDeltaType>
    struct RunCBORSerializer<transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType>, void> {
        static std::vector<uint8_t> apply(transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType> const &x) {
            std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataDeltaType const *> t {&x.key, &x.oldVersion, &x.oldDataSummary, &x.dataDelta};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *, VersionType const *, DataSummaryType const *, DataDeltaType const *>, 4>
                ::apply(t, {
                    "key", "old_version", "old_data_summary", "data_delta"
                });
        }
    };
    template <class KeyType, class VersionType, class DataSummaryType, class DataDeltaType>
    struct RunCBORDeserializer<transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType>, void> {
        static std::optional<std::tuple<transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType>,size_t>> apply(std::string const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType, VersionType, DataSummaryType, DataDeltaType>, 4>
                ::apply(data, start, {
                    "key", "old_version", "old_data_summary", "data_delta"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType>,size_t> {
                transaction::UpdateAction<KeyType,VersionType,DataSummaryType,DataDeltaType> {
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
    struct RunCBORSerializer<transaction::DeleteAction<KeyType,VersionType,DataSummaryType>, void> {
        static std::vector<uint8_t> apply(transaction::DeleteAction<KeyType,VersionType,DataSummaryType> const &x) {
            std::tuple<KeyType const *, VersionType const *, DataSummaryType const *> t {&x.key, &x.oldVersion, &x.oldDataSummary};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *, VersionType const *, DataSummaryType const *>, 3>
                ::apply(t, {
                    "key", "old_version", "old_data_summary"
                });
        }
    };
    template <class KeyType, class VersionType, class DataSummaryType>
    struct RunCBORDeserializer<transaction::DeleteAction<KeyType,VersionType,DataSummaryType>, void> {
        static std::optional<std::tuple<transaction::DeleteAction<KeyType,VersionType,DataSummaryType>,size_t>> apply(std::string const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType, VersionType, DataSummaryType>, 3>
                ::apply(data, start, {
                    "key", "old_version", "old_data_summary"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::DeleteAction<KeyType,VersionType,DataSummaryType>,size_t> {
                transaction::DeleteAction<KeyType,VersionType,DataSummaryType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , std::move(std::get<1>(std::get<0>(*t)))
                    , std::move(std::get<2>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class KeyType>
    struct RunCBORSerializer<transaction::Subscription<KeyType>, void> {
        static std::vector<uint8_t> apply(transaction::Subscription<KeyType> const &x) {
            std::tuple<KeyType const *> t {&x.key};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<KeyType const *>, 1>
                ::apply(t, {
                    "key"
                });
        }
    };
    template <class KeyType>
    struct RunCBORDeserializer<transaction::Subscription<KeyType>, void> {
        static std::optional<std::tuple<transaction::Subscription<KeyType>,size_t>> apply(std::string const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<KeyType>, 1>
                ::apply(data, start, {
                    "key"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::Subscription<KeyType>,size_t> {
                transaction::Subscription<KeyType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
    template <class IDType, class KeyType>
    struct RunCBORSerializer<transaction::Unsubscription<IDType,KeyType>, void> {
        static std::vector<uint8_t> apply(transaction::Unsubscription<IDType,KeyType> const &x) {
            std::tuple<IDType const *, KeyType const *> t {&x.originalSubscriptionID, &x.key};
            return bytedata_utils::RunCBORSerializerWithNameList<std::tuple<IDType const *, KeyType const *>, 2>
                ::apply(t, {
                    "original_subscription_id", "key"
                });
        }
    };
    template <class IDType, class KeyType>
    struct RunCBORDeserializer<transaction::Unsubscription<IDType,KeyType>, void> {
        static std::optional<std::tuple<transaction::Unsubscription<IDType,KeyType>,size_t>> apply(std::string const &data, size_t start) {
            auto t = bytedata_utils::RunCBORDeserializerWithNameList<std::tuple<IDType, KeyType>, 2>
                ::apply(data, start, {
                    "original_subscription_id", "key"
                });
            if (!t) {
                return std::nullopt;
            }
            return std::tuple<transaction::Unsubscription<IDType,KeyType>,size_t> {
                transaction::Unsubscription<IDType,KeyType> {
                    std::move(std::get<0>(std::get<0>(*t)))
                    , std::move(std::get<1>(std::get<0>(*t)))
                }
                , std::get<1>(*t)
            };
        }
    };
}

} } } }

#endif