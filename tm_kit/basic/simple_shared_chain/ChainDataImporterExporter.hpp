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
            , void
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

    //This is ONLY suitable if the writing of ChainData is unconditional,
    //if the writing of ChainData is conditional on some chain state, then 
    //the full ChainWriter should be used
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

    template <class R, template <class M> class ChainCreator, class ChainData>
    inline typename R::template Source<ChainData> setupChainTranscriber(
        R &r 
        , ChainCreator<typename R::AppType> &chainCreator
        , std::string const &inputChainDescriptor
        , std::string const &outputChainDescriptor
        , std::function<typename R::AppType::TimePoint(ChainData const &)> const &timestampExtractor
        , std::string const &prefix
    ) {
        class AugmentedTrivialChainDataFolder : public TrivialChainDataFetchingFolder<ChainData> {
        private:
            std::function<typename R::AppType::TimePoint(ChainData const &)> timestampExtractor_;
        public:
            AugmentedTrivialChainDataFolder(std::function<typename R::AppType::TimePoint(ChainData const &)> const &timestampExtractor) : timestampExtractor_(timestampExtractor) {}
            typename R::AppType::TimePoint extractTime(std::optional<ChainData> const &st) {
                if (st) {
                    return timestampExtractor_(*st);
                } else {
                    return typename R::AppType::TimePoint {};
                }
            }
        };

        auto chainDataSource = createChainDataSource<
            R, ChainData
        >(
            r 
            , chainCreator.template readerFactory<
                ChainData
                , AugmentedTrivialChainDataFolder
            >(
                r.environment()
                , inputChainDescriptor
                , ChainPollingPolicy()
                , AugmentedTrivialChainDataFolder(timestampExtractor)
            )
            , prefix+"/input_chain"
        );

        auto chainDataSink = createChainDataSink<
            R, ChainData
        >(
            r 
            , chainCreator.template writerFactory<
                ChainData
                , EmptyStateChainFolder
                , SimplyPlaceOnChainInputHandler<ChainData>
            >(
                r.environment()
                , outputChainDescriptor
            )
            , prefix+"/output_chain"
        );

        r.connect(chainDataSource.clone(), chainDataSink);

        return chainDataSource.clone();
    }

} } } } }

#endif