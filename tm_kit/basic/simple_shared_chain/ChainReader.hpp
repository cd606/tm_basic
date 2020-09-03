#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <chrono>
#include <thread>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class Env, class Chain, class ChainItemFolder>
    class ChainReader : public infra::RealTimeApp<Env>::template AbstractImporter<typename ChainItemFolder::ResultType> {
    private:
        Chain const *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        std::chrono::system_clock::duration checkPeriod_;
        typename ChainItemFolder::ResultType currentValue_;
        std::thread th_;
        std::atomic<bool> running_;

        virtual typename ChainItemFolder::ResultType fetchInitialValue(Env *) = 0;
        void run(Env *env) {
            while (running_) {
                bool hasNew = false;
                while (chain_->fetchNext(&currentItem_)) {
                    currentValue_ = folder_.fold(currentValue_, currentItem_);
                    hasNew = true;
                }
                if (hasNew) {
                    this->publish(env, typename ChainItemFolder::ResultType {currentValue_});
                }
                std::this_thread::sleep_for(checkPeriod_);
            }
        }
    public:
        ChainReader(Chain const *chain, std::chrono::system_clock::duration const &checkPeriod) : chain_(chain), currentItem_(chain->initialItem()), folder_(), checkPeriod_(checkPeriod), currentValue_(), th_(), running_(false) {}
        virtual ~ChainReader() {
            if (running_) {
                running_ = false;
                th_.join();
            }
        }
        virtual void start(Env *env) override final {
            currentValue_ = fetchInitialValue(env);
            running_ = true;
            th_ = std::thread(&ChainReader::run, this, env);
            th_.detach();
        }
    };
} } } } }

#endif