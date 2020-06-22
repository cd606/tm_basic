#ifndef TM_KIT_BASIC_TRANSACTION_EXCLUSIVE_SINGLE_KEY_ASYNC_WATCHABLE_TRANSACTION_HANDLER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_EXCLUSIVE_SINGLE_KEY_ASYNC_WATCHABLE_TRANSACTION_HANDLER_COMPONENT_HPP_

#include <tm_kit/infra/VersionedData.hpp>
#include <tm_kit/basic/transaction/SingleKeyTransactionInterface.hpp>

#include <optional>
#include <functional>
#include <string>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {
    
    template <
        class VersionType
        , class KeyType
        , class DataType
        , class DataDeltaType = DataType
        , class Cmp = std::less<VersionType>
    >
    class ExclusiveSingleKeyAsyncWatchableTransactionHandlerComponent 
    {
    public:
        class Callback {
        public:
            virtual ~Callback() {}
            virtual void onValueChange(KeyType const &key, VersionType &&version, std::optional<DataType> &&data) = 0;
        };
        
        virtual ~ExclusiveSingleKeyAsyncWatchableTransactionHandlerComponent() {}

        virtual bool exclusivelyBindTo(Callback *cb) = 0;
        virtual void setUpInitialWatching() = 0;
        virtual void startWatchIfNecessary(std::string const &account, KeyType const &key) = 0;
        virtual void discretionallyStopWatch(std::string const &account, KeyType const &key) = 0;
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
                , infra::VersionedData<VersionType, DataType, Cmp> const &oldData
                , DataDeltaType const &dataDelta
                , bool ignoreConsistencyCheckAsMuchAsPossible
            )
            = 0;
        virtual RequestDecision
            handleDelete(
                std::string const &account
                , KeyType const &key
                , infra::VersionedData<VersionType, DataType, Cmp> const &oldData
                , bool ignoreConsistencyCheckAsMuchAsPossible
            )
            = 0;
    };

} } } } }

#endif