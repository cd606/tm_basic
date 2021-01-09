#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_BASIC_WITH_TIME_APP_CHAIN_FACTORIES_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_BASIC_WITH_TIME_APP_CHAIN_FACTORIES_HPP_

#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class Env>
    class BasicWithTimeAppChainFactories {
    public:
        template <class ChainItemFolder, class TriggerT>
        static ChainReaderActionFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, TriggerT>
        chainReaderActionFactory() {
            return []() {
                return infra::BasicWithTimeApp<Env>::template emptyAction<TriggerT, typename ChainItemFolder::ResultType>();
            };
        }
        template <class ChainItemFolder>
        static ChainReaderImporterFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder>
        chainReaderImporterFactory() {
            return []() {
                return infra::BasicWithTimeApp<Env>::template vacuousImporter<typename ChainItemFolder::ResultType>();
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
        template <class ChainData, class ChainItemFolder, class TriggerT=void>
        static auto readerFactory(Env *, std::string const &, simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy(), ChainItemFolder &&=ChainItemFolder())
            -> std::conditional_t<
                std::is_same_v<TriggerT, void>
                , ChainReaderImporterFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder>
                , ChainReaderActionFactory<infra::BasicWithTimeApp<Env>, ChainItemFolder, TriggerT>
            >
        {
            if constexpr (std::is_same_v<TriggerT, void>) {
                return BasicWithTimeAppChainFactories<Env>::template chainReaderImporterFactory<ChainItemFolder>();
            } else {
                return BasicWithTimeAppChainFactories<Env>::template chainReaderActionFactory<ChainItemFolder, TriggerT>();
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
    };
} } } } } 

#endif