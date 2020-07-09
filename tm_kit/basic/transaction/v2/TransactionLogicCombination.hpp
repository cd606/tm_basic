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

    template <class R, class DI, class DataStoreUpdater>
    auto subscriptionLogicCombination(
        R &r
        , std::string const &componentPrefix
        , typename R::template Source<typename DI::Update> &&updateSource
        , TransactionDataStorePtr<DI,typename DataStoreUpdater::KeyHash,R::MonadType::PossiblyMultiThreaded> const &dataStorePtr
    ) -> typename R::template LocalOnOrderFacilityPtr<
            typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::InputWithAccountInfo
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Output
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::SubscriptionUpdate
        > {
        static_assert(
            ((DataStoreUpdater::IsMutexProtected == R::MonadType::PossiblyMultiThreaded)
            && std::is_same_v<typename DataStoreUpdater::DataStreamInterfaceType, DI>)
            , "DataStoreUpdater must be compatible with R and DI"
        );

        using M = typename R::MonadType;

        auto subscriptionFacility = M::template localOnOrderFacility<
            typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::InputWithAccountInfo
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Output
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::SubscriptionUpdate
        >(
            new GeneralSubscriber<M, DI, typename DataStoreUpdater::KeyHash>(dataStorePtr)
        );
        auto dataStoreUpdater = M::template pureExporter<typename DI::Update>(DataStoreUpdater {dataStorePtr});
        
        r.registerLocalOnOrderFacility(
            componentPrefix+"/subscription_handler"
            , subscriptionFacility
        );
        r.registerExporter(
            componentPrefix+"/data_store_updater"
            , dataStoreUpdater
        );
        r.connect(
            updateSource.clone()
            , r.localFacilityAsSink(subscriptionFacility)
        );
        r.connect(
            updateSource.clone()
            , r.exporterAsSink(dataStoreUpdater)
        );
        r.preservePointer(dataStorePtr);
        r.markStateSharing(dataStoreUpdater, subscriptionFacility, componentPrefix+"/data_store");
    
        return subscriptionFacility;
    }
   
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
    ) -> TransactionLogicCombinationResult<R, TI, DI, typename DataStoreUpdater::KeyHash> {
        using M = typename R::MonadType;

        auto transactionFacility = M::template onOrderFacilityWithExternalEffects<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
            , typename DI::Update
        >(
            transactionFacilityImpl
            , new typename DataStreamImporterTypeResolver<M,DI>::Importer
        );

        r.registerOnOrderFacilityWithExternalEffects(
            componentPrefix+"/transaction_handler"
            , transactionFacility
        );

        auto subscriptionFacility = subscriptionLogicCombination<R,DI,DataStoreUpdater>(
            r
            , componentPrefix
            , r.facilityWithExternalEffectsAsSource(transactionFacility)
            , transactionFacilityImpl->dataStorePtr()
        );

        r.markStateSharing(transactionFacility, subscriptionFacility, componentPrefix+"/data_store");
        
        return {transactionFacility, subscriptionFacility};
    }

} } 

} } } }

#endif