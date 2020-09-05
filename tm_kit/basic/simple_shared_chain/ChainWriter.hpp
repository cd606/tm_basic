#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class App, class Chain, class ChainItemFolder, class InputHandler>
    class ChainWriter {};

    template <class Env, class Chain, class ChainItemFolder, class InputHandler>
    class ChainWriter<typename infra::RealTimeApp<Env>, Chain, ChainItemFolder, InputHandler>
        : 
        public infra::RealTimeApp<Env>::IExternalComponent
        , public infra::RealTimeApp<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>
        , public infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> {
    private:
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;
    protected:
        virtual void idleWork() override final {
            std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
            if (nextItem) {
                currentItem_ = std::move(*nextItem);
                currentState_ = folder_.fold(currentState_, currentItem_);
            }
        }
        virtual void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            auto id = data.timedData.value.id();
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    currentState_ = folder_.fold(currentState_, currentItem_);
                }
                std::tuple<
                    typename InputHandler::ResponseType
                    , std::optional<typename Chain::ItemType>
                > processResult = inputHandler_.handleInput(data.environment, std::move(data.timedData.value), currentState_);
                if (std::get<1>(processResult)) {    
                    if (chain_->appendAfter(currentItem_, std::move(*std::get<1>(processResult)))) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else {
                    this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        id, std::move(std::get<0>(processResult))
                    }, true);
                    break;
                }
            }
        }
    public:
        ChainWriter(Chain *chain) : 
            infra::RealTimeApp<Env>::IExternalComponent()
            , infra::RealTimeApp<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>()
            , infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>()
            , chain_(chain)
            , currentItem_()
            , folder_()
            , inputHandler_()
            , currentState_() 
        {}
        virtual ~ChainWriter() {
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            currentItem_ = chain_->head(env);
            currentState_ = folder_.initialize(env);
            inputHandler_.initialize(env);
        }
    };

    template <class Env, class Chain, class ChainItemFolder, class InputHandler>
    class ChainWriter<typename infra::SinglePassIterationApp<Env>, Chain, ChainItemFolder, InputHandler>
        : 
        public infra::SinglePassIterationApp<Env>::IExternalComponent
        , public infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> {
    private:
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;
    protected:
        virtual void handle(typename infra::SinglePassIterationApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            auto id = data.timedData.value.id();
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    currentState_ = folder_.fold(currentState_, currentItem_);
                }
                std::tuple<
                    typename InputHandler::ResponseType
                    , std::optional<typename Chain::ItemType>
                > processResult = inputHandler_.handleInput(data.environment, std::move(data.timedData.value), currentState_);
                if (std::get<1>(processResult)) {    
                    if (chain_->appendAfter(currentItem_, std::move(*std::get<1>(processResult)))) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                } else {
                    this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        id, std::move(std::get<0>(processResult))
                    }, true);
                    break;
                }
            }
        }
    public:
        ChainWriter(Chain *chain) :
            infra::SinglePassIterationApp<Env>::IExternalComponent()
            , infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>()
            , chain_(chain)
            , currentItem_()
            , folder_()
            , inputHandler_()
            , currentState_() 
        {}
        virtual ~ChainWriter() {
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            currentItem_ = chain_->head(env);
            currentState_ = folder_.initialize(env);
            inputHandler_.initialize(env);
        }
    };
} } } } }

#endif