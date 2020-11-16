#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_DATA_IMPORTER_EXPORTER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_DATA_IMPORTER_EXPORTER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/KleisliUtils.hpp>
#include <tm_kit/basic/CommonFlowUtils.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    template <class R, class ChainData>
    inline typename R::template Source<ChainData> createChainDataSource(
        R &r 
        , ChainReaderImporterFactory<
            typename R::AppType 
            , TrivialChainDataFetchingFolder<ChainData>
        > chainReaderFactory
        , std::string const &prefix
    ) {
        using M = typename R::AppType;
        using Env = typename M::EnvironmentType;

        auto reader = chainReaderFactory();
        r.registerImporter(prefix+"/chainReader", reader);

        auto simpleFilter = infra::KleisliUtils<M>::action(
            basic::CommonFlowUtilComponents<M>::template filterOnOptional<ChainData>()
        );
        return r.execute(
            prefix+"/simpleFilter", simpleFilter 
            , r.importItem(reader)
        );
    }

    template <class R, class ChainData>
    inline typename R::template Sink<ChainData> createChainDataSink(
        R &r 
        , ChainWriterOnOrderFacilityFactory<
            typename R::AppType 
            , EmptyStateChainFolder
            , SimplyPlaceOnChainInputHandler<ChainData>
        > chainWriterFactory
        , std::string const &prefix
    ) {
        using M = typename R::AppType;
        using Env = typename M::EnvironmentType;

        auto writer = chainWriterFactory();
        r.registerOnOrderFacility(prefix+"/chainWriter", writer);

        auto keyify = infra::KleisliUtils<M>::action(
            basic::CommonFlowUtilComponents<M>::template keyify<ChainData>()
        );
        r.registerAction(prefix+"/keyify", keyify);

        auto discardResult = M::template trivialExporter<typename M::template KeyedData<ChainData,bool>>();
        r.registerExporter(prefix+"/discardResult", discardResult);

        r.placeOrderWithFacility(r.actionAsSource(keyify), writer, r.exporterAsSink(discardResult));

        return r.actionAsSink(keyify);
    }

} } } } }

#endif