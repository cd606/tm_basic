#ifndef TM_KIT_BASIC_TRANSACTION_TRANSACTION_INTERFACE_HPP_
#define TM_KIT_BASIC_TRANSACTION_TRANSACTION_INTERFACE_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <type_traits>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class DataSummaryType = DataType
        , class Cmp = std::less<VersionType>
        , class Enable = void 
    >
    class TransactionInterface {};

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class DataSummaryType
        , class Cmp
    >
    class TransactionInterface<
        KeyType
        , DataType
        , VersionType
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
        using InsertAction = std::tuple<ConstType<0>,KeyType,DataType>;
        using UpdateAction = std::tuple<ConstType<1>,KeyType,DataType>;
        using DeleteAction = std::tuple<ConstType<2>,KeyType>;

        using Modify = std::vector<std::variant<
                            InsertAction
                            , UpdateAction
                            , DeleteAction
                        >>;

        //nullopt means entry is not present
        using EntrySpec = std::tuple<KeyType,std::optional<std::tuple<VersionType,DataSummaryType>>>;

        using PreCondition = std::vector<EntrySpec>;
        using Transaction = std::tuple<PreCondition, Modify>;

        using Query = std::tuple<ConstType<3>,std::vector<KeyType>>;
        using Unsubscribe = std::tuple<ConstType<4>,std::vector<KeyType>>;
        
        using TransactionSuccess = ConstType<100>;
        using TransactionFailure = std::tuple<ConstType<101>,std::vector<EntrySpec>>;
        using QueryAck = ConstType<102>;
        using UnsubscribeAck = ConstType<103>;
        using QueryUpdate = std::tuple<ConstType<104>,std::vector<infra::GroupedVersionedData<KeyType,VersionType,DataType,Cmp>>>;

        using FacilityInput = SingleLayerWrapper<std::variant<Transaction,Query,Unsubscribe>>;
        using FacilityOutput = SingleLayerWrapper<std::variant<
                                    TransactionSuccess
                                    , TransactionFailure
                                    , QueryAck
                                    , UnsubscribeAck
                                    , QueryUpdate
                                >>;
    };

} } } } }

#endif