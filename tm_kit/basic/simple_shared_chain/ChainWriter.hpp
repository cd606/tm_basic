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
        Chain const *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;
    protected:
        virtual void idleWork() override final {
            if (chain_->fetchNext(&currentItem_)) {
                currentState_ = folder_.fold(currentState_, currentItem_);
            }
        }
        virtual void actuallyHandle(typename infra::RealTimeApp<Env>::template Data<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            while (true) {
                while (chain_->fetchNext(&currentItem_)) {
                    currentState_ = folder_.fold(currentState_, currentItem_);
                }
                typename Chain::ItemType itemToBeWritten;
                typename InputHandler::ResponseType response;
                if (inputHandler_.handleInput(data.timedData.value.key(), currentState_, &itemToBeWritten, &response)) {
                    if (chain_->append(&currentItem_, std::move(itemToBeWritten))) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            data.timedData.value.id(), std::move(response)
                        }, true);
                        break;
                    }
                } else {
                    this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        data.timedData.value.id(), std::move(response)
                    }, true);
                    break;
                }
            }
        }
    public:
        ChainWriter(Chain const *chain) : 
            infra::RealTimeApp<Env>::IExternalComponent()
            , infra::RealTimeApp<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>()
            , infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>()
            , chain_(chain)
            , currentItem_(chain->initialItem())
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
        Chain const *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;
    protected:
        virtual void handle(typename infra::SinglePassIterationApp<Env>::template Data<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            while (true) {
                while (chain_->fetchNext(&currentItem_)) {
                    currentState_ = folder_.fold(currentState_, currentItem_);
                }
                typename Chain::ItemType itemToBeWritten;
                typename InputHandler::ResponseType response;
                if (inputHandler_.handleInput(data.timedData.value.key(), currentState_, &itemToBeWritten, &response)) {
                    if (chain_->append(&currentItem_, itemToBeWritten)) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            data.timedData.value.id(), std::move(response)
                        }, true);
                        break;
                    }
                } else {
                    this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        data.timedData.value.id(), std::move(response)
                    }, true);
                    break;
                }
            }
        }
    public:
        ChainWriter(Chain const *chain) :
            infra::SinglePassIterationApp<Env>::IExternalComponent()
            , infra::SinglePassIterationApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>()
            , chain_(chain)
            , currentItem_(chain->initialItem())
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
            currentState_ = folder_.initialize(env);
            inputHandler_.initialize(env);
        }
    };
} } } } }

#endif