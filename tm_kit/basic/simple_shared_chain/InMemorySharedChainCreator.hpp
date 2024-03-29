#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_IN_MEMORY_SHARED_CHAIN_CREATOR_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_IN_MEMORY_SHARED_CHAIN_CREATOR_HPP_

#include <tm_kit/basic/simple_shared_chain/ChainReader.hpp>
#include <tm_kit/basic/simple_shared_chain/ChainWriter.hpp>
#include <tm_kit/basic/simple_shared_chain/OneShotChainWriter.hpp>
#include <tm_kit/basic/simple_shared_chain/InMemoryWithLockChain.hpp>
#include <tm_kit/basic/simple_shared_chain/InMemoryLockFreeChain.hpp>
#include <tm_kit/basic/simple_shared_chain/EmptyChain.hpp>

#include <unordered_map>
#include <mutex>

#include <boost/algorithm/string.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {

    template <class App>
    class InMemorySharedChainCreator {
    private:
        std::mutex mutex_;
        std::unordered_map<std::string, std::tuple<void *, std::function<void(void *)>>> chains_;

    public:
        enum class ChainType {
            LockFree, HasLock, Empty
        };
    private:
        inline auto parseDescriptor(std::string const &s)
            -> std::tuple<ChainType, std::string>
        {
            if (boost::starts_with(s, "empty://")) {
                return {ChainType::Empty, s};
            } else if (boost::starts_with(s, "lock_free://") || boost::starts_with(s, "in_memory_lock_free://")) {
                return {ChainType::LockFree, s};
            } else {
                return {ChainType::HasLock, s};
            }
        }

        template <class ChainItemFolder, class TriggerT, class ResultTransformer>
        static auto ImporterOrActionTypeHelper() {
            if constexpr (std::is_same_v<TriggerT, void>) {
                return std::shared_ptr<typename App::template Importer<std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>>>();
            } else {
                return std::shared_ptr<typename App::template Action<TriggerT, std::conditional_t<std::is_same_v<ResultTransformer, void>, typename ChainItemFolder::ResultType, typename ResultTypeExtractor<ResultTransformer>::TheType>>>();
            }
        }
        template <class ChainItemFolder, class TriggerT, class ResultTransformer>
        using ImporterOrAction = decltype(
            ImporterOrActionTypeHelper<ChainItemFolder,TriggerT,ResultTransformer>()
        );

        template <class ChainItemFolder, class TriggerT, class ResultTransformer, class Chain>
        inline auto chainReaderHelper(
            typename App::EnvironmentType *env
            , Chain *chain 
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy
            , ChainItemFolder &&folder
            , std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer
        ) 
            -> ImporterOrAction<ChainItemFolder, TriggerT, ResultTransformer>
        {
            if constexpr (std::is_same_v<TriggerT, void>) {
                return simple_shared_chain::ChainReader<
                    App
                    , Chain
                    , ChainItemFolder
                    , void
                    , ResultTransformer
                >::importer(
                    chain
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(resultTransformer)
                );
            } else {
                return simple_shared_chain::ChainReader<
                    App
                    , Chain
                    , ChainItemFolder
                    , TriggerT
                    , ResultTransformer
                >::action(
                    env
                    , chain
                    , std::move(folder)
                    , std::move(resultTransformer)
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

        template <class ChainData, class ChainItemFolder, class TriggerT=void, class ResultTransformer=void>
        auto reader(
            typename App::EnvironmentType *env
            , ChainType chainType
            , std::string const &name
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
            , std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()
        )
            -> ImporterOrAction<ChainItemFolder, TriggerT, ResultTransformer>
        {
            if (chainType == ChainType::Empty) {
                return chainReaderHelper<ChainItemFolder,TriggerT,ResultTransformer>(
                    env
                    , getChain<simple_shared_chain::EmptyChain<ChainData>>(
                        name
                        , []() {
                            return new basic::simple_shared_chain::EmptyChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(resultTransformer)
                );
            } else if (chainType == ChainType::LockFree) {
                return chainReaderHelper<ChainItemFolder,TriggerT,ResultTransformer>(
                    env
                    , getChain<simple_shared_chain::InMemoryLockFreeChain<ChainData>>(
                        name
                        , []() {
                            return new basic::simple_shared_chain::InMemoryLockFreeChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(resultTransformer)
                );
            } else {
                return chainReaderHelper<ChainItemFolder,TriggerT,ResultTransformer>(
                    env
                    , getChain<simple_shared_chain::InMemoryWithLockChain<ChainData>>(
                        name
                        , []() {
                            return new simple_shared_chain::InMemoryWithLockChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(resultTransformer)
                );
            }
        }

        template <class ChainData, class ChainItemFolder, class TriggerT=void, class ResultTransformer=void>
        auto reader(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
            , std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()
        )
            -> ImporterOrAction<ChainItemFolder, TriggerT, ResultTransformer>
        {
            auto parsed = parseDescriptor(descriptor);
            return reader<ChainData,ChainItemFolder,TriggerT,ResultTransformer>(
                env, std::get<0>(parsed), std::get<1>(parsed), pollingPolicy, std::move(folder), std::move(resultTransformer)
            );
        }

        template <class ChainData, class ChainItemFolder, class InputHandler, class IdleLogic=void>
        auto writer(
            typename App::EnvironmentType *env
            , ChainType chainType
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
            if (chainType == ChainType::Empty) {
                return chainWriterHelper<ChainItemFolder,InputHandler,IdleLogic>(
                    getChain<simple_shared_chain::EmptyChain<ChainData>>(
                        name
                        , []() {
                            return new basic::simple_shared_chain::EmptyChain<ChainData>();
                        }
                    )
                    , pollingPolicy
                    , std::move(folder)
                    , std::move(inputHandler)
                    , std::move(idleLogic)
                );
            } else if (chainType == ChainType::LockFree) {
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

    private:
        template <class ChainItemFolder, class TriggerT, class ResultTransformer>
        static auto ImporterOrActionFactoryTypeHelper() {
            if constexpr (std::is_same_v<TriggerT, void>) {
                return basic::simple_shared_chain::ChainReaderImporterFactory<App, ChainItemFolder, ResultTransformer>();
            } else {
                return basic::simple_shared_chain::ChainReaderActionFactory<App, ChainItemFolder, TriggerT, ResultTransformer>();
            }
        }
        template <class ChainItemFolder, class TriggerT, class ResultTransformer>
        using ImporterOrActionFactory = decltype(
            ImporterOrActionFactoryTypeHelper<ChainItemFolder,TriggerT,ResultTransformer>()
        );

    public:
        template <class ChainData, class ChainItemFolder, class TriggerT=void, class ResultTransformer=void>
        auto readerFactory(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , simple_shared_chain::ChainPollingPolicy const &pollingPolicy = simple_shared_chain::ChainPollingPolicy()
            , ChainItemFolder &&folder = ChainItemFolder {}
            , std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer> &&resultTransformer = std::conditional_t<std::is_same_v<ResultTransformer, void>, bool, ResultTransformer>()
        )
            -> ImporterOrActionFactory<ChainItemFolder,TriggerT,ResultTransformer>
        {
            auto f = std::make_shared<std::decay_t<decltype(folder)>>(std::move(folder));
            auto t = std::make_shared<std::decay_t<decltype(resultTransformer)>>(std::move(resultTransformer));
            return [this,env,descriptor,pollingPolicy,f,t]() {
                return reader<ChainData,ChainItemFolder,TriggerT,ResultTransformer>(
                    env, descriptor, pollingPolicy, std::move(*f), std::move(*t)
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
            auto f = std::make_shared<std::decay_t<decltype(folder)>>(std::move(folder));
            auto h = std::make_shared<std::decay_t<decltype(inputHandler)>>(std::move(inputHandler));
            auto l = std::make_shared<std::decay_t<decltype(idleLogic)>>(std::move(idleLogic));
            return [this,env,descriptor,pollingPolicy,f,h,l]() {
                return writer<ChainData,ChainItemFolder,InputHandler,IdleLogic>(
                    env, descriptor, pollingPolicy, std::move(*f), std::move(*h), std::move(*l)
                );
            };
        }

        template <class ChainData, class ExtraData>
        void writeExtraData(typename App::EnvironmentType *env, std::string const &descriptor, std::string const &key, ExtraData const &extraData) {
            auto parsed = parseDescriptor(descriptor);
            if (std::get<0>(parsed) == ChainType::Empty) {
            } else if (std::get<0>(parsed) == ChainType::LockFree) {
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
            if (std::get<0>(parsed) == ChainType::Empty) {
                return std::nullopt;
            } else if (std::get<0>(parsed) == ChainType::LockFree) {
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
        template <class ChainData, class ChainItemFolder, class F, bool ForceSeparateDataStorageIfPossible=false>
        bool oneShotWrite(
            typename App::EnvironmentType *env
            , std::string const &descriptor
            , F const &f
            , ChainItemFolder &&folder = ChainItemFolder {}
        ) {
            auto parsed = parseDescriptor(descriptor);
            if (std::get<0>(parsed) == ChainType::Empty) {
                return true;
            } else if (std::get<0>(parsed) == ChainType::LockFree) {
                return basic::simple_shared_chain::OneShotChainWriter<typename App::EnvironmentType,simple_shared_chain::InMemoryLockFreeChain<ChainData>>::write(
                    env
                    , getChain<simple_shared_chain::InMemoryLockFreeChain<ChainData>>(
                        std::get<1>(parsed)
                        , []() {
                            return new basic::simple_shared_chain::InMemoryLockFreeChain<ChainData>();
                        }
                    )
                    , f 
                    , std::move(folder)
                );
            } else {
                return basic::simple_shared_chain::OneShotChainWriter<typename App::EnvironmentType,simple_shared_chain::InMemoryWithLockChain<ChainData>>::write(
                    env
                    , getChain<simple_shared_chain::InMemoryWithLockChain<ChainData>>(
                        std::get<1>(parsed)
                        , []() {
                            return new basic::simple_shared_chain::InMemoryWithLockChain<ChainData>();
                        }
                    )
                    , f 
                    , std::move(folder)
                );
            }
        }
    };

} } } } }

#endif