#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_IN_MEMORY_SHARED_CHAIN_CREATOR_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_IN_MEMORY_SHARED_CHAIN_CREATOR_HPP_

#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/simple_shared_chain/OneShotChainWriter.hpp>
#include <tm_kit/basic/simple_shared_chain/InMemoryWithLockChain.hpp>
#include <tm_kit/basic/simple_shared_chain/InMemoryLockFreeChain.hpp>

#include <unordered_map>
#include <mutex>

#include <boost/algorithm/string.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    template <class App>
    class InMemorySharedChainCreator {
    private:
        std::mutex mutex_;
        std::unordered_map<std::string, std::tuple<void *, std::function<void(void *)>>> chains_;

        inline auto parseDescriptor(std::string const &s)
            -> std::tuple<bool, std::string>
        {
            if (boost::starts_with(s, "lock-free://")) {
                return {true, s};
            } else {
                return {false, s};
            }
        }

        template <class ChainItemFolder, class TriggerT, class Chain>
        inline auto chainReaderHelper(
            typename App::EnvironmentType *env
            , Chain *chain 
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy
            , ChainItemFolder &&folder
        ) 
            -> std::conditional_t<
                std::is_same_v<TriggerT, void>
                , std::shared_ptr<typename App::template Importer<typename ChainItemFolder::ResultType>>
                , std::shared_ptr<typename App::template Action<TriggerT, typename ChainItemFolder::ResultType>> 
            >
        {
            if constexpr (std::is_same_v<TriggerT, void>) {
                return simple_shared_chain::ChainReader<
                    App
                    , Chain
                    , ChainItemFolder
                    , void
                >::importer(
                    chain
                    , pollingPolicy
                    , std::move(folder)
                );
            } else {
                return simple_shared_chain::ChainReader<
                    App
                    , Chain
                    , ChainItemFolder
                    , TriggerT
                >::action(
                    env
                    , chain
                    , std::move(folder)
                );
            }
        }

        template <class ChainItemFolder, class InputHandler, class IdleLogic, class Chain>
        inline auto chainWriterHelper(
            Chain *chain 
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy
            , ChainItemFolder &&folder
            , InputHandler &&inputHandler
            , std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            > &&idleLogic
        ) 
            -> std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , std::shared_ptr<typename App::template OnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>>
                , std::shared_ptr<typename App::template OnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename basic::simple_shared_chain::OffChainUpdateTypeExtractor<IdleLogic>::T>> 
            >
        {
            if constexpr (std::is_same_v<IdleLogic, void>) {
                return simple_shared_chain::ChainWriter<
                    App
                    , Chain
                    , ChainItemFolder
                    , InputHandler
                    , void
                >::onOrderFacility(
                    chain
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(inputHandler)
                    , std::move(idleLogic)
                );
            } else {
                return simple_shared_chain::ChainWriter<
                    App
                    , Chain
                    , ChainItemFolder
                    , InputHandler
                    , IdleLogic
                >::onOrderFacilityWithExternalEffects(
                    chain
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(inputHandler)
                    , std::move(idleLogic)
                );
            }
        }

        template <class ExtraData, class Chain>
        inline auto readExtraDataHelper(
            Chain *chain 
            , std::string const &key
        ) -> std::optional<ExtraData> {
            if constexpr (Chain::SupportsExtraData) {
                return chain->template loadExtraData<ExtraData>(key);
            } else {
                return std::nullopt;
            }
        }

        template <class ExtraData, class Chain>
        inline void writeExtraDataHelper(
            Chain *chain 
            , std::string const &key
            , ExtraData const &data
        ) {
            if constexpr (Chain::SupportsExtraData) {
                chain->template saveExtraData<ExtraData>(key, data);
            }
        }

        template <class T>
        T *getChain(std::string const &name, std::function<T *()> creator) {
            std::lock_guard<std::mutex> _(mutex_);
            auto iter = chains_.find(name);
            if (iter == chains_.end()) {
                T *chain = creator();
                chains_.insert({name, {(void *) chain, [](void *p) {delete ((T *) p);}}});
                return chain;
            } else {
                return (T *) std::get<0>(iter->second);
            }
        }
    public:
        InMemorySharedChainCreator() : mutex_(), chains_() {}
        ~InMemorySharedChainCreator() {
            std::lock_guard<std::mutex> _(mutex_);
            for (auto &item : chains_) {
                std::get<1>(item.second)(std::get<0>(item.second));
            }
            chains_.clear();
        }

        template <class ChainData, class ChainItemFolder, class TriggerT=void>
        auto reader(
            typename App::EnvironmentType *env
            , bool lockFree
            , std::string const &name
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
        )
            -> std::conditional_t<
                std::is_same_v<TriggerT, void>
                , std::shared_ptr<typename App::template Importer<typename ChainItemFolder::ResultType>>
                , std::shared_ptr<typename App::template Action<TriggerT, typename ChainItemFolder::ResultType>> 
            >
        {
            if (lockFree) {
                return chainReaderHelper<ChainItemFolder,TriggerT>(
                    env
                    , getChain<simple_shared_chain::InMemoryLockFreeChain<ChainData>>(
                        name
                        , []() {
                            return new basic::simple_shared_chain::InMemoryLockFreeChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                );
            } else {
                return chainReaderHelper<ChainItemFolder,TriggerT>(
                    env
                    , getChain<simple_shared_chain::InMemoryWithLockChain<ChainData>>(
                        name
                        , []() {
                            return new simple_shared_chain::InMemoryWithLockChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                );
            }
        }

        template <class ChainData, class ChainItemFolder, class TriggerT=void>
        auto reader(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
        )
            -> std::conditional_t<
                std::is_same_v<TriggerT, void>
                , std::shared_ptr<typename App::template Importer<typename ChainItemFolder::ResultType>>
                , std::shared_ptr<typename App::template Action<TriggerT, typename ChainItemFolder::ResultType>> 
            >
        {
            auto parsed = parseDescriptor(descriptor);
            return reader<ChainData,ChainItemFolder,TriggerT>(
                env, std::get<0>(parsed), std::get<1>(parsed), pollingPolicy, std::move(folder)
            );
        }

        template <class ChainData, class ChainItemFolder, class InputHandler, class IdleLogic=void>
        auto writer(
            typename App::EnvironmentType *env
            , bool lockFree
            , std::string const &name
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
            , InputHandler &&inputHandler = InputHandler()
            , std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            > &&idleLogic = std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            >()
        )
            -> std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , std::shared_ptr<typename App::template OnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>>
                , std::shared_ptr<typename App::template OnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename basic::simple_shared_chain::OffChainUpdateTypeExtractor<IdleLogic>::T>> 
            >
        {
            if (lockFree) {
                return chainWriterHelper<ChainItemFolder,InputHandler,IdleLogic>(
                    getChain<simple_shared_chain::InMemoryLockFreeChain<ChainData>>(
                        name
                        , []() {
                            return new basic::simple_shared_chain::InMemoryLockFreeChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(inputHandler)
                    , std::move(idleLogic)
                );
            } else {
                return chainWriterHelper<ChainItemFolder,InputHandler,IdleLogic>(
                    getChain<simple_shared_chain::InMemoryWithLockChain<ChainData>>(
                        name
                        , []() {
                            return new basic::simple_shared_chain::InMemoryWithLockChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(inputHandler)
                    , std::move(idleLogic)
                );
            }
        }

        template <class ChainData, class ChainItemFolder, class InputHandler, class IdleLogic=void>
        auto writer(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
            , InputHandler &&inputHandler = InputHandler()
            , std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            > &&idleLogic = std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            >()
        )
            -> std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , std::shared_ptr<typename App::template OnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>>
                , std::shared_ptr<typename App::template OnOrderFacilityWithExternalEffects<typename InputHandler::InputType, typename InputHandler::ResponseType, typename basic::simple_shared_chain::OffChainUpdateTypeExtractor<IdleLogic>::T>> 
            >
        {
            auto parsed = parseDescriptor(descriptor);
            return writer<ChainData,ChainItemFolder,InputHandler,IdleLogic>(
                env, std::get<0>(parsed), std::get<1>(parsed), pollingPolicy, std::move(folder), std::move(inputHandler), std::move(idleLogic)
            );
        }

        template <class ChainData, class ChainItemFolder, class TriggerT=void>
        auto readerFactory(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
        )
            -> std::conditional_t<
                std::is_same_v<TriggerT, void>
                , basic::simple_shared_chain::ChainReaderImporterFactory<App, ChainItemFolder>
                , basic::simple_shared_chain::ChainReaderActionFactory<App, ChainItemFolder, TriggerT>
            >
        {
            auto f = std::move(folder);
            return [this,env,descriptor,pollingPolicy,f=std::move(f)]() {
                auto f1 = std::move(f);
                return reader<ChainData,ChainItemFolder,TriggerT>(
                    env, descriptor, pollingPolicy, std::move(f1)
                );
            };
        }

        template <class ChainData, class ChainItemFolder, class InputHandler, class IdleLogic=void>
        auto writerFactory(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
            , InputHandler &&inputHandler = InputHandler()
            , std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            > &&idleLogic = std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            >()
        )
            -> std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::simple_shared_chain::ChainWriterOnOrderFacilityFactory<App, ChainItemFolder, InputHandler>
                , basic::simple_shared_chain::ChainWriterOnOrderFacilityWithExternalEffectsFactory<App, ChainItemFolder, InputHandler, IdleLogic>
            >
        {
            ChainItemFolder f = std::move(folder);
            InputHandler h = std::move(inputHandler);
            std::conditional_t<
                std::is_same_v<IdleLogic, void>
                , basic::VoidStruct
                , IdleLogic
            > l = std::move(idleLogic);
            return [this,env,descriptor,pollingPolicy,f=std::move(f),h=std::move(h),l=std::move(l)]() {
                ChainItemFolder f1 = std::move(f);
                InputHandler h1 = std::move(h);
                std::conditional_t<
                    std::is_same_v<IdleLogic, void>
                    , basic::VoidStruct
                    , IdleLogic
                > l1 = std::move(l);
                return writer<ChainData,ChainItemFolder,InputHandler,IdleLogic>(
                    env, descriptor, pollingPolicy, std::move(f1), std::move(h1), std::move(l1)
                );
            };
        }

        template <class ChainData, class ExtraData>
        void writeExtraData(typename App::EnvironmentType *env, std::string const &descriptor, std::string const &key, ExtraData const &extraData) {
            auto parsed = parseDescriptor(descriptor);
            if (std::get<0>(parsed)) {
                writeExtraDataHelper<ExtraData>(
                    getChain<simple_shared_chain::InMemoryLockFreeChain<ChainData>>(
                        std::get<1>(parsed)
                        , []() {
                            return new basic::simple_shared_chain::InMemoryLockFreeChain<ChainData>();
                        }
                    )
                    , key
                    , extraData
                );
            } else {
                writeExtraDataHelper<ExtraData>(
                    getChain<simple_shared_chain::InMemoryWithLockChain<ChainData>>(
                        std::get<1>(parsed)
                        , []() {
                            return new basic::simple_shared_chain::InMemoryWithLockChain<ChainData>();
                        }
                    )
                    , key
                    , extraData
                );
            }
        }
        template <class ChainData, class ExtraData>
        std::optional<ExtraData> readExtraData(typename App::EnvironmentType *, std::string const &descriptor, std::string const &key) {
            auto parsed = parseDescriptor(descriptor);
            if (std::get<0>(parsed)) {
                return readExtraDataHelper<ExtraData>(
                    getChain<simple_shared_chain::InMemoryLockFreeChain<ChainData>>(
                        std::get<1>(parsed)
                        , []() {
                            return new basic::simple_shared_chain::InMemoryLockFreeChain<ChainData>();
                        }
                    )
                    , key
                );
            } else {
                return readExtraDataHelper<ExtraData>(
                    getChain<simple_shared_chain::InMemoryWithLockChain<ChainData>>(
                        std::get<1>(parsed)
                        , []() {
                            return new basic::simple_shared_chain::InMemoryWithLockChain<ChainData>();
                        }
                    )
                    , key
                );
            }
        }
    };

} } } } }

#endif