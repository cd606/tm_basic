#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_TRANSACTION_HANDLER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_TRANSACTION_HANDLER_COMPONENT_HPP_

#include <tm_kit/basic/transaction/SingleKeyTransactionInterface.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class KeyType
        , class DataType
    >
    class SingleKeyLocalTransactionHandlerComponent {
    public:
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
    >
    class ReadOnlySingleKeyLocalTransactionHandlerComponent 
        :
        public SingleKeyLocalTransactionHandlerComponent<KeyType,DataType>
    {
    public:
        virtual ~ReadOnlySingleKeyLocalTransactionHandlerComponent() {}
        virtual bool
            handleInsert(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            ) override final {
            return false;
        }
        virtual bool
            handleUpdate(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            ) override final {
            return false;
        }
        virtual bool
            handleDelete(
                std::string const &account
                , KeyType const &key
            ) override final {
            return false;
        }
    };

    template <
        class KeyType
        , class DataType
    >
    class TrivialSingleKeyLocalTransactionHandlerComponent 
        :
        public SingleKeyLocalTransactionHandlerComponent<KeyType,DataType>
    {
    public:
        virtual ~TrivialSingleKeyLocalTransactionHandlerComponent() {}
        virtual bool
            handleInsert(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            ) override final {
            return true;
        }
        virtual bool
            handleUpdate(
                std::string const &account
                , KeyType const &key
                , DataType const &data
            ) override final {
            return true;
        }
        virtual bool
            handleDelete(
                std::string const &account
                , KeyType const &key
            ) override final {
            return true;
        }
        virtual std::vector<std::tuple<KeyType,DataType>> loadInitialData() override final {
            return std::vector<std::tuple<KeyType,DataType>> {};
        }
    
    };

} } } } }

#endif