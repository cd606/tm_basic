#ifndef TM_KIT_BASIC_MONAD_RUNNER_UTILS_HPP_
#define TM_KIT_BASIC_MONAD_RUNNER_UTILS_HPP_

#include <tm_kit/infra/KleisliUtils.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/basic/CommonFlowUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class R>
    class MonadRunnerUtilComponents {
    private:
        using M = typename R::MonadType;
        using TheEnvironment = typename M::EnvironmentType;
        using CU = CommonFlowUtilComponents<M>;
    public:
        template <class Input, class Output, class ToSupplyDefaultValue>
        static auto wrapTuple2FacilitioidBySupplyingDefaultValue(
            typename R::template FacilitioidConnector<
                std::tuple<ToSupplyDefaultValue, Input>
                , Output
            > toBeWrapped
            , std::string const &wrapPrefix
            , ToSupplyDefaultValue const &v = ToSupplyDefaultValue()
        ) -> typename R::template FacilitioidConnector<
                Input
                , Output
            >
        {
            ToSupplyDefaultValue vCopy = v;
            std::string prefix = wrapPrefix;
            return [toBeWrapped,vCopy,prefix](
                R &r
                , typename R::template Source<typename M::template Key<Input>> &&source
                , typename R::template Sink<typename M::template KeyedData<Input,Output>> const &sink) 
            {
                auto convertKey = M::template kleisli<typename M::template Key<Input>>(
                    CU::template withKey<Input>(
                        infra::KleisliUtils<M>::template liftPure<Input>(
                            [vCopy](Input &&x) -> std::tuple<ToSupplyDefaultValue, Input> {
                                return {vCopy, std::move(x)};
                            }
                        )
                    )
                );
                auto convertOutput = M::template liftPure<typename M::template KeyedData<std::tuple<ToSupplyDefaultValue,Input>,Output>>(                   
                    [](typename M::template KeyedData<std::tuple<ToSupplyDefaultValue,Input>,Output> &&x) 
                        -> typename M::template KeyedData<Input,Output> 
                    {
                        return {{x.key.id(), std::get<1>(x.key.key())}, std::move(x.data)};
                    }
                );
                toBeWrapped(
                    r
                    , r.execute(prefix+"/convert_key", convertKey, std::move(source))
                    , r.actionAsSink(prefix+"/convert_output", convertOutput)
                );
                r.connect(r.actionAsSource(convertOutput), sink);
            };
        }

        template <class FacilityWithExternalEffects>
        static typename R::template Source<typename infra::withtime_utils::ImporterTypeInfo<M, FacilityWithExternalEffects>::DataType> importWithTrigger(
            R &r
            , typename R::template Source<typename M::template Key<typename infra::withtime_utils::OnOrderFacilityTypeInfo<M, FacilityWithExternalEffects>::InputType>> &&trigger 
            , std::shared_ptr<FacilityWithExternalEffects> const &triggerHandler
            , std::optional<typename R::template Sink<typename M::template KeyedData<typename infra::withtime_utils::OnOrderFacilityTypeInfo<M, FacilityWithExternalEffects>::InputType, typename infra::withtime_utils::OnOrderFacilityTypeInfo<M, FacilityWithExternalEffects>::OutputType>>> const &triggerResponseProcessor = std::nullopt
            , std::string const &triggerHandlerName = ""
        ) {
            if (triggerHandlerName != "") {
                r.registerOnOrderFacilityWithExternalEffects(triggerHandlerName, triggerHandler);
            }
            if (triggerResponseProcessor) {
                r.placeOrderWithFacilityWithExternalEffects(
                    std::move(trigger)
                    , triggerHandler
                    , *triggerResponseProcessor
                );
            } else {
                r.placeOrderWithFacilityWithExternalEffectsAndForget(
                    std::move(trigger)
                    , triggerHandler
                );
            }
            return r.facilityWithExternalEffectsAsSource(triggerHandler);            
        }
    };

} } } }

#endif