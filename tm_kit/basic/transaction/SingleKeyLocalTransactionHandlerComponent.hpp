#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_TRANSACTION_HANDLER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_TRANSACTION_HANDLER_COMPONENT_HPP_

#include <tm_kit/basic/transaction/SingleKeyTransactionInterface.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class IDType
        , class DataSummaryType = DataType
        , class Cmp = std::less<VersionType>
        , class CheckSummary = std::equal_to<DataType>
    >
    class SingleKeyLocalTransactionHandlerComponent {
    public:
        using TI = SingleKeyTransactionInterface<KeyType,DataType,VersionType,IDType,DataSummaryType,Cmp,CheckSummary>;
        virtual ~SingleKeyLocalTransactionHandlerComponent() {}
        virtual typename TI::TransactionResult
            handleTransaction(
                std::string const &account
                , typename TI::Transaction const &transaction
            )
            = 0;
        virtual std::vector<std::tuple<KeyType,ValueType>> loadInitialData() = 0;
    };

    template <
        class KeyType
        , class DataType
        , class VersionType
        , class IDType
        , class DataSummaryType = DataType
        , class Cmp = std::less<VersionType>
        , class CheckSummary = std::equal_to<DataType>
    >
    class ReadOnlySingleKeyLocalTransactionHandlerComponent 
        :
        public SingleKeyLocalTransactionHandlerComponent<KeyType,DataType,VersionType,IDType,DataSummaryType,Cmp,CheckSummary>
    {
    public:
        using TI = SingleKeyTransactionInterface<KeyType,DataType,VersionType,IDType,DataSummaryType,Cmp,CheckSummary>;
        virtual ~ReadOnlySingleKeyLocalTransactionHandlerComponent() {}
        virtual typename TI::TransactionResult
            handleTransaction(
                std::string const &account
                , typename TI::Transaction const &transaction
            ) final {
            return TI::TransactionFailurePermission {};
        }
    };

} } } } }

#endif