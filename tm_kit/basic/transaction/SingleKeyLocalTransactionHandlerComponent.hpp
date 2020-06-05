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
        //All the false results will be interpreted as permission failure
        //since the assumption is that preconditions have been checked in 
        //the broker already
        virtual bool
            handleInsert(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            )
            = 0;
        virtual bool
            handleUpdate(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            )
            = 0;
        virtual bool
            handleDelete(
                std::string const &account
                , KeyType const &key
            )
            = 0;
        virtual std::vector<std::tuple<KeyType,DataType>> loadInitialData() = 0;
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
        virtual bool
            handleInsert(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            ) final {
            return false;
        }
        virtual bool
            handleUpdate(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            ) final {
            return false;
        }
        virtual bool
            handleDelete(
                std::string const &account
                , KeyType const &key
            ) final {
            return false;
        }
    };

} } } } }

#endif