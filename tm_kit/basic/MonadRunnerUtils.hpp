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

        template <class A, class B>
        static auto simulatedFacility(
            typename R::template Sink<typename M::template Key<A>> const &inputHandler
            , typename R::template Source<typename M::template KeyedData<A,B>> &&outputProducer
        ) -> typename R::template FacilitioidConnector<A,B> 
        {
            return [&inputHandler,outputProducer=std::move(outputProducer)](
                R &r
                , typename R::template Source<typename M::template Key<A>> &&inputProvider
                , typename R::template Sink<typename M::template KeyedData<A,B>> const &outputReceiver
            ) {
                r.connect(std::move(inputProvider), inputHandler);
                r.connect(std::move(outputProducer), outputReceiver);
            };
        }

        template <class A0, class B0, class A1, class B1>
        static auto wrapFacilitioid(
            typename infra::KleisliUtils<M>::template KleisliFunction<A0,A1> const &forwardKeyTranslator
            , typename infra::KleisliUtils<M>::template KleisliFunction<A1,A0> const &backwardKeyTranslator
            , typename infra::KleisliUtils<M>::template KleisliFunction<B1,B0> const &backwardDataTranslator
            , typename R::template FacilitioidConnector<A1,B1> const &innerFacilitioid
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<A0,B0>
        {
            return 
            [
                forwardKeyTranslator
                , backwardKeyTranslator
                , backwardDataTranslator
                , &innerFacilitioid
                , prefix
            ](
                R &r
                , typename R::template Source<typename M::template Key<A0>> &&inputProvider
                , typename R::template Sink<typename M::template KeyedData<A0,B0>> const &outputReceiver
            ) {
                auto forwardKeyTrans = M::template kleisli<M::template Key<A0>>(
                    CommonFlowUtilComponents<M>::template withKey<A0>(std::move(forwardKeyTranslator))
                );
                r.registerAction(prefix+"/forwardKeyTranslator", forwardKeyTrans);
                r.connect(std::move(inputProvider), r.actionAsSink(forwardKeyTrans));
                auto backwardTrans = M::template kleisli<typename M::template KeyedData<A1,B1>>(
                    [k=std::move(backwardKeyTranslator),d=std::move(backwardDataTranslator)](
                        typename M::template InnerData<typename M::template KeyedData<A1,B1>> &&x
                    ) -> typename M::template Data<typename M::template KeyedData<A0,B0>> {
                        typename M::template InnerData<A1> a1 {
                            x.env, {x.timedData.timePoint, x.timedData.value.key.key(), x.timedData.finalFlag}
                        };
                        auto a0 = k(std::move(a1));
                        if (!a0) {
                            return std::nullopt;
                        }
                        typename M::template InnerData<B1> b1 {
                            x.env, {x.timedData.timePoint, std::move(x.timedData.value.data), x.timedData.finalFlag}
                        };
                        auto b0 = d(std::move(b1));
                        if (!b0) {
                            return std::nullopt;
                        }
                        return { typename M::template InnerData<typename M::template KeyedData<A0,B0>> {
                            x.env, {
                                std::max(a0.timedData.timePoint, b0.timedData.timePoint)
                                , typename M::template KeyedData<A0,B0> {
                                    typename M::template Key<A0> {
                                        x.timedData.value.key.id()
                                        , std::move(a0.timedData.value)
                                    }   
                                    , std::move(b0.timedData.value)
                                }
                                , (a0.timedData.finalFlag && b0.timedData.finalFlag)
                            }
                        } };
                    }
                );
                r.registerAction(prefix+"/backwardTranslator", backwardTrans);
                innerFacilitioid(r, r.actionAsSource(forwardKeyTrans), r.actionAsSink(backwardTrans));
            };
        }
    };

} } } }

#endif