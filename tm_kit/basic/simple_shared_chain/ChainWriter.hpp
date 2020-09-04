#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class Env, class Chain, class ChainItemFolder, class InputHandler>
    class ChainWriter : public infra::RealTimeApp<Env>::IExternalComponent, public infra::RealTimeApp<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>, public infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType> {
    private:
        Chain const *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;

        virtual typename ChainItemFolder::ResultType fetchInitialState(Env *) = 0;
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
                    if (chain_->append(&currentItem_, itemToBeWritten)) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            data.timedData.value.id(), std::move(response)
                        });
                        break;
                    }
                } else {
                    this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                        data.timedData.value.id(), std::move(response)
                    });
                    break;
                }
            }
        }
    public:
        ChainWriter(Chain const *chain) : infra::RealTimeApp<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>(), chain_(chain), currentItem_(chain->initialItem()), folder_(), inputHandler_(), currentState_() {}
        virtual ~ChainWriter() {
        }
        virtual void start(Env *env) override final {
            currentState_ = fetchInitialValue(env);
        }
    };
} } } } }

#endif