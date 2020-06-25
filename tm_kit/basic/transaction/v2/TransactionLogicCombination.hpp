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
    
    template <class R, class TI, class DI, class Enable=void>
    struct TransactionLogicCombinationResult {};

    template <class R, class TI, class DI>
    struct TransactionLogicCombinationResult<
        R, TI, DI
        , std::enable_if_t<
            std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
            && std::is_same_v<typename TI::Key, typename DI::Key>
            && std::is_same_v<typename TI::Version, typename DI::Version>
            && std::is_same_v<typename TI::Data, typename DI::Data>
        >
    > {
        typename R::OnOrderFacilityWithExternalEffectsPtr<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
            , typename DI::Update
        > transactionFacility;
        typename R::LocalOnOrderFacilityPtr<
            typename GeneralSubscriberTypes<typename R::MonadType, DI>::Input
            , typename GeneralSubscriberTypes<typename R::MonadType, DI>::Output
            , typename GeneralSubscriberTypes<typename R::MonadType, DI>::SubscriptionUpdate
        > subscriptionFacility;
    };


    template <class R, class TI, class DI, class DataStoreUpdater>
    auto transactionLogicCombination(
        R &r
        , std::string const &componentPrefix
        , ITransactionFacility<typename R::MonadType, TI> *transactionFacilityImpl
        , typename GeneralSubscriberTypes<typename R::MonadType, DI>::IGeneralSubscriber *subscriptionFacilityImpl
    ) -> TransactionLogicCombinationResult<R, TI, DI> {
        using M = typenaem R::MonadType;

        auto transactionFacility = M::onOrderFacilityWithExternalEffects<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
            , typename DI::Update
        >(
            transactionFacilityImpl
            , new typename DataStreamImporterTypeResolver<M,DI>::Importer
        );
        auto subscriptionFacility = M::localOnOrderFacility<
            typename GeneralSubscriberTypes<typename R::MonadType, DI>::Input
            , typename GeneralSubscriberTypes<typename R::MonadType, DI>::Output
            , typename GeneralSubscriberTypes<typename R::MonadType, DI>::SubscriptionUpdate
        >(
            subscriptionFacilityImpl
        );
        auto dataStoreUpdater = M::simpleExporter<typename DI::Update>(DataStoreUpdater {transactionFacilityImpl->dataStorePtr()});
        
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
            M::onOrderFacilityWithExternalEffectsAsSource(transactionFacility)
            , M::localOnOrderFacilityAsSink(subscriptionFacility)
        );
        r.connect(
            M::onOrderFacilityWithExternalEffectsAsSource(transactionFacility)
            , M::exporterAsSink(dataStoreUpdater)
        );
        r.preservePointer(transactionFacility->dataStorePtr());
        r.markStateSharing(transactionFacility, subscriptionFacility, componentPrefix+"_data_store");
        r.markStateSharing(transactionFacility, dataStoreUpdater, componentPrefix+"_data_store");
        r.markStateSharing(dataStoreUpdater, subscriptionFacility, componentPrefix+"_data_store");
    
        return {transactionFacility, subscriptionFacility};
    }

} } 

} } } }

#endif