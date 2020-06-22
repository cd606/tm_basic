#ifndef TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_ASYNC_WATCHABLE_TRANSACTION_HANDLER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_SINGLE_KEY_ASYNC_WATCHABLE_TRANSACTION_HANDLER_COMPONENT_HPP_

#include <tm_kit/basic/transaction/SingleKeyTransactionInterface.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class IDType
        , class VersionType
        , class KeyType
        , class DataType
        , class DataSummaryType = DataType
        , class DataDeltaType = DataType
    >
    class SingleKeyAsyncWatchableTransactionHandlerComponent {
    public:
        enum class RequestDecision {
            Success
            , FailurePrecondition
            , FailurePermission
        };
        class Callback {
        public:
            virtual ~Callback() {}
            virtual void onRequestDecision(IDType const &id, RequestDecision decision) = 0;
            virtual void onValueChange(KeyType const &key, VersionType &&version, std::optional<DataType> &&data) = 0;
            virtual void onStopWatch(KeyType const &key) = 0;
        };
        virtual ~SingleKeyAsyncWatchableTransactionHandlerComponent() {}
        virtual void initialize() = 0;
        virtual void
            handleInsert(
                IDType requestID
                , std::string const &account
                , KeyType const &key
                , DataType const &data
                , Callback *callback
            )
            = 0;
        virtual void
            handleUpdate(
                IDType requestID
                , std::string const &account
                , KeyType const &key
                , VersionType const &oldVersion
                , DataSummaryType const &oldDataSummary
                , DataDeltaType const &dataDelta
                , bool forceUpdate
                , Callback *callback
            )
            = 0;
        virtual void
            handleDelete(
                IDType requestID
                , std::string const &account
                , KeyType const &key
                , VersionType const &oldVersion
                , DataSummaryType const &oldDataSummary
                , bool forceDelete
                , Callback *callback
            )
            = 0;
        virtual void
            startWatch(
                std::string const &account
                , KeyType const &key
                , Callback *callback
            )
            = 0;
        virtual void 
            stopWatch(
                KeyType const &key
                , Callback *callback
            )
            = 0;
    };

} } } } }

#endif