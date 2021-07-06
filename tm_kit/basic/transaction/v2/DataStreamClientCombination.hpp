#ifndef TM_KIT_BASIC_TRANSACTION_V2_DATA_STREAM_CLIENT_COMBINATION_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_DATA_STREAM_CLIENT_COMBINATION_HPP_

#include <tm_kit/basic/transaction/v2/TransactionInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/GeneralSubscriber.hpp>
#include <tm_kit/basic/transaction/v2/DefaultUtilities.hpp>
#include <tm_kit/basic/transaction/v2/TransactionFacility.hpp>
#include <tm_kit/basic/transaction/v2/RealTimeDataStreamImporter.hpp>
#include <tm_kit/basic/transaction/v2/SinglePassIterationDataStreamImporter.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDeltaMerger.hpp>
#include <tm_kit/basic/CommonFlowUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction { namespace v2 {
    template <class R, class DI, class Input=typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Input>
    using BasicDataStreamClientCombinationResult 
        = typename R::template Source<typename R::AppType::template KeyedData<Input, typename DI::FullUpdate>>;

    template <
        class R, class DI, class Input=typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Input
        , class VersionDeltaMerger = TriviallyMerge<typename DI::Version, typename DI::VersionDelta>
        , class DataDeltaMerger = TriviallyMerge<typename DI::Data, typename DI::DataDelta>
        , class KeyHashType = std::hash<typename DI::Key>
    >
    auto basicDataStreamClientCombination(
        R &r
        , std::string const &componentPrefix
        , typename R::template Sourceoid<typename R::AppType::template KeyedData<
            Input
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Output
        >> subscriptionCallbackReceiver
        , std::shared_ptr<TransactionDataStore<DI,KeyHashType,R::AppType::PossiblyMultiThreaded>> *dataStorePtrOutput = nullptr
    ) -> BasicDataStreamClientCombinationResult<R, DI, Input>
    {
        using M = typename R::AppType;
        using GS = GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>;

        auto dataStorePtr = std::make_shared<TransactionDataStore<DI,KeyHashType,M::PossiblyMultiThreaded>>();
        r.preservePointer(dataStorePtr);
        if (dataStorePtrOutput) {
            *dataStorePtrOutput = dataStorePtr;
        }

        using DM = TransactionDeltaMerger<
            DI, true, M::PossiblyMultiThreaded
            , VersionDeltaMerger, DataDeltaMerger, KeyHashType
        >;
        auto deltaMerger = infra::KleisliUtils<M>::template liftPure<typename DI::Update>(DM {dataStorePtr});
        
        auto getOutput = infra::KleisliUtils<M>::template liftMaybe<typename GS::Output>(
            [](typename GS::Output &&o) -> std::optional<typename DI::Update> {
                return std::visit([](auto &&x) -> std::optional<typename DI::Update> {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T,typename DI::Update>) {
                        return std::move(x);
                    } else {
                        return std::nullopt;
                    }
                }, std::move(o.value));
            }
        );

        auto getFullOutput = M::template kleisli<typename M::template KeyedData<Input, typename GS::Output>>(
            CommonFlowUtilComponents<M>::template withKeyAndInput<Input, typename GS::Output>(
                infra::KleisliUtils<M>::template compose(
                    std::move(getOutput), std::move(deltaMerger)
                )
            )
        );
        r.registerAction(componentPrefix+"/getFullOutput", getFullOutput);
        subscriptionCallbackReceiver(r, r.actionAsSink(getFullOutput));
        return r.actionAsSource(getFullOutput);
    }

    template <class R, class DI>
    struct DataStreamClientCombinationResult {
        using M = typename R::AppType;
        using GS = GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>;
        using Input = typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Input;

        typename R::template Source<typename M::template KeyedData<Input, typename DI::FullUpdate>> fullUpdates;
        typename R::template Source<typename M::template KeyedData<Input, typename GS::Output>> rawSubscriptionOutputs;
    };

    template <
        class R, class DI
        , class VersionDeltaMerger = TriviallyMerge<typename DI::Version, typename DI::VersionDelta>
        , class DataDeltaMerger = TriviallyMerge<typename DI::Data, typename DI::DataDelta>
        , class KeyHashType = std::hash<typename DI::Key>
    >
    auto dataStreamClientCombination(
        R &r
        , std::string const &componentPrefix
        , typename R::template FacilitioidConnector<
            typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Input
            , typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Output
        > subscriptionFacility
        , typename R::template Source<
            typename R::AppType::template Key<typename GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>::Input>
        > &&subscriptionKeySource
        , std::shared_ptr<TransactionDataStore<DI,KeyHashType,R::AppType::PossiblyMultiThreaded>> *dataStorePtrOutput = nullptr
    ) -> DataStreamClientCombinationResult<R, DI>
    {
        using M = typename R::AppType;
        using GS = GeneralSubscriberTypes<typename R::EnvironmentType::IDType, DI>;
        using Input = typename GS::Input;

        auto outputMultiplexer = M::template kleisli<typename M::template KeyedData<Input, typename GS::Output>>(
            CommonFlowUtilComponents<M>::template idFunc<typename M::template KeyedData<Input, typename GS::Output>>()
        );
        r.registerAction(componentPrefix+"/outputMultiplexer", outputMultiplexer);

        subscriptionFacility(
            r
            , std::move(subscriptionKeySource)
            , r.actionAsSink(outputMultiplexer)
        );

        auto fullUpdates = basicDataStreamClientCombination<
            R, DI, Input, VersionDeltaMerger, DataDeltaMerger, KeyHashType
        >(r, componentPrefix, r.sourceAsSourceoid(r.actionAsSource(outputMultiplexer)), dataStorePtrOutput);

        return {
            std::move(fullUpdates)
            , r.actionAsSource(outputMultiplexer)
        };
    }

} } } } } }

#endif