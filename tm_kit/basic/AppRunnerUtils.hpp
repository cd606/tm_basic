#ifndef TM_KIT_BASIC_APP_RUNNER_UTILS_HPP_
#define TM_KIT_BASIC_APP_RUNNER_UTILS_HPP_

#include <tm_kit/infra/KleisliUtils.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/infra/TupleVariantHelper.hpp>
#include <tm_kit/basic/CommonFlowUtils.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/ForceDifferentType.hpp>
#include <tm_kit/basic/AppClockHelper.hpp>
#include <tm_kit/basic/ByteData.hpp>

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

        template <class Input, class Output, class ExtraValue>
        static auto convertIntoTuple2FacilitioidByDiscardingExtraValue(
            typename R::template FacilitioidConnector<
                Input
                , Output
            > toBeConverted
            , std::string const &convertPrefix
        ) -> typename R::template FacilitioidConnector<
                std::tuple<ExtraValue, Input>
                , Output
            >
        {
            class LocalF : public M::template AbstractIntegratedVIEOnOrderFacilityWithPublish<
                std::tuple<ExtraValue, Input>
                , Output
                , typename M::template KeyedData<Input, Output>
                , typename M::template Key<Input>
            > {
            private:
                using P = typename M::template AbstractIntegratedVIEOnOrderFacilityWithPublish<
                    std::tuple<ExtraValue, Input>
                    , Output
                    , typename M::template KeyedData<Input, Output>
                    , typename M::template Key<Input>
                >;
                using ImporterParent = typename P::ImporterParent;
                using FacilityParent = typename P::FacilityParent;
            public:
                LocalF() {}
                virtual void start(TheEnvironment *) override final {}
                virtual void handle(typename M::template InnerData<typename M::template Key<std::tuple<ExtraValue, Input>>> &&input) override final {
                    this->ImporterParent::publish(
                        input.environment
                        , typename M::template Key<Input> {
                            input.timedData.value.id()
                            , std::move(std::get<1>(input.timedData.value.key()))
                        }
                    );
                }
                virtual void handle(typename M::template InnerData<typename M::template KeyedData<Input,Output>> &&result) override final {
                    this->FacilityParent::publish(
                        result.environment
                        , typename M::template Key<Output> {
                            result.timedData.value.key.id()
                            , std::move(result.timedData.value.data)
                        }
                        , result.timedData.finalFlag
                    );
                }
            };
            return [toBeConverted,convertPrefix](
                R &r 
                , typename R::template Source<typename M::template Key<std::tuple<ExtraValue,Input>>> &&source 
                , std::optional<typename R::template Sink<typename M::template KeyedData<std::tuple<ExtraValue, Input>, Output>>> const &sink
            ) {
                auto vieFacility = M::template vieOnOrderFacility<
                    std::tuple<ExtraValue, Input>
                    , Output
                    , typename M::template KeyedData<Input, Output>
                    , typename M::template Key<Input>
                    , true
                >(new LocalF());
                r.registerVIEOnOrderFacility(convertPrefix+"/vieFacility", vieFacility);
                toBeConverted(r, r.vieFacilityAsSource(vieFacility), r.vieFacilityAsSink(vieFacility));
                if (sink) {
                    r.placeOrderWithVIEFacility(std::move(source), vieFacility, *sink);
                } else {
                    r.placeOrderWithVIEFacilityAndForget(std::move(source), vieFacility);
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
                CommonFlowUtilComponents<M>::template pureTwoWayFilter<
                    typename infra::withtime_utils::VIEOnOrderFacilityTypeInfo<
                        M, VIEFacility
                    >::ExtraOutputType
                >(ExtraOutputClassifierF {extraOutputClassifierF})
            );
            r.registerAction(prefix+"/filterForSelfLoop", filterForSelfLoop);

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
            r.connect_2_0(
                r.execute(
                    filterForSelfLoop
                    , r.vieFacilityAsSource(facility)
                )
                , r.actionAsSink(selfLoopCreator)
            );
            auto selfLoopInput = r.actionAsSource(
                selfLoopCreator
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
                    std::variant<FacilityOutput, FacilityExtraOutput> &&
                ) -> ForceDifferentType<OutsideBypassingData> {
                    return {};
                }
            );
            r.registerAction(prefix+"/triggerCreator", triggerCreator);
            r.execute(triggerCreator, r.execute(initialCallbackFilter, r.actionAsSource(facilityOutputDispatcher)));
            r.connect_2_1(r.actionAsSource(filterForSelfLoop), r.actionAsSink_2_1(triggerCreator));

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
        template <class T>
        static void setupExitTimer(
            R &r
            , typename M::Duration const &exitAfterThisDurationFromFirstInput
            , typename R::template Source<T> &&inputSource
            , std::function<void(TheEnvironment *)> wrapUpFunc
            , std::string const &componentNamePrefix) 
        {
            using ClockFacility = typename AppClockHelper<M>::Facility;
            auto exitTimer = ClockFacility::template createClockCallback<VoidStruct, VoidStruct>(
                [](typename M::TimePoint const &, std::size_t thisIdx, std::size_t totalCount) {
                    return VoidStruct {};
                }
            );
            using ClockFacilityOutput = typename M::template KeyedData<typename ClockFacility::template FacilityInput<VoidStruct>, VoidStruct>;
            auto doExit = M::template simpleExporter<ClockFacilityOutput>(
                [wrapUpFunc](typename M::template InnerData<ClockFacilityOutput> &&data) {
                    data.environment->resolveTime(data.timedData.timePoint);
                    wrapUpFunc(data.environment);
                    data.environment->exit();
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
            if constexpr (infra::withtime_utils::IsVariant<T>::Value) {
                r.registerAction(componentNamePrefix+"/setupExitTimer", setupExitTimer);

                r.placeOrderWithFacility(
                    infra::AppRunnerHelper::GenericExecute<R>::call(r, setupExitTimer, std::move(inputSource))
                    , componentNamePrefix+"/exitTimer", exitTimer
                    , r.exporterAsSink(componentNamePrefix+"/doExit", doExit)
                );
            } else {
                r.placeOrderWithFacility(
                    r.execute(componentNamePrefix+"/setupExitTimer", setupExitTimer, std::move(inputSource))
                    , componentNamePrefix+"/exitTimer", exitTimer
                    , r.exporterAsSink(componentNamePrefix+"/doExit", doExit)
                );
            }
        } 

        static void setupBackgroundClockAndExiter(
            R &r
            , typename M::TimePoint const &startTime
            , typename M::TimePoint const &exitTime 
            , typename M::Duration const &frequency
            , std::function<void(TheEnvironment *)> const &wrapUpFunc
            , std::string const &prefix) 
        {
            if constexpr (std::is_same_v<M, typename infra::SinglePassIterationApp<TheEnvironment>>) {
                using ClockImporter = typename basic::AppClockHelper<M>::Importer;
                auto backgroundClock = ClockImporter::template createRecurringClockConstImporter<basic::VoidStruct>(
                    startTime
                    , exitTime
                    , frequency
                    , basic::VoidStruct {}
                );
                r.registerImporter(prefix+"/backgroundClock", backgroundClock);
                auto exiter = M::template simpleExporter<basic::VoidStruct>(
                    [exitTime,frequency,wrapUpFunc](typename M::template InnerData<basic::VoidStruct> &&data) {
                        data.environment->resolveTime(data.timedData.timePoint);
                        if (data.timedData.timePoint+frequency > exitTime) {
                            wrapUpFunc(data.environment);
                            data.environment->exit();
                        }
                    }
                );
                r.registerExporter(prefix+"/exiter", exiter);
                r.exportItem(exiter, r.importItem(backgroundClock));
            }
        } 

        template <class A, class B>
        static typename R::template Pathway<A,B> singleActionPathway(
            typename R::template ActionPtr<A,B> const &action
        ) {
            return [action](
                R &r 
                , typename R::template Source<A> &&source 
                , typename R::template Sink<B> const &sink
            ) {
                r.connect(r.execute(action, std::move(source)), sink);
            };
        }
        template <class A, class B>
        static typename R::template Pathway<A,B> singleFacilityPathway(
            typename R::template FacilitioidConnector<A,B> facility
            , std::string const &prefix
        ) {
            return [facility,prefix](
                R &r 
                , typename R::template Source<A> &&source 
                , typename R::template Sink<B> const &sink
            ) {
                auto keyify = M::template kleisli<A>(
                    CommonFlowUtilComponents<M>::template keyify<A>()
                );
                auto extractData = M::template kleisli<typename M::template KeyedData<A,B>>(
                    CommonFlowUtilComponents<M>::template extractDataFromKeyedData<A,B>()
                );
                r.registerAction(prefix+"/keyify", keyify);
                r.registerAction(prefix+"/extractData", extractData);
                facility(
                    r 
                    , r.execute(keyify, std::move(source))
                    , r.actionAsSink(extractData)
                );
                r.connect(r.actionAsSource(extractData), sink);
            };
        }

        template <class A, class B>
        static typename R::template Pathway<A,B> pathwayWithTimeout(
            typename M::Duration const &timeout 
            , typename R::template Pathway<A,B> mainPathway 
            , typename R::template Pathway<A,B> secondaryPathway 
            , std::string const &prefix
        ) {
            using ClockFacility = typename AppClockHelper<M>::Facility;
            return [timeout,mainPathway,secondaryPathway,prefix](
                R &r 
                , typename R::template Source<A> &&source 
                , typename R::template Sink<B> const &sink
            ) {
                auto timer = ClockFacility::template createClockCallback<VoidStruct, VoidStruct>(
                    [](typename M::TimePoint const &, std::size_t thisIdx, std::size_t totalCount) {
                        return VoidStruct {};
                    }
                );
                auto setupTimer = M::template liftMaybe<A>(
                    [timeout](A &&) -> std::optional<typename M::template Key<typename ClockFacility::template FacilityInput<VoidStruct>>> {
                        return infra::withtime_utils::keyify<typename ClockFacility::template FacilityInput<VoidStruct>,TheEnvironment>(
                            typename ClockFacility::template FacilityInput<VoidStruct> {
                                VoidStruct {}
                                , {timeout}
                            }
                        );
                    }
                    , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
                );
                using ClockFacilityOutput = typename M::template KeyedData<typename ClockFacility::template FacilityInput<VoidStruct>, VoidStruct>;
                auto synchronizer = CommonFlowUtilComponents<M>::template synchronizer2<
                    A
                    , ClockFacilityOutput
                >([](A &&a, ClockFacilityOutput &&) -> A {
                    return std::move(a);
                });
                auto passThrough = M::template kleisli2<A,B>(
                    CommonFlowUtilComponents<M>::template idFunc<std::variant<A,B>>()
                    , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
                );
                auto passThroughConverter = M::template kleisli<A>(
                    CommonFlowUtilComponents<M>::template idFunc<A>()
                    , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
                );
                auto collector = M::template kleisli<B>(
                    CommonFlowUtilComponents<M>::template idFunc<B>()
                    , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
                );

                r.registerOnOrderFacility(prefix+"/timer", timer);
                r.registerAction(prefix+"/setupTimer", setupTimer);
                r.registerAction(prefix+"/synchronizer", synchronizer);
                r.registerAction(prefix+"/passThrough", passThrough);
                r.registerAction(prefix+"/passThroughConverter", passThroughConverter);
                r.registerAction(prefix+"/collector", collector);

                r.placeOrderWithFacility(
                    r.execute(setupTimer, source.clone())
                    , timer 
                    , r.actionAsSink_2_1(synchronizer)
                );
                r.connect(source.clone(), r.actionAsSink_2_0(synchronizer));
                mainPathway(r, source.clone(), r.actionAsSink_2_1(passThrough));
                r.connect_2_1(r.actionAsSource(passThrough), r.actionAsSink(collector));
                r.connect(r.actionAsSource(synchronizer), r.actionAsSink_2_0(passThrough));
                r.connect_2_0(r.actionAsSource(passThrough), r.actionAsSink(passThroughConverter));
                secondaryPathway(r, r.actionAsSource(passThroughConverter), r.actionAsSink(collector));
                r.connect(r.actionAsSource(collector), sink);
            };
        }

    private:
        template <std::size_t N, std::size_t K, class S, class T>
        class DispatchForParallel {
        public:
            using ToBeDispatched = infra::VariantConcat<S, infra::VariantRepeat<N,T>>;
            using SinkCreator = std::function<typename R::template Sink<T>(std::size_t)>;
            static void apply(R &r, typename R::template Source<ToBeDispatched> &&source, SinkCreator sinkCreator) {
                auto sink = sinkCreator(K);
                infra::AppRunnerHelper::Connect<N+1,K+1>::template call<R,ToBeDispatched>(r, source.clone(), sink);
                if constexpr (K+1 < N) {
                    DispatchForParallel<N, K+1, S, T>::apply(r, source.clone(), sinkCreator);
                }
            }
        };
    public:
        template <
            std::size_t N //how many processor copies
            , class Input //overall input
            , class Output //overall output
            , class InputSeparatorF //separate input into shared and individual inputs
                    //requires:
                    //InputSeparatorF::SharedPartialInputType
                    //InputSeparatorF::IndividualPartialInputType
                    //std::variant<SharedPartialInputType, IndividualPartialInputType> call(Input &&)
            , class InputDispatcherF //dispatch individual inputs
                    //requires:
                    //std::size_t call(std::size_t, IndividualPartialInputType const &)
        >
        static void parallel_withSeparatorNode(
            R &r
            , typename R::template Sourceoid<Input> inputSource
            , InputSeparatorF &&inputSeparator 
            , InputDispatcherF &&inputDispatcher 
            , std::function<
                typename R::template ActionPtr<
                    std::variant<
                        typename InputSeparatorF::SharedPartialInputType
                        , typename InputSeparatorF::IndividualPartialInputType 
                    >
                    , Output
                >(std::size_t)
            > actionCreator
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            if constexpr (std::is_same_v<M, typename infra::RealTimeApp<TheEnvironment>>) {
                //parallel only makes sense in real time
                using SeparatedInput =
                    infra::VariantConcat<
                        typename InputSeparatorF::SharedPartialInputType
                        , infra::VariantRepeat<N, typename InputSeparatorF::IndividualPartialInputType>
                    >;
                class LocalSeparatorAction {
                private:
                    InputSeparatorF separator_;
                    InputDispatcherF dispatcher_;
                public:
                    LocalSeparatorAction(InputSeparatorF &&separator, InputDispatcherF &&dispatcher) : separator_(std::move(separator)), dispatcher_(std::move(dispatcher)) {}
                    SeparatedInput operator()(Input &&input) {
                        return std::visit([this](auto &&x) -> SeparatedInput {
                            using T = std::decay_t<decltype(x)>;
                            if constexpr (std::is_same_v<T, typename InputSeparatorF::SharedPartialInputType>) {
                                return SeparatedInput {
                                    std::in_place_index<0>
                                    , std::move(x)
                                };
                            } else {
                                auto idx = dispatcher_.call(N, x) % N;
                                return infra::augmentedDispatchVariantConstruct<
                                    N, typename InputSeparatorF::SharedPartialInputType, typename InputSeparatorF::IndividualPartialInputType
                                >(idx, std::move(x));
                            }
                        }, separator_.call(std::move(input)));
                    }
                };
                auto separator = M::template liftPure<Input>(LocalSeparatorAction(std::move(inputSeparator), std::move(inputDispatcher)));
                r.registerAction(prefix+"/separator", separator);
                inputSource(r, r.actionAsSink(separator));

                DispatchForParallel<N,0,typename InputSeparatorF::SharedPartialInputType,typename InputSeparatorF::IndividualPartialInputType>::apply(
                    r
                    , r.actionAsSource(separator)
                    , [&r,&prefix,&separator,actionCreator,outputSink](std::size_t idx) -> typename R::template Sink<typename InputSeparatorF::IndividualPartialInputType> {
                        auto action = actionCreator(idx);
                        r.registerAction(prefix+"/action_"+std::to_string(idx), action);
                        infra::AppRunnerHelper::Connect<N+1,0>::template call<R,SeparatedInput>(r, r.actionAsSource(separator), r.actionAsSink_2_0(action));
                        outputSink(r, r.actionAsSource(action));
                        return r.actionAsSink_2_1(action);
                    }
                );
            } else {
                class LocalSeparatorAction {
                private:
                    InputSeparatorF separator_;
                    InputDispatcherF dispatcher_;
                public:
                    LocalSeparatorAction(InputSeparatorF &&separator, InputDispatcherF &&dispatcher) : separator_(std::move(separator)), dispatcher_(std::move(dispatcher)) {}
                    std::variant<
                        typename InputSeparatorF::SharedPartialInputType, typename InputSeparatorF::IndividualPartialInputType
                    > operator()(Input &&input) {
                        auto x = separator_.call(std::move(input));
                        if (x.index() == 1) {
                            dispatcher_.call(0, std::get<1>(x));
                        }
                        return std::move(x);
                    }
                };
                auto separator = M::template liftPure<Input>(LocalSeparatorAction(std::move(inputSeparator), std::move(inputDispatcher)));
                r.registerAction(prefix+"/separator", separator);
                inputSource(r, r.actionAsSink(separator));

                auto action = actionCreator(0);
                r.registerAction(prefix+"/action", action);
                r.connect_2_0(r.actionAsSource(separator), r.actionAsSink_2_0(action));
                r.connect_2_1(r.actionAsSource(separator), r.actionAsSink_2_1(action));
                outputSink(r, r.actionAsSource(action));
            }
        }
    private:
        template <std::size_t N, std::size_t K, class T>
        class DispatchForParallel2 {
        public:
            using ToBeDispatched = infra::VariantRepeat<N,T>;
            using SinkCreator = std::function<typename R::template Sink<T>(std::size_t)>;
            static void apply(R &r, typename R::template Source<ToBeDispatched> &&source, SinkCreator sinkCreator) {
                auto sink = sinkCreator(K);
                infra::AppRunnerHelper::Connect<N,K>::template call<R,ToBeDispatched>(r, source.clone(), sink);
                if constexpr (K+1 < N) {
                    DispatchForParallel2<N, K+1, T>::apply(r, source.clone(), sinkCreator);
                }
            }
        };
    public:
        template <
            std::size_t N //how many processor copies
            , class SharedInput //shared input
            , class IndividualInput //individual input
            , class Output //overall output
            , class InputDispatcherF //dispatch individual inputs
                    //requires:
                    //std::size_t call(std::size_t, IndividualInput const &)
        >
        static void parallel_withoutSeparatorNode(
            R &r
            , typename R::template Sourceoid<SharedInput> sharedInputSource
            , typename R::template Sourceoid<IndividualInput> individualInputSource
            , InputDispatcherF &&inputDispatcher 
            , std::function<
                typename R::template ActionPtr<
                    std::variant<
                        SharedInput
                        , IndividualInput
                    >
                    , Output
                >(std::size_t)
            > actionCreator
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            if constexpr (std::is_same_v<M, typename infra::RealTimeApp<TheEnvironment>>) {
                //parallel only makes sense in real time
                using DispatchedInput = infra::VariantRepeat<N, IndividualInput>;
                class LocalDispatcherAction {
                private:
                    InputDispatcherF dispatcher_;
                public:
                    LocalDispatcherAction(InputDispatcherF &&dispatcher) : dispatcher_(std::move(dispatcher)) {}
                    DispatchedInput operator()(IndividualInput &&input) {
                        auto idx = dispatcher_.call(N, input) % N;
                        return infra::dispatchVariantConstruct<
                            N, IndividualInput
                        >(idx, std::move(input));
                    }
                };
                auto dispatcher = M::template liftPure<IndividualInput>(LocalDispatcherAction(std::move(inputDispatcher)));
                r.registerAction(prefix+"/dispatcher", dispatcher);
                individualInputSource(r, r.actionAsSink(dispatcher));

                DispatchForParallel2<N,0,IndividualInput>::apply(
                    r
                    , r.actionAsSource(dispatcher)
                    , [&r,&prefix,sharedInputSource,actionCreator,outputSink](std::size_t idx) -> typename R::template Sink<IndividualInput> {
                        auto action = actionCreator(idx);
                        r.registerAction(prefix+"/action_"+std::to_string(idx), action);
                        sharedInputSource(r, r.actionAsSink_2_0(action));
                        outputSink(r, r.actionAsSource(action));
                        return r.actionAsSink_2_1(action);
                    }
                );
            } else {
                class LocalDispatcherAction {
                private:
                    InputDispatcherF dispatcher_;
                public:
                    LocalDispatcherAction(InputDispatcherF &&dispatcher) : dispatcher_(std::move(dispatcher)) {}
                    IndividualInput operator()(IndividualInput &&input) {
                        dispatcher_.call(0, input);
                        return std::move(std::move(input));
                    }
                };
                auto dispatcher = M::template liftPure<IndividualInput>(LocalDispatcherAction(std::move(inputDispatcher)));
                r.registerAction(prefix+"/dispatcher", dispatcher);
                individualInputSource(r, r.actionAsSink(dispatcher));

                auto action = actionCreator(0);
                r.registerAction(prefix+"/action", action);
                sharedInputSource(r, r.actionAsSink_2_0(action));
                r.connect(r.actionAsSource(dispatcher), r.actionAsSink_2_1(action));
                outputSink(r, r.actionAsSource(action));
            }
        }
        
        template <class T>
        class WrapWithCBORIfNecessaryForServerFacilityInput {
        public:
            using WrappedType = std::conditional_t<
                bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()
                , T 
                , CBOR<T>
            >;
            static constexpr bool IsWrapped = !(bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable());
            static WrappedType wrap(T &&t) {
                if constexpr (IsWrapped) {
                    return {std::move(t)};
                } else {
                    return std::move(t);
                }
            }
            static T unwrap(WrappedType &&t) {
                if constexpr (IsWrapped) {
                    return {std::move(t.value)};
                } else {
                    return std::move(t);
                }
            }
        };
        template <class A, class T>
        class WrapWithCBORIfNecessaryForServerFacilityInput<std::tuple<A,T>> {
        public:
            using WrappedType = std::conditional_t<
                bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()
                , std::tuple<A,T> 
                , std::tuple<A,CBOR<T>>
            >;
            static constexpr bool IsWrapped = !(bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable());
            static WrappedType wrap(std::tuple<A,T> &&t) {
                if constexpr (IsWrapped) {
                    return {std::move(std::get<0>(t)), {std::move(std::get<1>(t))}};
                } else {
                    return std::move(t);
                }
            }
            static std::tuple<A,T> unwrap(WrappedType &&t) {
                if constexpr (IsWrapped) {
                    return {std::move(std::get<0>(t)), std::move(std::get<1>(t).value)};
                } else {
                    return std::move(t);
                }
            }
        };

        template <class T>
        class WrapWithCBORIfNecessaryForServerFacilityOutput {
        public:
            using WrappedType = std::conditional_t<
                bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()
                , T 
                , CBOR<T>
            >;
            static constexpr bool IsWrapped = !(bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable());
            static WrappedType wrap(T &&t) {
                if constexpr (IsWrapped) {
                    return {std::move(t)};
                } else {
                    return std::move(t);
                }
            }
            static T unwrap(WrappedType &&t) {
                if constexpr (IsWrapped) {
                    return {std::move(t.value)};
                } else {
                    return std::move(t);
                }
            }
        };
        
        template <class T>
        using WrapWithCBORIfNecessarySimple = WrapWithCBORIfNecessaryForServerFacilityOutput<T>;

        template <class A, class B>
        static auto makeServerSideFacilitioidConnectorSerializable(
            typename R::template FacilitioidConnector<A,B> const &innerFacilitioid
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<
            typename WrapWithCBORIfNecessaryForServerFacilityInput<A>::WrappedType
            , typename WrapWithCBORIfNecessaryForServerFacilityOutput<B>::WrappedType
        > {
            if constexpr (!WrapWithCBORIfNecessaryForServerFacilityInput<A>::IsWrapped) {
                if constexpr (!WrapWithCBORIfNecessaryForServerFacilityOutput<B>::IsWrapped) {
                    return innerFacilitioid;
                } else {
                    return wrapFacilitioidConnectorBackOnly<
                        A 
                        , typename WrapWithCBORIfNecessaryForServerFacilityOutput<B>::WrappedType
                        , B
                    >(
                        infra::KleisliUtils<M>::template liftPure<B>(
                            &(WrapWithCBORIfNecessaryForServerFacilityOutput<B>::wrap)
                        )
                        , innerFacilitioid
                        , prefix
                    );
                }
            } else {
                if constexpr (!WrapWithCBORIfNecessaryForServerFacilityOutput<B>::IsWrapped) {
                    return wrapFacilitioidConnectorFrontOnly<
                        typename WrapWithCBORIfNecessaryForServerFacilityInput<A>::WrappedType
                        , B
                        , A
                    >(
                        infra::KleisliUtils<M>::template liftPure<typename WrapWithCBORIfNecessaryForServerFacilityInput<A>::WrappedType>(
                            &(WrapWithCBORIfNecessaryForServerFacilityInput<A>::unwrap)
                        )
                        , infra::KleisliUtils<M>::template liftPure<A>(
                            &(WrapWithCBORIfNecessaryForServerFacilityInput<A>::wrap)
                        )
                        , innerFacilitioid
                        , prefix
                    );
                } else {
                    return wrapFacilitioidConnector<
                        typename WrapWithCBORIfNecessaryForServerFacilityInput<A>::WrappedType 
                        , typename WrapWithCBORIfNecessaryForServerFacilityOutput<B>::WrappedType
                        , A 
                        , B
                    >(
                        infra::KleisliUtils<M>::template liftPure<typename WrapWithCBORIfNecessaryForServerFacilityInput<A>::WrappedType>(
                            &(WrapWithCBORIfNecessaryForServerFacilityInput<A>::unwrap)
                        )
                        , infra::KleisliUtils<M>::template liftPure<A>(
                            &(WrapWithCBORIfNecessaryForServerFacilityInput<A>::wrap)
                        )
                        , infra::KleisliUtils<M>::template liftPure<B>(
                            &(WrapWithCBORIfNecessaryForServerFacilityOutput<B>::wrap)
                        )
                        , innerFacilitioid
                        , prefix
                    );
                }
            }
        }

        template <class T>
        static void If(
            R &r
            , typename R::template Source<std::tuple<T,bool>> &&source
            , typename R::template Sink<T> const &trueSink
            , typename R::template Sink<T> const &falseSink
            , std::string const &componentPrefix
        ) {
            auto ifDispatch = typename R::AppType::template liftPure<std::tuple<T,bool>>(
                [](std::tuple<T,bool> &&x) -> std::variant<T,T> {
                    if (std::get<1>(x)) {
                        return {std::in_place_index<0>, std::move(std::get<0>(x))};
                    } else {
                        return {std::in_place_index<1>, std::move(std::get<0>(x))};
                    }
                }
            );
            r.registerAction(componentPrefix+"/ifDispatch", ifDispatch);
            r.connect_2_0(r.actionAsSource(ifDispatch), trueSink);
            r.connect_2_1(r.actionAsSource(ifDispatch), falseSink);
            r.connect(std::move(source), r.actionAsSink(ifDispatch));
        }
        template <class T>
        static void DoWhile(
            R &r
            , typename R::template Source<T> &&source
            , typename R::template FacilitioidConnector<
                T, std::tuple<T,bool>
            > loopPart
            , typename R::template Sink<T> const &continuationSink
            , std::string const &componentPrefix
        ) {
            auto doWhileDispatch = typename R::AppType::template liftPure2<
                T 
                , typename R::AppType::template KeyedData<T, std::tuple<T,bool>>
            >(
                [](std::variant<
                    T 
                    , typename R::AppType::template KeyedData<T, std::tuple<T,bool>>
                > &&data) 
                    -> std::variant<T,T>
                {
                    return std::visit([](auto &&x) -> std::variant<T,T> {
                        using X = std::decay_t<decltype(x)>;
                        if constexpr (std::is_same_v<X, T>) {
                            return {std::in_place_index<1>, std::move(x)};
                        } else {
                            if (std::get<1>(x.data)) {
                                return {std::in_place_index<1>, std::move(std::get<0>(x.data))};
                            } else {
                                return {std::in_place_index<0>, std::move(std::get<0>(x.data))};
                            }
                        }
                    }, std::move(data));
                }
            );
            r.registerAction(componentPrefix+"/doWhileDispatch", doWhileDispatch);
            auto keyify = infra::KleisliUtils<typename R::AppType>::action(
                CommonFlowUtilComponents<typename R::AppType>::template keyify<T>()
            );
            r.registerAction(componentPrefix+"/keyify", keyify);
            r.connect_2_0(r.actionAsSource(doWhileDispatch), continuationSink);
            r.connect_2_1(r.actionAsSource(doWhileDispatch), r.actionAsSink(keyify));
            loopPart(
                r 
                , r.actionAsSource(keyify)
                , r.actionAsSink_2_1(doWhileDispatch)
            );
            r.connect(std::move(source), r.actionAsSink_2_0(doWhileDispatch));
        }

        template <class A, class B, class DurationsGen, class ClockOp>
        static auto clockBasedFacility(
            DurationsGen &&durationsGen
            , ClockOp &&clockOp
            , std::string const &componentPrefix
        ) -> typename R::template FacilitioidConnector<
            A, B
        > {
            return [durationsGen=std::move(durationsGen), clockOp=std::move(clockOp), componentPrefix](
                R &r 
                , typename R::template Source<typename R::AppType::template Key<A>> &&source 
                , std::optional<typename R::template Sink<typename R::AppType::template KeyedData<A,B>>> const &sink
            ) {
                using ClockF = typename AppClockHelper<typename R::AppType>::Facility;
                using ClockFInput = typename ClockF::template FacilityInput<A>;
                class GenKey {
                private:
                    DurationsGen durationsGen_;
                public:
                    GenKey(DurationsGen &&g) : durationsGen_(std::move(g)) {}
                    typename R::AppType::template Key<ClockFInput> operator()(typename R::AppType::template Key<A> &&k) {
                        return {
                            k.id()
                            , {
                                k.key()
                                , durationsGen_(k.key())
                            }
                        };
                    }
                };
                auto durationsGen1 = std::move(durationsGen);
                auto genKey = R::AppType::template liftPure<typename R::AppType::template Key<A>>(
                    GenKey(std::move(durationsGen1))
                );
                r.registerAction(componentPrefix+"/genKey", genKey);
                auto clockOp1 = std::move(clockOp);
                auto clockF = ClockF::template createGenericClockCallback<A,B>(std::move(clockOp1));
                r.registerOnOrderFacility(componentPrefix+"/clockFacility", clockF);
                if (!sink) {
                    r.placeOrderWithFacilityAndForget(
                        r.execute(genKey, std::move(source))
                        , clockF
                    );
                } else {
                    auto retrieveData = R::AppType::template liftPure<typename R::AppType::template KeyedData<ClockFInput,B>>(
                        [](typename R::AppType::template KeyedData<ClockFInput,B> &&d) 
                            -> typename R::AppType::template KeyedData<A,B>
                        {
                            return {
                                {d.key.id(), std::move(d.key.key().inputData)}
                                , std::move(d.data)
                            };
                        }
                    );
                    r.registerAction(componentPrefix+"/retrieveData", retrieveData);
                    r.placeOrderWithFacility(
                        r.execute(genKey, std::move(source))
                        , clockF
                        , r.actionAsSink(retrieveData)
                    );
                    r.connect(r.actionAsSource(retrieveData), *sink);
                }
            };
        }

        template <class A, class B1, class B2, class Merger>
        static auto facilitoidSynchronizer2(
            typename R::template FacilitioidConnector<A, B1> facility1
            , typename R::template FacilitioidConnector<A, B2> facility2
            , Merger &&merger
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<
            A, decltype(merger(std::declval<A>(), std::declval<B1>(), std::declval<B2>()))
        > {
            using B = decltype(merger(std::declval<A>(), std::declval<B1>(), std::declval<B2>()));
            return [facility1,facility2,merger=std::move(merger),prefix](
                R &r 
                , typename R::template Source<typename R::AppType::template Key<A>> &&source 
                , std::optional<typename R::template Sink<typename R::AppType::template KeyedData<A,B>>> const &sink
            ) {
                auto m1 = std::move(merger);
                auto sync = CommonFlowUtilComponents<typename R::AppType>::template keyedSynchronizer2<
                    typename R::AppType::template KeyedData<A,B1>
                    , typename R::AppType::template KeyedData<A,B2>
                >(
                    [](typename R::AppType::template KeyedData<A,B1> const &x) {
                        return x.key.id();
                    }
                    , [](typename R::AppType::template KeyedData<A,B2> const &y) {
                        return y.key.id();
                    }
                    , [m1=std::move(m1)](typename R::AppType::template KeyedData<A,B1> &&x, typename R::AppType::template KeyedData<A,B2> &&y) -> typename R::AppType::template KeyedData<A,B> {
                        return typename R::AppType::template KeyedData<A,B> {
                            std::move(x.key)
                            , m1(y.key.key(), std::move(x.data), std::move(y.data))
                        };
                    }
                );
                r.registerAction(prefix+"/sync", sync);
                facility1(r, source.clone(), r.actionAsSink_2_0(sync));
                facility2(r, source.clone(), r.actionAsSink_2_1(sync));
                if (sink) {
                    r.connect(r.actionAsSource(sync), *sink);
                }
            };
        }

        template <class T>
        static auto singlePassTestRunOneUpdate(
            typename R::template Sourceoid<T> source
        ) ->  std::optional<infra::WithTime<T, typename R::AppType::TimePoint>> 
        {
            static_assert(
                std::is_same_v<
                    typename R::AppType 
                    , infra::SinglePassIterationApp<TheEnvironment>
                >
                , "singlePassTestRunOneUpdate can only be used with single pass app"
            );
            TheEnvironment env;
            R r(&env);

            std::optional<infra::WithTime<T, typename R::AppType::TimePoint>> ret = std::nullopt;
            auto getFirstUpdate = R::AppType::template kleisli<T>(
                [&ret](typename R::AppType::template InnerData<T> &&data) -> typename R::AppType::template Data<VoidStruct> {
                    ret = { std::move(data.timedData) };
                    return typename R::AppType::template InnerData<VoidStruct> {
                        data.environment 
                        , {
                            data.environment->resolveTime(ret->timePoint)
                            , {}
                            , true
                        }
                    };
                }
                , infra::LiftParameters<typename R::AppType::TimePoint>().FireOnceOnly(true)
            );
            r.registerAction("getFirstUpdate", getFirstUpdate);

            auto exporter = R::AppType::template trivialExporter<VoidStruct>();
            r.registerExporter("exporter", exporter);

            source(r, r.actionAsSink(getFirstUpdate));
            r.exportItem(exporter, r.actionAsSource(getFirstUpdate));
            r.finalize();

            return std::move(ret);
        }
        template <class T>
        static auto singlePassTestRunOneUpdate(
            std::function<
                typename R::template ImporterPtr<T>()
            > importerGen
        ) ->  std::optional<infra::WithTime<T, typename R::AppType::TimePoint>> 
        {
            static_assert(
                std::is_same_v<
                    typename R::AppType 
                    , infra::SinglePassIterationApp<TheEnvironment>
                >
                , "singlePassTestRunOneUpdate can only be used with single pass app"
            );
            return singlePassTestRunOneUpdate<T>(
                [importerGen](R &r, typename R::template Sink<T> const &sink) {
                    auto importer = importerGen();
                    r.registerImporter("importer", importer);
                    r.connect(r.importItem(importer), sink);
                }
            );
        }
    };

} } } }

#endif