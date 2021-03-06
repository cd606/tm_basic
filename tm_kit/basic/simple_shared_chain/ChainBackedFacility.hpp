#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_BACKED_FACILITY_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_BACKED_FACILITY_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    //IDAndFinalFlagExtractor must have the following two methods:
    //extract(ChainData const &) -> std::vector<std::tuple<ID, bool>> (bool is the final flag)
    //writeSucceeded(typename InputHandler::ResponseType const &) -> bool
    //IDAndFinalFlagExtractor is responsible for managing its own locks, if any
    template <class R, class ChainData, class ChainItemFolder, class InputHandler, class IDAndFinalFlagExtractor, class IdleLogic=void>
    struct ChainBackedFacilityOutput {
        typename R::template FacilitioidConnector<typename InputHandler::InputType, std::optional<ChainData>> facility;
        typename R::template Sourceoid<typename OffChainUpdateTypeExtractor<IdleLogic>::T> offChainUpdateSource;
        std::string registeredNameForFacilitioidConnector;
    };

    namespace chain_backed_facility_internal {
        template <class M, class ChainData, class ChainItemFolder, class InputHandler, class IDAndFinalFlagExtractor>
        class ChainBackedFacilityLocalVIEFacility final : public M::template AbstractIntegratedVIEOnOrderFacilityWithPublish<
            typename InputHandler::InputType
            , std::optional<ChainData>
            , basic::SingleLayerWrapper<
                std::variant<
                    std::optional<ChainData>
                    , std::tuple<typename M::EnvironmentType::IDType, bool>
                >
            >
            , typename M::template Key<typename InputHandler::InputType>
        >
        {
        private:
            std::shared_ptr<IDAndFinalFlagExtractor> extractor_;
            using VIEExtraInput = basic::SingleLayerWrapper<
                std::variant<
                    std::optional<ChainData>
                    , std::tuple<typename M::EnvironmentType::IDType, bool>
                >
            >;
            using P=typename M::template AbstractIntegratedVIEOnOrderFacilityWithPublish<typename InputHandler::InputType,std::optional<ChainData>,VIEExtraInput,typename M::template Key<typename InputHandler::InputType>>;
        public:
            ChainBackedFacilityLocalVIEFacility(std::shared_ptr<IDAndFinalFlagExtractor> const &extractor) : extractor_(extractor) {}
            virtual ~ChainBackedFacilityLocalVIEFacility() {}
            virtual void start(typename M::EnvironmentType *env) override final {}
            virtual void handle(typename M::template InnerData<typename M::template Key<typename InputHandler::InputType>> &&queryInput) override final {
                TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(queryInput.environment, ":handleRequest");
                this->P::ImporterParent::publish(
                    std::move(queryInput)
                );
            }
            virtual void handle(typename M::template InnerData<VIEExtraInput> &&data) override final {
                TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(data.environment, ":feedResult");
                auto *env = data.environment;
                std::visit([env,this](auto &&v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::optional<ChainData>>) {
                        if (!v) {
                            return;
                        }
                        std::vector<std::tuple<typename M::EnvironmentType::IDType, bool>> idsAndFinalFlags = extractor_->extract(*v);
                        switch (idsAndFinalFlags.size()) {
                        case 0:
                            return;
                        case 1:
                            {
                                auto &x = idsAndFinalFlags[0];
                                this->P::FacilityParent::publish(
                                    env
                                    , typename M::template Key<std::optional<ChainData>> {
                                        std::move(std::get<0>(x))
                                        , std::move(v)
                                    }
                                    , std::get<1>(x)
                                );
                            }
                            break;
                        default:
                            {
                                for (auto &x : idsAndFinalFlags) {
                                    this->P::FacilityParent::publish(
                                        env
                                        , typename M::template Key<std::optional<ChainData>> {
                                            std::move(std::get<0>(x))
                                            , {infra::withtime_utils::makeValueCopy(*v)}
                                        }
                                        , std::get<1>(x)
                                    );
                                }
                            }
                            break;
                        }
                    } else if constexpr (std::is_same_v<T, std::tuple<typename M::EnvironmentType::IDType, bool>>) {
                        if (!std::get<1>(v)) {
                            this->P::FacilityParent::publish(
                                env
                                , typename M::template Key<std::optional<ChainData>> {
                                    std::move(std::get<0>(v))
                                    , std::nullopt
                                }
                                , true
                            );
                        }
                    }
                }, std::move(data.timedData.value.value));             
            }
        };
        template <class Env, class ChainData, class ChainItemFolder, class InputHandler, class IDAndFinalFlagExtractor>
        class ChainBackedFacilityLocalVIEFacility<
            infra::TopDownSinglePassIterationApp<Env>
            , ChainData, ChainItemFolder, InputHandler, IDAndFinalFlagExtractor
        > final : public infra::TopDownSinglePassIterationApp<Env>::template AbstractIntegratedVIEOnOrderFacilityWithPublish<
            typename InputHandler::InputType
            , std::optional<ChainData>
            , basic::SingleLayerWrapper<
                std::variant<
                    std::optional<ChainData>
                    , std::tuple<typename Env::IDType, bool>
                >
            >
            , typename infra::TopDownSinglePassIterationApp<Env>::template Key<typename InputHandler::InputType>
        >
        {
        private:
            std::shared_ptr<IDAndFinalFlagExtractor> extractor_;
            using M = infra::TopDownSinglePassIterationApp<Env>;
            using VIEExtraInput = basic::SingleLayerWrapper<
                std::variant<
                    std::optional<ChainData>
                    , std::tuple<typename Env::IDType, bool>
                >
            >;
            using P=typename M::template AbstractIntegratedVIEOnOrderFacilityWithPublish<typename InputHandler::InputType,std::optional<ChainData>,VIEExtraInput,typename M::template Key<typename InputHandler::InputType>>;
            std::deque<typename M::template InnerData<typename M::template Key<typename InputHandler::InputType>>> topDownImporterPublishQueue_;
        public:
            ChainBackedFacilityLocalVIEFacility(std::shared_ptr<IDAndFinalFlagExtractor> const &extractor) : extractor_(extractor), topDownImporterPublishQueue_() {}
            virtual ~ChainBackedFacilityLocalVIEFacility() {}
            virtual void start(typename M::EnvironmentType *env) override final {}
            virtual void handle(typename M::template InnerData<typename M::template Key<typename InputHandler::InputType>> &&queryInput) override final {
                TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(queryInput.environment, ":handleRequest");
                topDownImporterPublishQueue_.push_back(std::move(queryInput));
            }
            virtual void handle(typename M::template InnerData<VIEExtraInput> &&data) override final {
                TM_INFRA_FACILITY_TRACER_WITH_SUFFIX(data.environment, ":feedResult");
                auto *env = data.environment;
                std::visit([env,this](auto &&v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::optional<ChainData>>) {
                        if (!v) {
                            return;
                        }
                        std::vector<std::tuple<typename Env::IDType, bool>> idsAndFinalFlags = extractor_->extract(*v);
                        switch (idsAndFinalFlags.size()) {
                        case 0:
                            return;
                        case 1:
                            {
                                auto &x = idsAndFinalFlags[0];
                                this->P::FacilityParent::publish(
                                    env
                                    , typename M::template Key<std::optional<ChainData>> {
                                        std::move(std::get<0>(x))
                                        , std::move(v)
                                    }
                                    , std::get<1>(x)
                                );
                            }
                            break;
                        default:
                            {
                                for (auto &x : idsAndFinalFlags) {
                                    this->P::FacilityParent::publish(
                                        env
                                        , typename M::template Key<std::optional<ChainData>> {
                                            std::move(std::get<0>(x))
                                            , {infra::withtime_utils::makeValueCopy(*v)}
                                        }
                                        , std::get<1>(x)
                                    );
                                }
                            }
                            break;
                        }
                    } else if constexpr (std::is_same_v<T, std::tuple<typename Env::IDType, bool>>) {
                        if (!std::get<1>(v)) {
                            this->P::FacilityParent::publish(
                                env
                                , typename M::template Key<std::optional<ChainData>> {
                                    std::move(std::get<0>(v))
                                    , std::nullopt
                                }
                                , true
                            );
                        }
                    }
                }, std::move(data.timedData.value.value));             
            }
            virtual std::tuple<bool, typename M::template Data<typename M::template Key<typename InputHandler::InputType>>> generate(
                typename M::template Key<typename InputHandler::InputType> const *notUsed
            ) override final {
                if (topDownImporterPublishQueue_.empty()) {
                    return {true, std::nullopt};
                } else {
                    typename M::template InnerData<typename M::template Key<typename InputHandler::InputType>> ret = std::move(topDownImporterPublishQueue_.front());
                    topDownImporterPublishQueue_.pop_front();
                    return {!(ret.timedData.finalFlag), {std::move(ret)}};
                }
            }
        };
    }

    template <class R, class ChainData, class ChainItemFolder, class InputHandler, class IDAndFinalFlagExtractor, class IdleLogic=void>
    inline auto createChainBackedFacility(
        R &r 
        , std::conditional_t<
            std::is_same_v<IdleLogic, void>
            , ChainWriterOnOrderFacilityFactory<
                typename R::AppType 
                , ChainItemFolder
                , InputHandler
            >
            , ChainWriterOnOrderFacilityWithExternalEffectsFactory<
                typename R::AppType 
                , ChainItemFolder
                , InputHandler
                , IdleLogic
            >
        > chainWriterFactory
        , ChainReaderImporterFactory<
            typename R::AppType
            , TrivialChainDataFetchingFolder<ChainData>
            , void
        > chainReaderFactory
        , std::shared_ptr<IDAndFinalFlagExtractor> const &idAndFinalFlagExtractor
        , std::string const &prefix
    )
    -> ChainBackedFacilityOutput<R, ChainData, ChainItemFolder, InputHandler, IDAndFinalFlagExtractor, IdleLogic> {
        using M = typename R::AppType;
        using Env = typename M::EnvironmentType;

        auto writer = chainWriterFactory();
        if constexpr (std::is_same_v<IdleLogic, void>) {
            r.registerOnOrderFacility(prefix+"/chainWriter", writer);
        } else {
            r.registerOnOrderFacilityWithExternalEffects(prefix+"/chainWriter", writer);
        }

        auto reader = chainReaderFactory();
        r.registerImporter(prefix+"/chainReader", reader);

        r.preservePointer(idAndFinalFlagExtractor);

        using VIEExtraInput = basic::SingleLayerWrapper<
            std::variant<
                std::optional<ChainData>
                , std::tuple<typename Env::IDType, bool>
            >
        >;

        typename R::template VIEOnOrderFacilityPtr<
            typename InputHandler::InputType,std::optional<ChainData>,VIEExtraInput,typename M::template Key<typename InputHandler::InputType>
        > facility = M::vieOnOrderFacility(new chain_backed_facility_internal::ChainBackedFacilityLocalVIEFacility<M,ChainData,ChainItemFolder,InputHandler,IDAndFinalFlagExtractor>(idAndFinalFlagExtractor));
        r.registerVIEOnOrderFacility(prefix+"/facility", facility);

        auto vieFeedback = M::template liftPure2<
            std::optional<ChainData>
            , typename M::template KeyedData<typename InputHandler::InputType, typename InputHandler::ResponseType>
        >(
            [idAndFinalFlagExtractor](std::variant<std::optional<ChainData>, typename M::template KeyedData<typename InputHandler::InputType, typename InputHandler::ResponseType>> &&data) 
            -> VIEExtraInput {
                switch (data.index()) {
                case 0:
                    return {{std::move(std::get<0>(data))}};
                case 1:
                    return {{std::tuple<typename Env::IDType, bool> {std::get<1>(data).key.id(), idAndFinalFlagExtractor->writeSucceeded(std::get<1>(data).data)}}};
                default:
                    return {{(std::optional<ChainData>) std::nullopt}};
                }
            }
        );
        r.registerAction(prefix+"/vieFeedback", vieFeedback);

        r.markStateSharing(vieFeedback, facility, prefix+"/idAndFinalFlagExtractor");
        r.execute(vieFeedback, r.importItem(reader));
        r.placeOrderWithFacility(
            r.vieFacilityAsSource(facility)
            , writer 
            , r.actionAsSink_2_1(vieFeedback)
        );

        r.connect(r.actionAsSource(vieFeedback), r.vieFacilityAsSink(facility));

        if constexpr (std::is_same_v<IdleLogic, void>) {
            return {
                r.vieFacilityConnector(facility)
                , {}
                , r.getRegisteredName(facility)
            };
        } else {
            return {
                r.vieFacilityConnector(facility)
                , R::sourceAsSourceoid(R::facilityWithExternalEffectsAsSource(writer))
                , r.getRegisteredName(facility)
            };
        }
    }

} } } } }

#endif