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
            if constexpr (
                std::is_same_v<M, typename infra::SinglePassIterationApp<TheEnvironment>>
                ||
                std::is_same_v<M, typename infra::TopDownSinglePassIterationApp<TheEnvironment>>
            ) {
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
            static void applySourceoid(R &r, typename R::template Sourceoid<ToBeDispatched> const &source, SinkCreator sinkCreator) {
                auto sink = sinkCreator(K);
                source(r, infra::VariantRepeat<N,typename R::template Sink<T>> {std::in_place_index<K>, sink});
                if constexpr (K+1 < N) {
                    DispatchForParallel2<N, K+1, T>::applySourceoid(r, source, sinkCreator);
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
        }

        template <
            std::size_t N //how many processor copies
            , class Input
            , class Output 
            , class InputDispatcherF //dispatch individual inputs
                    //requires:
                    //std::size_t call(std::size_t, Input const &)
        >
        static void parallel_fullyIndependent(
            R &r
            , typename R::template Sourceoid<Input> inputSource
            , InputDispatcherF &&inputDispatcher 
            , std::function<
                typename R::template ActionPtr<
                    Input
                    , Output
                >(std::size_t)
            > actionCreator
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            using DispatchedInput = infra::VariantRepeat<N, Input>;
            class LocalDispatcherAction {
            private:
                InputDispatcherF dispatcher_;
            public:
                LocalDispatcherAction(InputDispatcherF &&dispatcher) : dispatcher_(std::move(dispatcher)) {}
                DispatchedInput operator()(Input &&input) {
                    auto idx = dispatcher_.call(N, input) % N;
                    return infra::dispatchVariantConstruct<
                        N, Input
                    >(idx, std::move(input));
                }
            };
            auto dispatcher = M::template liftPure<Input>(LocalDispatcherAction(std::move(inputDispatcher)));
            r.registerAction(prefix+"/dispatcher", dispatcher);
            inputSource(r, r.actionAsSink(dispatcher));

            DispatchForParallel2<N,0,Input>::apply(
                r
                , r.actionAsSource(dispatcher)
                , [&r,&prefix,actionCreator,outputSink](std::size_t idx) -> typename R::template Sink<Input> {
                    auto action = actionCreator(idx);
                    r.registerAction(prefix+"/action_"+std::to_string(idx), action);
                    outputSink(r, r.actionAsSource(action));
                    return r.actionAsSink(action);
                }
            );
        }

        template <
            std::size_t N //how many processor copies
            , class Input
            , class Output 
        >
        static void parallel_fullyIndependent_alreadyDispatched(
            R &r
            , typename R::template Source<infra::VariantRepeat<N, Input>> &&dispatchedInputSource
            , std::function<
                typename R::template ActionPtr<
                    Input
                    , Output
                >(std::size_t)
            > actionCreator
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            DispatchForParallel2<N,0,Input>::apply(
                r
                , std::move(dispatchedInputSource)
                , [&r,&prefix,actionCreator,outputSink](std::size_t idx) -> typename R::template Sink<Input> {
                    auto action = actionCreator(idx);
                    r.registerAction(prefix+"/action_"+std::to_string(idx), action);
                    outputSink(r, r.actionAsSource(action));
                    return r.actionAsSink(action);
                }
            );
        }
        template <
            std::size_t N //how many processor copies
            , class Input
            , class Output 
        >
        static void parallel_fullyIndependent_alreadyDispatched(
            R &r
            , typename R::template Sourceoid<infra::VariantRepeat<N, Input>> const &dispatchedInputSource
            , std::function<
                typename R::template ActionPtr<
                    Input
                    , Output
                >(std::size_t)
            > actionCreator
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            DispatchForParallel2<N,0,Input>::applySourceoid(
                r
                , dispatchedInputSource
                , [&r,&prefix,actionCreator,outputSink](std::size_t idx) -> typename R::template Sink<Input> {
                    auto action = actionCreator(idx);
                    r.registerAction(prefix+"/action_"+std::to_string(idx), action);
                    outputSink(r, r.actionAsSource(action));
                    return r.actionAsSink(action);
                }
            );
        }
        template <
            std::size_t N //how many processor copies
            , class Input
            , class Output 
        >
        static void parallel_fullyIndependent_alreadyDispatched_arrayVersion(
            R &r
            , typename R::template Source<infra::VariantRepeat<N, Input>> &&dispatchedInputSource
            , std::array<
                typename R::template ActionPtr<
                    Input
                    , Output
                >
                , N
            > const &actionArray
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            parallel_fullyIndependent_alreadyDispatched<N, Input, Output>(
                r 
                , std::move(dispatchedInputSource)
                , [&actionArray](std::size_t idx) {
                    return actionArray[idx];
                }
                , outputSink
                , prefix
            );
        }
        template <
            std::size_t N //how many processor copies
            , class Input
            , class Output 
        >
        static void parallel_fullyIndependent_alreadyDispatched_arrayVersion(
            R &r
            , typename R::template Sourceoid<infra::VariantRepeat<N, Input>> const &dispatchedInputSource
            , std::array<
                typename R::template ActionPtr<
                    Input
                    , Output
                >
                , N
            > const &actionArray
            , typename R::template Sinkoid<Output> outputSink
            , std::string const &prefix
        ) {
            parallel_fullyIndependent_alreadyDispatched<N, Input, Output>(
                r 
                , dispatchedInputSource
                , [&actionArray](std::size_t idx) {
                    return actionArray[idx];
                }
                , outputSink
                , prefix
            );
        }

        class RoundRobinDispatcher {
        private:
            std::mutex mutex_;
            std::size_t idx_;
        public:
            RoundRobinDispatcher() : mutex_(), idx_(0) {}
            RoundRobinDispatcher(RoundRobinDispatcher &&d) : mutex_(), idx_(std::move(d.idx_)) {}
            template <class T>
            std::size_t call(std::size_t total, T const &data) {
                std::lock_guard<std::mutex> _(mutex_);
                auto ret = idx_++;
                idx_ = idx_%total;
                return ret;
            }
        };
        class RandomizedDispatcher {
        public:
            RandomizedDispatcher() {
                std::srand(std::time(nullptr));
            }
            template <class T>
            std::size_t call(std::size_t total, T const &data) {
                return ((std::size_t) std::rand())%total;
            }
        };
        
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
                using M = typename R::AppType;
                using ClockF = typename AppClockHelper<M>::Facility;
                using ClockFInput = typename ClockF::template FacilityInput<A>;
                class GenKey {
                private:
                    DurationsGen durationsGen_;
                public:
                    GenKey(DurationsGen &&g) : durationsGen_(std::move(g)) {}
                    typename M::template Key<ClockFInput> operator()(typename M::template Key<A> &&k) {
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
                auto genKey = M::template liftPure<typename M::template Key<A>>(
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
                    auto retrieveData = M::template liftPure<typename M::template KeyedData<ClockFInput,B>>(
                        [](typename M::template KeyedData<ClockFInput,B> &&d) 
                            -> typename M::template KeyedData<A,B>
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
                (std::is_same_v<
                    typename R::AppType 
                    , infra::SinglePassIterationApp<TheEnvironment>
                >
                ||
                std::is_same_v<
                    typename R::AppType 
                    , infra::TopDownSinglePassIterationApp<TheEnvironment>
                >)
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
            if constexpr (std::is_same_v<
                typename R::AppType 
                , infra::SinglePassIterationApp<TheEnvironment>
            >) {
                r.finalize();
            } else {
                auto step = r.finalizeForInterleaving();
                while (!ret) {
                    if (!step()) {
                        break;
                    }
                }
            }

            return ret;
        }
        template <class T>
        static auto singlePassTestRunOneUpdate(
            std::function<
                typename R::template ImporterPtr<T>()
            > importerGen
        ) ->  std::optional<infra::WithTime<T, typename R::AppType::TimePoint>> 
        {
            static_assert(
                (std::is_same_v<
                    typename R::AppType 
                    , infra::SinglePassIterationApp<TheEnvironment>
                >
                ||
                std::is_same_v<
                    typename R::AppType 
                    , infra::TopDownSinglePassIterationApp<TheEnvironment>
                >)
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
        
        template <class Input, class Output>
        class OnOrderFacilityAPIWrapperClient {
        public:
            virtual ~OnOrderFacilityAPIWrapperClient() {}
            virtual void apiCallback(typename M::template KeyedData<Input, Output> &&) = 0;
        };
        template <class Input, class Output>
        class OnOrderFacilityAPIWrapper {
        private:
            TheEnvironment *env_;
            OnOrderFacilityAPIWrapperClient<Input,Output> *client_;
            std::function<void(typename M::template Key<Input> &&)> invoker_;
        public:
            OnOrderFacilityAPIWrapper(
                R &r
                , std::string const &wrapperPrefix
                , typename R::template FacilitioidConnector<Input, Output> const &facility
                , OnOrderFacilityAPIWrapperClient<Input,Output> *client
            ) : env_(r.environment()), client_(client), invoker_()
            {
                auto importer = M::template triggerImporter<typename M::template Key<Input>>();
                r.registerImporter(wrapperPrefix+"/trigger", std::get<0>(importer));
                invoker_ = std::get<1>(importer);
                auto exporter = M::template pureExporter<typename M::template KeyedData<Input,Output>>(
                    [this](typename M::template KeyedData<Input,Output> &&data) {
                        client_->apiCallback(std::move(data));
                    }
                );
                r.registerExporter(wrapperPrefix+"/apiCallback", exporter);
                facility(r, r.importItem(std::get<0>(importer)), r.exporterAsSink(exporter));
            }
            ~OnOrderFacilityAPIWrapper() = default;
            OnOrderFacilityAPIWrapper(OnOrderFacilityAPIWrapper const &) = delete;
            OnOrderFacilityAPIWrapper &operator=(OnOrderFacilityAPIWrapper const &) = delete;
            OnOrderFacilityAPIWrapper(OnOrderFacilityAPIWrapper &&) = default;
            OnOrderFacilityAPIWrapper &operator=(OnOrderFacilityAPIWrapper &&) = default;
            void call(typename M::template Key<Input> &&key) {
                invoker_(std::move(key));
            }
            void call(Input &&input) {
                invoker_(infra::withtime_utils::keyify<Input,TheEnvironment>(std::move(input)));
            }
            template <class CounterMarkType=NotConstructibleStruct>
            void callWithCounterAsID(Input &&input) {
                invoker_(CommonFlowUtilComponents<M>::template keyifyThroughCounterFunc<Input,CounterMarkType>(env_, std::move(input)));
            }
        };

        template <class Input, class Output, class DelayGenerator, class CalculateLogic>
        static auto delayBasedFacilitioid(
            std::string const &prefix
            , DelayGenerator &&delayGenerator
            , CalculateLogic &&calculateLogic 
        ) -> typename R::template FacilitioidConnector<Input, Output>
        {
            return [prefix,delayGenerator=std::move(delayGenerator),calculateLogic=std::move(calculateLogic)](
                R &r
                , typename R::template Source<typename M::template Key<Input>> &&source
                , std::optional<typename R::template Sink<typename M::template KeyedData<Input,Output>>> const &sink)
            mutable {
                auto timer = AppClockHelper<M>::Facility::template createClockCallback<
                    Input
                    , std::tuple<typename M::TimePoint, std::size_t, std::size_t>
                >(
                    [](typename M::TimePoint tp, std::size_t idx, std::size_t total) 
                        -> std::tuple<typename M::TimePoint, std::size_t, std::size_t>
                    {
                        return {std::move(tp), idx, total};
                    }
                );
                r.registerOnOrderFacility(prefix+"/timer", timer);
                auto convertInput = M::template liftPure<typename M::template Key<Input>>(
                    [delayGenerator=std::move(delayGenerator)](typename M::template Key<Input> &&input)
                        mutable -> typename M::template Key<typename AppClockHelper<M>::Facility::template FacilityInput<Input>>
                    {
                        return {
                            input.id()
                            , typename AppClockHelper<M>::Facility::template FacilityInput<Input> {
                                input.key() 
                                , delayGenerator(input.key())
                            }
                        };
                    }
                );
                r.registerAction(prefix+"/convertInput", convertInput);
                if (!sink) {
                    auto discard = M::template trivialExporter<
                        typename M::template KeyedData<
                            typename AppClockHelper<M>::Facility::template FacilityInput<Input>
                            , std::tuple<typename M::TimePoint, std::size_t, std::size_t>
                        >
                    >();
                    r.registerExporter(prefix+"/discard", discard);
                    r.placeOrderWithFacility(
                        r.execute(convertInput, std::move(source))
                        , timer
                        , r.exporterAsSink(discard)
                    );
                } else {
                    auto convertOutput = M::template liftPure<
                        typename M::template KeyedData<
                            typename AppClockHelper<M>::Facility::template FacilityInput<Input>
                            , std::tuple<typename M::TimePoint, std::size_t, std::size_t>
                        >
                    >(
                        [calculateLogic=std::move(calculateLogic)](typename M::template KeyedData<
                            typename AppClockHelper<M>::Facility::template FacilityInput<Input>
                            , std::tuple<typename M::TimePoint, std::size_t, std::size_t>
                        > &&x) mutable -> typename M::template KeyedData<
                            Input, Output
                        > {
                            auto const &input = x.key.key();
                            return typename M::template KeyedData<Input, Output> {
                                typename M::template Key<Input> {
                                    x.key.id(), input.inputData
                                }
                                , calculateLogic(
                                    input.inputData
                                    , std::get<0>(x.data)
                                    , std::get<1>(x.data)
                                    , std::get<2>(x.data)
                                )
                            };
                        }
                    );
                    r.registerAction(prefix+"/convertOutput", convertOutput);
                    r.placeOrderWithFacility(
                        r.execute(convertInput, std::move(source))
                        , timer
                        , r.actionAsSink(convertOutput)
                    );
                    r.connect(r.actionAsSource(convertOutput), *sink);
                }
            };
        }

        template <class A>
        static std::future<void> getNofiticationFutureForFirstPieceOfInput(
            R &r
            , typename R::template Sourceoid<A> const &source 
            , std::string const &prefix
        ) {
            auto promise = std::make_shared<std::promise<void>>();
            auto action = M::template liftPure<A>(
                [promise](A &&) -> basic::VoidStruct {
                    try {
                        promise->set_value();
                    } catch (...) {}
                    return basic::VoidStruct {};
                }
                , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
            );
            r.registerAction(prefix+"/set_future", action);
            source(r, r.actionAsSink(action));
            auto discard = M::template trivialExporter<basic::VoidStruct>();
            r.registerExporter(prefix+"/discard_set_future_output", discard);
            r.exportItem(discard, r.actionAsSource(action));
            return promise->get_future();
        }
        template <class A>
        static std::future<A> getTypedFutureForFirstPieceOfInput(
            R &r
            , typename R::template Sourceoid<A> const &source 
            , std::string const &prefix
        ) {
            auto promise = std::make_shared<std::promise<A>>();
            auto action = M::template liftPure<A>(
                [promise](A &&a) -> basic::VoidStruct {
                    try {
                        promise->set_value(std::move(a));
                    } catch (...) {}
                    return basic::VoidStruct {};
                }
                , infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true)
            );
            r.registerAction(prefix+"/set_future", action);
            source(r, r.actionAsSink(action));
            auto discard = M::template trivialExporter<basic::VoidStruct>();
            r.registerExporter(prefix+"/discard_set_future_output", discard);
            r.exportItem(discard, r.actionAsSource(action));
            return promise->get_future();
        }

        template <class A, class B, bool MultiCallback=false>
        static auto syntheticRemoteFacility(
            std::string const &prefix
            , typename R::template Sinkoid<typename M::template Key<A>> const &outgoingPoint 
            , typename R::template Sourceoid<typename M::template Key<
                std::conditional_t<
                    MultiCallback, std::tuple<B, bool>, B
                >
            >> const &incomingPoint
        ) -> typename R::template FacilitioidConnector<A,B>
        {
            static_assert(infra::app_classification_v<M> == infra::AppClassification::RealTime, "syntheticRemoteFacility is only supported in RealTime app");
            return [prefix,outgoingPoint,incomingPoint](
                R &r 
                , typename R::template Source<typename M::template Key<A>> &&source 
                , std::optional<typename R::template Sink<typename M::template KeyedData<A,B>>> const &sink
            ) {
                if (sink) {                        
                    auto theMap = std::make_shared<std::unordered_map<
                        typename TheEnvironment::IDType 
                        , A 
                        , typename TheEnvironment::IDHash 
                    >>();
                    auto theMapMutex = std::make_shared<std::mutex>();
                    r.preservePointer(theMap);
                    r.preservePointer(theMapMutex);

                    auto outgoingConverter = M::template liftPure<typename M::template Key<A>>(
                        [theMap,theMapMutex](typename M::template Key<A> &&x) -> typename M::template Key<A> {
                            {
                                std::lock_guard<std::mutex> _(*theMapMutex);
                                (*theMap)[x.id()] = infra::withtime_utils::makeValueCopy<A>(x.key());
                            }
                            return std::move(x);
                        }
                    );
                    r.registerAction(prefix+"/outgoingConverter", outgoingConverter);
                    
                    if constexpr (MultiCallback) {
                        auto incomingConverterStep1 = M::template liftPure<typename M::template Key<std::tuple<B,bool>>>(
                            [](typename M::template Key<std::tuple<B,bool>> &&x) -> typename M::template KeyedData<std::monostate, std::tuple<B,bool>> {
                                return {
                                    {std::move(x.id()), std::monostate {}}
                                    , std::move(x.key())
                                };
                            }
                        );
                        auto incomingConverterStep2 = M::template kleisli<typename M::template KeyedData<std::monostate, std::tuple<B,bool>>>(
                            [theMap,theMapMutex](typename M::template InnerData<typename M::template KeyedData<std::monostate, std::tuple<B,bool>>> &&x) -> typename M::template Data<typename M::template KeyedData<A, B>> {
                                std::lock_guard<std::mutex> _(*theMapMutex);
                                auto id = std::move(x.timedData.value.key.id());
                                auto iter = theMap->find(id);
                                if (iter == theMap->end()) {
                                    return std::nullopt;
                                }
                                if (x.timedData.finalFlag || std::get<1>(x.timedData.value.data)) {
                                    typename M::template KeyedData<A, B> ret {
                                        {std::move(id), std::move(iter->second)}
                                        , std::move(std::get<0>(x.timedData.value.data))
                                    };
                                    theMap->erase(iter);
                                    return typename M::template InnerData<typename M::template KeyedData<A, B>> {
                                        x.environment 
                                        , {
                                            x.environment->resolveTime()
                                            , std::move(ret)
                                            , true
                                        }
                                    };
                                } else {
                                    typename M::template KeyedData<A, B> ret {
                                        {std::move(id), infra::withtime_utils::makeValueCopy<A>(iter->second)}
                                        , std::move(std::get<0>(x.timedData.value.data))
                                    };
                                    return typename M::template InnerData<typename M::template KeyedData<A, B>> {
                                        x.environment 
                                        , {
                                            x.environment->resolveTime()
                                            , std::move(ret)
                                            , false
                                        }
                                    };
                                }
                            }
                        );
                        r.registerAction(prefix+"/incomingConverterStep1", incomingConverterStep1);
                        r.registerAction(prefix+"/incomingConverterStep2", incomingConverterStep2);

                        outgoingPoint(r, r.execute(outgoingConverter, std::move(source)));
                        incomingPoint(r, r.actionAsSink(incomingConverterStep1));
                        auto s = r.execute(incomingConverterStep2, r.actionAsSource(incomingConverterStep1));

                        r.connect(s.clone(), *sink);
                    } else {
                        auto incomingConverterStep1 = M::template liftPure<typename M::template Key<B>>(
                            [](typename M::template Key<B> &&x) -> typename M::template KeyedData<std::monostate, B> {
                                return {
                                    {std::move(x.id()), std::monostate {}}
                                    , std::move(x.key())
                                };
                            }
                        );
                        auto incomingConverterStep2 = M::template kleisli<typename M::template KeyedData<std::monostate, B>>(
                            [theMap,theMapMutex](typename M::template InnerData<typename M::template KeyedData<std::monostate, B>> &&x) -> typename M::template Data<typename M::template KeyedData<A, B>> {
                                std::lock_guard<std::mutex> _(*theMapMutex);
                                auto id = std::move(x.timedData.value.key.id());
                                auto iter = theMap->find(id);
                                if (iter == theMap->end()) {
                                    return std::nullopt;
                                }
                                typename M::template KeyedData<A, B> ret {
                                    {std::move(id), std::move(iter->second)}
                                    , std::move(x.timedData.value.data)
                                };
                                theMap->erase(iter);
                                return typename M::template InnerData<typename M::template KeyedData<A, B>> {
                                    x.environment 
                                    , {
                                        x.environment->resolveTime()
                                        , std::move(ret)
                                        , true
                                    }
                                };
                            }
                        );
                        r.registerAction(prefix+"/incomingConverterStep1", incomingConverterStep1);
                        r.registerAction(prefix+"/incomingConverterStep2", incomingConverterStep2);

                        outgoingPoint(r, r.execute(outgoingConverter, std::move(source)));
                        incomingPoint(r, r.actionAsSink(incomingConverterStep1));
                        auto s = r.execute(incomingConverterStep2, r.actionAsSource(incomingConverterStep1));

                        r.connect(s.clone(), *sink);
                    }
                } else {
                    auto discard = M::template trivialExporter<typename M::template Key<
                        std::conditional_t<MultiCallback, std::tuple<B, bool>, B>
                    >>();
                    r.registerExporter(prefix+"/discard", discard);
                    outgoingPoint(r, std::move(source));
                    incomingPoint(r, r.exporterAsSink(discard));
                }
            };
        }

        template <class T>
        static typename R::template Source<T> pacer(
            R &r
            , std::string const &prefix 
            , typename R::template Sourceoid<T> const &originalSource
            , typename M::Duration const &timeWindowLength
            , uint64_t maxCountInWindow
            , bool paceByDiscard
        ) {
            if (paceByDiscard) {
                class Discarder {
                private:
                    typename M::Duration timeWindowLength_;
                    uint64_t maxCountInWindow_;
                    std::deque<typename M::TimePoint> pastEvents_;
                public:
                    Discarder(typename M::Duration const &timeWindowLength, uint64_t maxCountInWindow) 
                        : timeWindowLength_(timeWindowLength), maxCountInWindow_(maxCountInWindow), pastEvents_() 
                    {}
                    ~Discarder() = default;
                    Discarder(Discarder const &) = delete;
                    Discarder &operator=(Discarder const &) = delete;
                    Discarder(Discarder &&) = default;
                    Discarder &operator=(Discarder &&) = default;

                    typename M::template Data<T> operator()(typename M::template InnerData<T> &&input) {
                        auto now = input.environment->now();
                        while (!pastEvents_.empty()) {
                            if (pastEvents_.front()+timeWindowLength_ < now) {
                                pastEvents_.pop_front();
                            } else {
                                break;
                            }
                        }
                        if (pastEvents_.size() < maxCountInWindow_) {
                            pastEvents_.push_back(now);
                            return typename M::template InnerData<T> {std::move(input)};
                        } else {
                            return std::nullopt;
                        }
                    }
                };

                auto discarder = infra::GenericLift<M>::lift(Discarder {timeWindowLength, maxCountInWindow});
                r.registerAction(prefix+"/discarder", discarder);
                
                originalSource(r, r.actionAsSink(discarder));
                return r.actionAsSource(discarder);
            } else {
                using Fac = typename AppClockHelper<M>::Facility;
                class PacerBuffer {
                private:
                    typename M::Duration timeWindowLength_;
                    uint64_t maxCountInWindow_;
                    std::deque<typename M::TimePoint> pastEvents_;
                    std::deque<std::tuple<T, bool>> buffer_;
                public:
                    PacerBuffer(typename M::Duration const &timeWindowLength, uint64_t maxCountInWindow) 
                        : timeWindowLength_(timeWindowLength), maxCountInWindow_(maxCountInWindow), pastEvents_(), buffer_() 
                    {}
                    ~PacerBuffer() = default;
                    PacerBuffer(PacerBuffer const &) = delete;
                    PacerBuffer &operator=(PacerBuffer const &) = delete;
                    PacerBuffer(PacerBuffer &&) = default;
                    PacerBuffer &operator=(PacerBuffer &&) = default;

                    typename M::template Data<
                        std::tuple<
                            std::vector<T>
                            , std::tuple<bool, typename M::Duration>
                        >
                    > operator()(
                        typename M::template InnerData<
                            std::variant<
                                T
                                , typename M::template KeyedData<typename Fac::template FacilityInput<VoidStruct>, VoidStruct>
                            >
                        > &&input
                    ) {
                        auto now = input.environment->now();
                        while (!pastEvents_.empty()) {
                            if (pastEvents_.front()+timeWindowLength_ < now) {
                                pastEvents_.pop_front();
                            } else {
                                break;
                            }
                        }
                        if (input.timedData.value.index() == 0) {
                            if (buffer_.empty() && pastEvents_.size() < maxCountInWindow_) {
                                pastEvents_.push_back(now);
                                return typename M::template InnerData<
                                    std::tuple<
                                        std::vector<T>
                                        , std::tuple<bool, typename M::Duration>
                                    > 
                                > {
                                    input.environment
                                    , {
                                        now
                                        , {{std::move(std::get<0>(input.timedData.value))}, {true, typename M::Duration {}}}
                                        , input.timedData.finalFlag
                                    }
                                };
                            } else {
                                buffer_.push_back({
                                    std::move(std::get<0>(input.timedData.value))
                                    , input.timedData.finalFlag
                                });
                            }
                        }

                        std::vector<T> res;
                        bool final = false;
                        while (!buffer_.empty() && pastEvents_.size() < maxCountInWindow_) {
                            res.push_back(std::move(std::get<0>(buffer_.front())));
                            if (std::get<1>(buffer_.front())) {
                                final = true;
                            }
                            pastEvents_.push_back(now);
                            buffer_.pop_front();
                        }
                        bool hasRes = !res.empty();
                        typename M::Duration waitTime {};
                        if (!hasRes) {
                            auto peIter = pastEvents_.begin();
                            if (peIter != pastEvents_.end()) {
                                auto t1 = *peIter;
                                ++peIter;
                                if (peIter != pastEvents_.end()) {
                                    waitTime = *peIter-t1;
                                } else {
                                    waitTime = now-t1;
                                }
                            }
                        }
                        return typename M::template InnerData<
                            std::tuple<
                                std::vector<T>
                                , std::tuple<bool, typename M::Duration>
                            >
                        > {
                            input.environment
                            , {
                                now
                                , {std::move(res), {hasRes, waitTime}}
                                , final
                            }
                        };
                    }
                };

                auto pacerBuffer = infra::GenericLift<M>::lift(PacerBuffer {timeWindowLength, maxCountInWindow});
                r.registerAction(prefix+"/pacerBuffer", pacerBuffer);

                auto t2v = M::template dispatchTupleAction<
                    std::vector<T>
                    , std::tuple<bool, typename M::Duration>
                >();
                r.registerAction(prefix+"/t2v", t2v);

                auto createKey = infra::GenericLift<M>::lift(
                    [](std::tuple<bool, typename M::Duration> &&x) -> std::optional<typename M::template Key<typename Fac::template FacilityInput<VoidStruct>>> {
                        if (std::get<0>(x)) {
                            return std::nullopt;
                        } else {
                            return {{typename Fac::template FacilityInput<VoidStruct> {
                                VoidStruct {}
                                , {std::get<1>(x)}
                            }}};
                        }
                    }
                );
                r.registerAction(prefix+"/createKey", createKey);

                auto fac = Fac::template createClockCallback<VoidStruct, VoidStruct>(
                    [](typename M::TimePoint const &, std::size_t, std::size_t) {
                        return VoidStruct {};
                    }
                );
                r.registerOnOrderFacility(prefix+"/clockFacility", fac);

                originalSource(r, r.actionAsSink_2_0(pacerBuffer));
                r.placeOrderWithFacility(
                    r.actionAsSource(createKey)
                    , fac
                    , r.actionAsSink_2_1(pacerBuffer)
                );
                r.connect(r.actionAsSource(pacerBuffer), r.actionAsSink(t2v));
                r.connect_2_1(r.actionAsSource(t2v), r.actionAsSink(createKey));

                auto d = CommonFlowUtilComponents<M>::template dispatchOneByOne<T>();
                r.registerAction(prefix+"/dispatcher", d);
                r.connect_2_0(r.actionAsSource(t2v), r.actionAsSink(d));

                return r.actionAsSource(d);
            }
        }
    };

} } } }

#endif
