#ifndef TM_KIT_BASIC_APP_RUNNER_UTILS_HPP_
#define TM_KIT_BASIC_APP_RUNNER_UTILS_HPP_

#include <tm_kit/infra/KleisliUtils.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/basic/CommonFlowUtils.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/ForceDifferentType.hpp>
#include <tm_kit/basic/real_time_clock/ClockOnOrderFacility.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockOnOrderFacility.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class R>
    class AppRunnerUtilComponents {
    private:
        using M = typename R::AppType;
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
            auto discardOutput = M::template trivialExporter<typename M::template KeyedData<Input,Output>>();                  
            return [toBeWrapped,prefix,convertKey,convertOutput,discardOutput](
                R &r
                , typename R::template Source<typename M::template Key<Input>> &&source
                , std::optional<typename R::template Sink<typename M::template KeyedData<Input,Output>>> const &sink) 
            {
                static bool insideConnected = false;

                auto convertKeyCopy = convertKey;
                auto convertOutputCopy = convertOutput;
                auto discardOutputCopy = discardOutput;

                if (!insideConnected) {
                    r.registerAction(prefix+"/convert_key", convertKeyCopy);
                    r.registerAction(prefix+"/convert_output", convertOutputCopy);
                    r.registerExporter(prefix+"/discard_output", discardOutputCopy);
                    toBeWrapped(
                        r
                        , r.actionAsSource(convertKeyCopy)
                        , r.actionAsSink(convertOutputCopy)
                    );
                    r.exportItem(discardOutputCopy, r.actionAsSource(convertOutputCopy));
                    insideConnected = true;
                }

                r.execute(convertKeyCopy, std::move(source));
                
                if (sink) {
                    r.connect(r.actionAsSource(convertOutputCopy), *sink);
                }
            };
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
                , std::optional<typename R::template Sink<typename M::template KeyedData<A,B>>> const &outputReceiver
            ) {
                r.connect(std::move(inputProvider), inputHandler);
                if (outputReceiver) {
                    r.connect(std::move(outputProducer), *outputReceiver);
                }
            };
        }

        template <class A, class B>
        static auto simulatedFacility(
            typename R::template Sinkoid<typename M::template Key<A>> const &inputHandler
            , typename R::template Sourceoid<typename M::template KeyedData<A,B>> const &outputProducer
        ) -> typename R::template FacilitioidConnector<A,B> 
        {
            return [inputHandler,outputProducer](
                R &r
                , typename R::template Source<typename M::template Key<A>> &&inputProvider
                , std::optional<typename R::template Sink<typename M::template KeyedData<A,B>>> const &outputReceiver
            ) {
                inputHandler(r, std::move(inputProvider));
                if (outputReceiver) {
                    outputProducer(r, *outputReceiver);
                }
            };
        }

        template <class A0, class B0, class A1, class B1>
        static auto wrapFacilitioidConnector(
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
                , innerFacilitioid
                , prefix
            ](
                R &r
                , typename R::template Source<typename M::template Key<A0>> &&inputProvider
                , std::optional<typename R::template Sink<typename M::template KeyedData<A0,B0>>> const &outputReceiver
            ) {
                auto forwardKeyTrans = M::template kleisli<typename M::template Key<A0>>(
                    CommonFlowUtilComponents<M>::template withKey<A0>(std::move(forwardKeyTranslator))
                );
                r.registerAction(prefix+"/forwardKeyTranslator", forwardKeyTrans);
                r.connect(std::move(inputProvider), r.actionAsSink(forwardKeyTrans));
                if (outputReceiver) {
                    auto backwardTrans = M::template kleisli<typename M::template KeyedData<A1,B1>>(
                        [k=std::move(backwardKeyTranslator),d=std::move(backwardDataTranslator)](
                            typename M::template InnerData<typename M::template KeyedData<A1,B1>> &&x
                        ) -> typename M::template Data<typename M::template KeyedData<A0,B0>> {
                            typename M::template InnerData<A1> a1 {
                                x.environment, {x.timedData.timePoint, x.timedData.value.key.key(), x.timedData.finalFlag}
                            };
                            auto a0 = k(std::move(a1));
                            if (!a0) {
                                return std::nullopt;
                            }
                            typename M::template InnerData<B1> b1 {
                                x.environment, {x.timedData.timePoint, std::move(x.timedData.value.data), x.timedData.finalFlag}
                            };
                            auto b0 = d(std::move(b1));
                            if (!b0) {
                                return std::nullopt;
                            }
                            return { typename M::template InnerData<typename M::template KeyedData<A0,B0>> {
                                x.environment, {
                                    std::max(a0->timedData.timePoint, b0->timedData.timePoint)
                                    , typename M::template KeyedData<A0,B0> {
                                        typename M::template Key<A0> {
                                            x.timedData.value.key.id()
                                            , std::move(a0->timedData.value)
                                        }   
                                        , std::move(b0->timedData.value)
                                    }
                                    , (a0->timedData.finalFlag && b0->timedData.finalFlag)
                                }
                            } };
                        }
                    );
                    r.registerAction(prefix+"/backwardTranslator", backwardTrans);
                    innerFacilitioid(r, r.actionAsSource(forwardKeyTrans), r.actionAsSink(backwardTrans));
                    r.connect(r.actionAsSource(backwardTrans), *outputReceiver);
                } else {
                    innerFacilitioid(r, r.actionAsSource(forwardKeyTrans), std::nullopt);
                }
            };
        }
        template <class A0, class B0, class B1>
        static auto wrapFacilitioidConnectorBackOnly(
            typename infra::KleisliUtils<M>::template KleisliFunction<B1,B0> const &backwardDataTranslator
            , typename R::template FacilitioidConnector<A0,B1> const &innerFacilitioid
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<A0,B0>
        {
            return 
            [
                backwardDataTranslator
                , innerFacilitioid
                , prefix
            ](
                R &r
                , typename R::template Source<typename M::template Key<A0>> &&inputProvider
                , std::optional<typename R::template Sink<typename M::template KeyedData<A0,B0>>> const &outputReceiver
            ) {
                if (outputReceiver) {
                    auto backwardTrans = M::template kleisli<typename M::template KeyedData<A0,B1>>(
                        [d=std::move(backwardDataTranslator)](
                            typename M::template InnerData<typename M::template KeyedData<A0,B1>> &&x
                        ) -> typename M::template Data<typename M::template KeyedData<A0,B0>> {
                            typename M::template InnerData<B1> b1 {
                                x.environment, {x.timedData.timePoint, std::move(x.timedData.value.data), x.timedData.finalFlag}
                            };
                            auto b0 = d(std::move(b1));
                            if (!b0) {
                                return std::nullopt;
                            }
                            return { typename M::template InnerData<typename M::template KeyedData<A0,B0>> {
                                x.environment, {
                                    std::max(x.timedData.timePoint, b0->timedData.timePoint)
                                    , typename M::template KeyedData<A0,B0> {
                                        std::move(x.timedData.value.key) 
                                        , std::move(b0->timedData.value)
                                    }
                                    , (x.timedData.finalFlag && b0->timedData.finalFlag)
                                }
                            } };
                        }
                    );
                    r.registerAction(prefix+"/backwardTranslator", backwardTrans);
                    innerFacilitioid(r, std::move(inputProvider), r.actionAsSink(backwardTrans));
                    r.connect(r.actionAsSource(backwardTrans), *outputReceiver);
                } else {
                    innerFacilitioid(r, std::move(inputProvider), std::nullopt);
                }
            };
        }
        template <class A0, class B0, class A1>
        static auto wrapFacilitioidConnectorFrontOnly(
            typename infra::KleisliUtils<M>::template KleisliFunction<A0,A1> const &forwardKeyTranslator
            , typename infra::KleisliUtils<M>::template KleisliFunction<A1,A0> const &backwardKeyTranslator
            , typename R::template FacilitioidConnector<A1,B0> const &innerFacilitioid
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<A0,B0>
        {
            return 
            [
                forwardKeyTranslator
                , backwardKeyTranslator
                , innerFacilitioid
                , prefix
            ](
                R &r
                , typename R::template Source<typename M::template Key<A0>> &&inputProvider
                , std::optional<typename R::template Sink<typename M::template KeyedData<A0,B0>>> const &outputReceiver
            ) {
                auto forwardKeyTrans = M::template kleisli<typename M::template Key<A0>>(
                    CommonFlowUtilComponents<M>::template withKey<A0>(std::move(forwardKeyTranslator))
                );
                r.registerAction(prefix+"/forwardKeyTranslator", forwardKeyTrans);
                r.connect(std::move(inputProvider), r.actionAsSink(forwardKeyTrans));
                if (outputReceiver) {
                    auto backwardTrans = M::template kleisli<typename M::template KeyedData<A1,B0>>(
                        [k=std::move(backwardKeyTranslator)](
                            typename M::template InnerData<typename M::template KeyedData<A1,B0>> &&x
                        ) -> typename M::template Data<typename M::template KeyedData<A0,B0>> {
                            typename M::template InnerData<A1> a1 {
                                x.environment, {x.timedData.timePoint, x.timedData.value.key.key(), x.timedData.finalFlag}
                            };
                            auto a0 = k(std::move(a1));
                            if (!a0) {
                                return std::nullopt;
                            }
                            return { typename M::template InnerData<typename M::template KeyedData<A0,B0>> {
                                x.environment, {
                                    std::max(a0->timedData.timePoint, x.timedData.timePoint)
                                    , typename M::template KeyedData<A0,B0> {
                                        typename M::template Key<A0> {
                                            x.timedData.value.key.id()
                                            , std::move(a0->timedData.value)
                                        }   
                                        , std::move(x.timedData.value.data)
                                    }
                                    , (a0->timedData.finalFlag && x.timedData.finalFlag)
                                }
                            } };
                        }
                    );
                    r.registerAction(prefix+"/backwardTranslator", backwardTrans);
                    innerFacilitioid(r, r.actionAsSource(forwardKeyTrans), r.actionAsSink(backwardTrans));
                    r.connect(r.actionAsSource(backwardTrans), *outputReceiver);
                } else {
                    innerFacilitioid(r, r.actionAsSource(forwardKeyTrans), std::nullopt);
                }
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

        template <class OutsideBypassingData, class VIEFacility>
        struct SetupVIEFacilitySelfLoopAndWaitOutput {
            typename R::template Sinkoid<
                typename M::template Key<typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                    M, VIEFacility
                >::InputType>
            > callIntoFacility;
            typename R::template Source<
                typename M::template KeyedData<
                    typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                    >::InputType
                    , typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                    >::OutputType
                >
            > facilityOutput;
            typename R::template Source<
                typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                    M, VIEFacility
                >::ExtraOutputType
            > facilityExtraOutput;
            typename R::template Source<
                OutsideBypassingData
            > nextTriggeringSource;
        };

        //For this to work, injectF MUST be a pure 1-to-1 transformation,
        //moreover, each call into the facility on the "extra input" connector 
        //MUST produce either exactly one "initial callback" or exactly one 
        //output on the "extra output" connector, otherwise the synchronizer 
        //will simply fail to progress.
        //The logic here has no way of enforcing these 1-to-1 correspondences (in 
        //fact, there is no way to enforce 1-to-1, correspondence between the 
        //input and output of any graph node, and that is a basic property of
        //this system). Therefore, it is the user's responsibility to
        //enforce these (in general, the process from "extra input" to "extra output"
        //is fully locally controlled, and therefore absent some local failure --
        //which should cause the program to crash anyway --, the user should have
        //control over this). If the user fails to enforce these, then failure to progress
        //is going to happen.
        template <class OutsideBypassingData, class VIEFacility, class InjectF, class ExtraOutputClassifierF, class SelfLoopCreatorF, class InitialCallbackFilterF>
        static SetupVIEFacilitySelfLoopAndWaitOutput<OutsideBypassingData, VIEFacility> setupVIEFacilitySelfLoopAndWait(
            R &r
            , typename R::template Source<
                OutsideBypassingData
            > &&triggeringSource
            , InjectF &&injectF
            , ExtraOutputClassifierF const &extraOutputClassifierF //this is a predicate on extra output
                                                              //true means self-loop (initial call) is
                                                              //required, false means it is self-loop is
                                                              //not required. This predicate must be copiable
                                                              //because it will be used twice
            , SelfLoopCreatorF &&selfLoopCreatorF
            , std::shared_ptr<VIEFacility> const &facility
            , InitialCallbackFilterF &&initialCallbackFilterF //this is a predicate on output,
                                                              //true means it is a callback corresponding
                                                              //to an initial call
            , std::string const &prefix = ""
            , std::string const &facilityName = ""
        ) {
            using FacilityOutput = typename M::template KeyedData<
                typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                    M, VIEFacility
                >::InputType
                , typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                    M, VIEFacility
                >::OutputType
            >;
            using FacilityExtraOutput = typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                M, VIEFacility
            >::ExtraOutputType;
            using TriggeredFirstResponse = SingleLayerWrapper<
                std::variant<
                    typename M::template KeyedData<
                        typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                            M, VIEFacility
                        >::InputType
                        , typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                            M, VIEFacility
                        >::OutputType
                    >
                    , typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                    >::ExtraOutputType
                >
            >;

            static_assert(std::is_same_v<
                decltype(injectF(std::move(*(OutsideBypassingData *) nullptr)))
                , typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                >::ExtraInputType
            >, "injectF must convert OutsideByPassingData to ExtraInput");
            static_assert(std::is_same_v<
                decltype(extraOutputClassifierF(
                    * (FacilityExtraOutput *) nullptr
                ))
                , bool
            >, "extraOutputClassifierF must be a predicate on ExtraOutput");
            static_assert(std::is_same_v<
                decltype(selfLoopCreatorF(
                    std::move(
                        * (FacilityExtraOutput *) nullptr
                    )
                ))
                , typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                    M, VIEFacility
                >::InputType
            >, "selfLoopCreatorF must convert ExtraOutput to Input");
            static_assert(std::is_same_v<
                decltype(initialCallbackFilterF(
                    * (FacilityOutput *) nullptr
                ))
                , bool
            >, "initialCalbackFilterF must be a predicate on KeyedData<Input,Output>");

            if (facilityName != "") {
                r.registerVIEOnOrderFacility(prefix+"/"+facilityName, facility);
            }

            auto inject = M::template liftPure<OutsideBypassingData>(
                std::move(injectF)
            );
            r.registerAction(prefix+"/injection", inject);
            r.connect(
                r.execute(inject, triggeringSource.clone())
                , r.vieFacilityAsSink(facility)
            );

            auto filterForSelfLoop = M::template kleisli<typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                M, VIEFacility
            >::ExtraOutputType>(
                CommonFlowUtilComponents<M>::template pureFilter<
                    typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                    >::ExtraOutputType
                >(ExtraOutputClassifierF {extraOutputClassifierF})
            );
            auto filterForNonSelfLoop = M::template kleisli<typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                M, VIEFacility
            >::ExtraOutputType>(
                CommonFlowUtilComponents<M>::template pureFilter<
                    typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                    >::ExtraOutputType
                >(std::not_fn(extraOutputClassifierF))
            );
            r.registerAction(prefix+"/filterForSelfLoop", filterForSelfLoop);
            r.registerAction(prefix+"/filterForNonSelfLoop", filterForNonSelfLoop);

            auto selfLoopCreator = M::template liftPure<FacilityExtraOutput>(
                [selfLoopCreatorF=std::move(selfLoopCreatorF)](
                    FacilityExtraOutput &&x
                ) 
                {
                    return infra::withtime_utils::keyify<
                        typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                            M, VIEFacility
                        >::InputType
                        , typename M::EnvironmentType>
                    (
                        selfLoopCreatorF(std::move(x))
                    );
                }
            );
            r.registerAction(prefix+"/selfLoopCreation", selfLoopCreator);
            auto selfLoopInput = r.execute(
                selfLoopCreator
                , r.execute(
                    filterForSelfLoop
                    , r.vieFacilityAsSource(facility)
                )
            );

            auto facilityOutputDispatcher = M::template kleisli<FacilityOutput>(
                CommonFlowUtilComponents<M>::template idFunc<FacilityOutput>()
            );
            r.registerAction(prefix+"/facilityOutputDispatcher", facilityOutputDispatcher);
            r.placeOrderWithVIEFacility(
                std::move(selfLoopInput)
                , facility
                , r.actionAsSink(facilityOutputDispatcher)
            );

            auto initialCallbackFilter = M::template kleisli<FacilityOutput>(
                CommonFlowUtilComponents<M>::template pureFilter<
                    FacilityOutput
                >(std::move(initialCallbackFilterF))
            );
            r.registerAction(prefix+"/initialCallbackFilter", initialCallbackFilter);

            auto triggerCreator = M::template liftPure2<
                FacilityOutput
                , FacilityExtraOutput
            >(
                [](
                    int which
                    , FacilityOutput &&x
                    , FacilityExtraOutput && y
                ) -> ForceDifferentType<OutsideBypassingData> {
                    return {};
                }
            );
            r.registerAction(prefix+"/triggerCreator", triggerCreator);
            r.execute(triggerCreator, r.execute(initialCallbackFilter, r.actionAsSource(facilityOutputDispatcher)));
            r.execute(triggerCreator, r.execute(filterForNonSelfLoop, r.vieFacilityAsSource(facility)));

            auto synchronizer = CommonFlowUtilComponents<M>::template synchronizer2<
                ForceDifferentType<OutsideBypassingData>
                , OutsideBypassingData
            >([](ForceDifferentType<OutsideBypassingData> &&, OutsideBypassingData &&x) -> OutsideBypassingData {
                return std::move(x);
            });
            r.registerAction(prefix+"/synchronizer", synchronizer);
            r.execute(synchronizer, r.actionAsSource(triggerCreator));
            r.execute(synchronizer, triggeringSource.clone());

            return SetupVIEFacilitySelfLoopAndWaitOutput<OutsideBypassingData, VIEFacility> {
                r.vieFacilityAsSinkoid(facility)
                , r.actionAsSource(facilityOutputDispatcher)
                , r.vieFacilityAsSource(facility)
                , r.actionAsSource(synchronizer)
            };
        }

    private:
        template <class T>
        struct DefaultClockFacility {};
        template <class Env>
        struct DefaultClockFacility<infra::RealTimeApp<Env>> {
            using TheFacility = real_time_clock::ClockOnOrderFacility<Env>;
        };
        template <class Env>
        struct DefaultClockFacility<infra::SinglePassIterationApp<Env>> {
            using TheFacility = single_pass_iteration_clock::ClockOnOrderFacility<Env>;
        };

    public:

        //This utility function is provided because in SinglePassIterationApp
        //it is difficult to ensure a smooth exit. Even though we can pass down
        //finalFlag, the flag might get lost. An example is where an action happens
        //to return a std::nullopt at the input with the final flag, and at this point
        //the finalFlag is lost forever.
        //To help exit from SinglePassIterationApp where the final flag may get lost
        //, we can set up this exit timer.
        //However, this is not limited to SinglePassIterationApp, it is completely ok
        //to use this in RealTimeApp too.
        template <class T, class ClockFacility = typename DefaultClockFacility<M>::TheFacility>
        static void setupExitTimer(
            R &r
            , std::chrono::system_clock::duration const &exitAfterThisDurationFromFirstInput
            , typename R::template Source<T> &&inputSource
            , std::function<void(TheEnvironment *)> wrapUpFunc
            , std::string const &componentNamePrefix) 
        {
            auto exitTimer = ClockFacility::template createClockCallback<VoidStruct, VoidStruct>(
                [](std::chrono::system_clock::time_point const &, std::size_t thisIdx, std::size_t totalCount) {
                    return VoidStruct {};
                }
            );
            auto setupExitTimer = M::template liftMaybe<T>(
                [exitAfterThisDurationFromFirstInput](T &&) -> std::optional<typename M::template Key<typename ClockFacility::template FacilityInput<VoidStruct>>> {
                    return infra::withtime_utils::keyify<typename ClockFacility::template FacilityInput<VoidStruct>,TheEnvironment>(
                        typename ClockFacility::template FacilityInput<VoidStruct> {
                            VoidStruct {}
                            , {exitAfterThisDurationFromFirstInput}
                        }
                    );
                }
                , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
            );
            using ClockFacilityOutput = typename M::template KeyedData<typename ClockFacility::template FacilityInput<VoidStruct>, VoidStruct>;
            auto doExit = M::template simpleExporter<ClockFacilityOutput>(
                [wrapUpFunc](typename M::template InnerData<ClockFacilityOutput> &&data) {
                    data.environment->resolveTime(data.timedData.timePoint);
                    wrapUpFunc(data.environment);
                    data.environment->exit();
                }
            );

            r.placeOrderWithFacility(
                r.execute(componentNamePrefix+"/setupExitTimer", setupExitTimer, std::move(inputSource))
                , componentNamePrefix+"/exitTimer", exitTimer
                , r.exporterAsSink(componentNamePrefix+"/doExit", doExit)
            );
        } 
    };

} } } }

#endif