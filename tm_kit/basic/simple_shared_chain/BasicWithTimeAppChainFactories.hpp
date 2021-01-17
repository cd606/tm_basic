#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_BASIC_WITH_TIME_APP_CHAIN_FACTORIES_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_BASIC_WITH_TIME_APP_CHAIN_FACTORIES_HPP_

#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class Env>
    class BasicWithTimeAppChainFactories {
    public:
        template <class ChainItemFolder, class TriggerT, class ResultTransformer>
        static ChainReaderActionFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, TriggerT, ResultTransformer>
        chainReaderActionFactory() {
            return []() {
                return infra::BasicWithTimeApp<Env>::template emptyAction<TriggerT, std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>>();
            };
        }
        template <class ChainItemFolder, class ResultTransformer>
        static ChainReaderImporterFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, ResultTransformer>
        chainReaderImporterFactory() {
            return []() {
                return infra::BasicWithTimeApp<Env>::template vacuousImporter<std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>>();
            };
        }
        template <class ChainItemFolder, class InputHandler, class IdleLogic>
        static ChainWriterOnOrderFacilityWithExternalEffectsFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, InputHandler, IdleLogic>
        chainWriterOnOrderFacilityWithExternalEffectsFactory() {
            return []() {
                return infra::BasicWithTimeApp<Env>::template emptyOnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename OffChainUpdateTypeExtractor<IdleLogic>::T>();
            };
        }
        template <class ChainItemFolder, class InputHandler>
        static ChainWriterOnOrderFacilityFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, InputHandler>
        chainWriterOnOrderFacilityFactory() {
            return []() {
                return infra::BasicWithTimeApp<Env>::template emptyOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>();
            };
        };
    };

    template <class M>
    class BasicWithTimeAppChainCreator {};

    template <class Env>
    class BasicWithTimeAppChainCreator<infra::BasicWithTimeApp<Env>> {
    public:
        template <class ChainData, class ChainItemFolder, class TriggerT=void, class ResultTransformer=void>
        static auto readerFactory(Env *, std::string const &, simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy(), ChainItemFolder &&=ChainItemFolder(), std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>())
            -> std::conditional_t<
                std::is_same_v<TriggerT, void>
                , ChainReaderImporterFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, ResultTransformer>
                , ChainReaderActionFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, TriggerT, ResultTransformer>
            >
        {
            if constexpr (std::is_same_v<TriggerT, void>) {
                return BasicWithTimeAppChainFactories<Env>::template chainReaderImporterFactory<ChainItemFolder, ResultTransformer>();
            } else {
                return BasicWithTimeAppChainFactories<Env>::template chainReaderActionFactory<ChainItemFolder, TriggerT, ResultTransformer>();
            }
        }
        template <class ChainData, class ChainItemFolder, class InputHandler, class IdleLogic=void>
        static auto writerFactory(Env *, std::string const &, simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy(), ChainItemFolder &&=ChainItemFolder(), InputHandler &&=InputHandler(), std::conditional_t<std::is_same_v<IdleLogic,void>,bool,IdleLogic> &&=std::conditional_t<std::is_same_v<IdleLogic,void>,bool,IdleLogic>())
            -> std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , ChainWriterOnOrderFacilityFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, InputHandler>
                , ChainWriterOnOrderFacilityWithExternalEffectsFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, InputHandler, IdleLogic>
            >
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return BasicWithTimeAppChainFactories<Env>::template chainWriterOnOrderFacilityFactory<ChainItemFolder, InputHandler>();
            } else {
                return BasicWithTimeAppChainFactories<Env>::template chainWriterOnOrderFacilityWithExternalEffectsFactory<ChainItemFolder, InputHandler, IdleLogic>();
            }
        }
        template <class ChainData, class ExtraData>
        static void writeExtraData(Env *, std::string const &, std::string const &, ExtraData const &) {}
        template <class ChainData, class ExtraData>
        static std::optional<ExtraData> readExtraData(Env *, std::string const &, std::string const &) {
            return std::nullopt;
        }
    };
} } } } } 

#endif