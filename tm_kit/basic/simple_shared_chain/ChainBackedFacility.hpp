#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_BACKED_FACILITY_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_BACKED_FACILITY_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    //This is a simplistic facility in that it unconditionally writes
    //something to the chain, and then tries to find updates on the chain
    //to these unconditionally written items.
    //If the writing cannot be made unconditional, then this facility is 
    //not suitable.
    //IDAndFinalFlagExtractor must have the following two functions
    //registerID: (ID const &, ChainData const &) => ()
    //extract: (ChainData const &) -> std::optional<std::tuple<ID, bool>> (bool is the final flag)
    //IDAndFinalFlagExtractor is responsible for managing its own locks, if any
    template <class R, class ChainData, class IDAndFinalFlagExtractor>
    inline typename R::template FacilioidConnector<
        ChainData
        , ChainData
    > createChainBackedFacility(
        R &r 
        , ChainWriterOnOrderFacilityWithExternalEffectsFactory<
            typename R::AppType 
            , TrivialChainDataFetchingFolder<ChainData>
            , SimplyPlaceOnChainInputHandler<ChainData>
            , SimplyReturnStateIdleLogic<ChainData, std::optional<ChainData>>
        > chainWriterFactory
        , IDAndFinalFlagExtractor &&idAndFinalFlagExtractor
        , std::string const &prefix
    ) {
        using M = typename R::AppType;
        using Env = typename M::EnvironmentType;

        auto writer = chainWriterFactory();
        r.registerOnOrderFacilityWithExternalEffects(prefix+"/chainWriter", writer);

        class LocalFacility : public M::template AbstractIntegratedVIEOnOrderFacility<ChainData,ChainData,std::optional<ChainData>,typename M::template Key<ChainData>>
        {
        private:
            IDAndFinalFlagExtractor extractor_;
        public:
            LocalFacility(IDAndFinalFlagExtractor &&extractor) : extractor_(std::move(extractor)) {}
            virtual ~LocalFacility() {}
            virtual void handle(typename M::template InnerData<typename M::template Key<ChainData>> &&queryInput) {
                extractor_.registerID(queryInput.timedData.value.id(), queryInput.timedData.value.key());
                this->M::template AbstractImporter<typename M::template Key<ChainData>>::publish(
                    std::move(queryInput)
                );
            }
            virtual void handle(typename M::template InnerData<ChainData> &&data) {
                std::optional<std::tuple<typename Env::IDType, bool>> idAndFinalFlag = extractor_.extract(data.timedData.value);
                if (idAndFinalFlag) {
                    this->M::template AbstractOnOrderFacility<ChainData,ChainData>::publish(
                        data.environment
                        , typename M::template Key<ChainData> {
                            std::move(std::get<0>(*idAndFinalFlag))
                            , std::move(data.timedData.value)
                        }
                        , std::get<1>(*idAndFinalFlag)
                    );
                }
            }
        };

        auto facility = M::vieOnOrderFacility(new LocalFacility(std::move(idAndFinalFlagExtractor)));
        r.registerVIEOnOrderFacility(prefix+"/facility", facility);

        auto discardResult = M::template trivialExporter<typename M::template KeyedData<ChainData,bool>>();
        r.registerExporter(prefix+"/discardResult", discardResult);

        r.placeOrderWithFacilityWithExternalEffects(
            r.vieFacilityAsSource(facility)
            , writer
            , r.exporterAsSink(discardResult)
        );
        r.connect(r.facilityWithExternalEffectsAsSource(writer), r.vieFacilityAsSink(facility));
        return r.vieFacilityConnector(facility);
    }

} } } } }

#endif