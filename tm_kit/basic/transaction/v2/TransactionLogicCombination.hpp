#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_LOGIC_COMBINATION_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_LOGIC_COMBINATION_HPP_

#include <tm_kit/basic/transaction/v2/TransactionInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/GeneralSubscriber.hpp>
#include <tm_kit/basic/transaction/v2/DefaultUtilities.hpp>
#include <tm_kit/basic/transaction/v2/TransactionFacility.hpp>
#include <tm_kit/basic/transaction/v2/RealTimeDataStreamImporter.hpp>
#include <tm_kit/basic/transaction/v2/SinglePassIterationDataStreamImporter.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDeltaMerger.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 

namespace transaction { namespace v2 {

    template <class M, class DI>
    struct DataStreamImporterTypeResolver {
    };
    template <class Env, class DI>
    struct DataStreamImporterTypeResolver<infra::RealTimeMonad<Env>, DI> {
        using Importer = real_time::DataStreamImporter<Env, DI>;
    };
    template <class Env, class DI>
    struct DataStreamImporterTypeResolver<infra::SinglePassIterationMonad<Env>, DI> {
        using Importer = single_pass_iteration::DataStreamImporter<Env, DI>;
    };
    
    template <class R, class TI, class DI, class KeyHash=std::hash<typename DI::Key>, class Enable=void>
    struct TransactionLogicCombinationResult {};

    template <class R, class TI, class DI, class KeyHash>
    struct TransactionLogicCombinationResult<
        R, TI, DI, KeyHash
        , std::enable_if_t<
            std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
            && std::is_same_v<typename TI::Key, typename DI::Key>
            && std::is_same_v<typename TI::Version, typename DI::Version>
            && std::is_same_v<typename TI::Data, typename DI::Data>
        >
    > {
        typename R::template OnOrderFacilityWithExternalEffectsPtr<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
            , typename DI::Update
        > transactionFacility;
        typename R::template LocalOnOrderFacilityPtr<
            typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::InputWithAccountInfo
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Output
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::SubscriptionUpdate
        > subscriptionFacility;
    };


    template <class R, class TI, class DI, class DataStoreUpdater>
    auto transactionLogicCombination(
        R &r
        , std::string const &componentPrefix
        , ITransactionFacility<typename R::MonadType, TI, DI, typename DataStoreUpdater::KeyHash> *transactionFacilityImpl
        , IGeneralSubscriber<typename R::MonadType, DI> *subscriptionFacilityImpl
    ) -> TransactionLogicCombinationResult<R, TI, DI, typename DataStoreUpdater::KeyHash> {
        static_assert(
            ((DataStoreUpdater::IsMutexProtected == R::MonadType::PossiblyMultiThreaded)
            && std::is_same_v<typename DataStoreUpdater::DataStreamInterfaceType, DI>)
            , "DataStoreUpdater must be compatible with R and DI"
        );

        using M = typename R::MonadType;

        auto transactionFacility = M::template onOrderFacilityWithExternalEffects<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
            , typename DI::Update
        >(
            transactionFacilityImpl
            , new typename DataStreamImporterTypeResolver<M,DI>::Importer
        );
        auto subscriptionFacility = M::template localOnOrderFacility<
            typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::InputWithAccountInfo
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Output
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::SubscriptionUpdate
        >(
            subscriptionFacilityImpl
        );
        auto dataStoreUpdater = M::template pureExporter<typename DI::Update>(DataStoreUpdater {transactionFacilityImpl->dataStorePtr()});
        
        r.registerOnOrderFacilityWithExternalEffects(
            componentPrefix+"_transaction_handler"
            , transactionFacility
        );
        r.registerLocalOnOrderFacility(
            componentPrefix+"_subscription_handler"
            , subscriptionFacility
        );
        r.registerExporter(
            componentPrefix+"_data_store_updater"
            , dataStoreUpdater
        );
        r.connect(
            r.facilityWithExternalEffectsAsSource(transactionFacility)
            , r.localFacilityAsSink(subscriptionFacility)
        );
        r.connect(
            r.facilityWithExternalEffectsAsSource(transactionFacility)
            , r.exporterAsSink(dataStoreUpdater)
        );
        r.preservePointer(transactionFacilityImpl->dataStorePtr());
        r.markStateSharing(transactionFacility, subscriptionFacility, componentPrefix+"_data_store");
        r.markStateSharing(transactionFacility, dataStoreUpdater, componentPrefix+"_data_store");
        r.markStateSharing(dataStoreUpdater, subscriptionFacility, componentPrefix+"_data_store");
    
        return {transactionFacility, subscriptionFacility};
    }

} } 

} } } }

#endif