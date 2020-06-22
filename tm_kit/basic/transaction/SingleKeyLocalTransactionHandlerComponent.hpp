#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_TRANSACTION_HANDLER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_LOCAL_TRANSACTION_HANDLER_COMPONENT_HPP_

#include <tm_kit/basic/transaction/SingleKeyTransactionInterface.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class KeyType
        , class DataType
        , class DataDeltaType = DataType
    >
    class SingleKeyLocalTransactionHandlerComponent {
    public:
        virtual ~SingleKeyLocalTransactionHandlerComponent() {}
        virtual RequestDecision
            handleInsert(
                std::string const &account
                , KeyType const &key
                , DataType const &data
                , bool ignoreConsistencyCheckAsMuchAsPossible
            )
            = 0;
        virtual RequestDecision
            handleUpdate(
                std::string const &account
                , KeyType const &key
                , DataDeltaType const &dataDelta
                , bool ignoreConsistencyCheckAsMuchAsPossible
            )
            = 0;
        virtual RequestDecision
            handleDelete(
                std::string const &account
                , KeyType const &key
                , bool ignoreConsistencyCheckAsMuchAsPossible
            )
            = 0;
        virtual std::vector<std::tuple<KeyType,DataType>> loadInitialData() = 0;
    };

    template <
        class KeyType
        , class DataType
        , class DataDeltaType = DataType
    >
    class ReadOnlySingleKeyLocalTransactionHandlerComponent 
        :
        public SingleKeyLocalTransactionHandlerComponent<KeyType,DataType>
    {
    public:
        virtual ~ReadOnlySingleKeyLocalTransactionHandlerComponent() {}
        virtual RequestDecision
            handleInsert(
                std::string const &account
                , KeyType const &key
                , DataType const &data
                , bool ignoreConsistencyCheckAsMuchAsPossible
            ) override final {
            return RequestDecision::FailurePermission;
        }
        virtual RequestDecision
            handleUpdate(
                std::string const &account
                , KeyType const &key
                , DataDeltaType const &dataDelta
                , bool ignoreConsistencyCheckAsMuchAsPossible
            ) override final {
            return RequestDecision::FailurePermission;
        }
        virtual RequestDecision
            handleDelete(
                std::string const &account
                , KeyType const &key
                , bool ignoreConsistencyCheckAsMuchAsPossible
            ) override final {
            return RequestDecision::FailurePermission;
        }
    };

    template <
        class KeyType
        , class DataType
        , class DataDeltaType = DataType
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
                , DataDeltaType const &data
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