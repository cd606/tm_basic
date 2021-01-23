#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_SELF_CONTAINED_CHAIN_MODIFIER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_SELF_CONTAINED_CHAIN_MODIFIER_HPP_

#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/CommonFlowUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    template <class IdleLogic, typename = std::enable_if_t<std::is_same_v<VoidStruct, typename IdleLogic::OffChainUpdateType>>>
    class SelfContainedChainModifierInputHandler {
    private:
        IdleLogic logic_;
    public:
        using InputType = VoidStruct;
        using ResponseType = VoidStruct;

        SelfContainedChainModifierInputHandler(IdleLogic &&logic) : logic_(logic) {}
        static void initialize(void *, void *) {}
        template <class Env, class Chain, class StateType>
        auto handleInput(Env *env, Chain *chain, infra::WithTime<infra::Key<VoidStruct, Env>, typename Env::TimePointType> const &, StateType const &state) 
        -> std::tuple<VoidStruct, decltype(std::get<1>(logic_.work(env, chain, state)))> {
            return {
                VoidStruct {}
                , std::move(std::get<1>(logic_.work(env, chain, state)))
            };
        }
    };
    template <class R, template <class M> class ChainCreator, class ChainData, class ChainItemFolder, class IdleLogic>
    void createSelfContainedChainModifier(
        R &r 
        , ChainCreator<typename R::AppType> &chainCreator
        , std::string const &chainDescriptor
        , std::string const &prefix
        , std::optional<typename R::template Source<VoidStruct>> drivingUpdate = std::nullopt
        , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy {}
        , ChainItemFolder &&chainItemFolder = ChainItemFolder {}
        , IdleLogic &&idleLogic = IdleLogic {}
    ) {
        if (drivingUpdate) {
            auto factory = chainCreator.template writerFactory<
                ChainData 
                , ChainItemFolder
                , SelfContainedChainModifierInputHandler<IdleLogic>
                , void
            >(
                r.environment()
                , chainDescriptor
                , pollingPolicy
                , std::move(chainItemFolder)
                , SelfContainedChainModifierInputHandler<IdleLogic>(std::move(idleLogic))
            );
            auto writer = factory();
            r.registerOnOrderFacility(prefix+"/facility", writer);
            auto discard = R::AppType::template trivialExporter<
                typename R::AppType::template KeyedData<VoidStruct, VoidStruct>
            >();
            r.registerExporter(prefix+"/discard", discard);
            auto keyify = R::AppType::template kleisli<VoidStruct>(CommonFlowUtilComponents<typename R::AppType>::template keyify<VoidStruct>());
            r.registerAction(prefix+"/keyify", keyify);
            r.placeOrderWithFacility(r.execute(keyify, drivingUpdate->clone()), writer, r.exporterAsSink(discard));
        } else {
            auto factory = chainCreator.template writerFactory<
                ChainData 
                , ChainItemFolder
                , EmptyInputHandler<ChainData>
                , IdleLogic
            >(
                r.environment()
                , chainDescriptor
                , pollingPolicy
                , std::move(chainItemFolder)
                , EmptyInputHandler<ChainData> {}
                , std::move(idleLogic)
            );
            auto writer = factory();
            r.registerOnOrderFacilityWithExternalEffects(prefix+"/facility", writer);
            auto discard1 = R::AppType::template trivialExporter<
                typename R::AppType::template KeyedData<VoidStruct, VoidStruct>
            >();
            r.registerExporter(prefix+"/discard1", discard1);
            auto discard2 = R::AppType::template trivialExporter<
                VoidStruct
            >();
            r.registerExporter(prefix+"/discard1", discard1);
            auto emptyKey = R::AppType::template vacuousImporter<typename R::AppType::template Key<VoidStruct>>();
            r.registerImporter(prefix+"/emptyKey", emptyKey);
            r.placeOrderWithFacilityWithExternalEffects(r.importItem(emptyKey), writer, r.exporterAsSink(discard1));
            r.connect(r.facilityWithExternalEffectsAsSource(writer), r.exporterAsSink(discard2));
        }
    }
} } } } } 

#endif