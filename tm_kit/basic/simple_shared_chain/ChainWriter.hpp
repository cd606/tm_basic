#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_WRITER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class App, class Chain, class ChainItemFolder, class InputHandler>
    class ChainWriter {};

    template <class Env, class Chain, class ChainItemFolder, class InputHandler>
    class ChainWriter<typename infra::RealTimeApp<Env>, Chain, ChainItemFolder, InputHandler>
        : 
        public virtual infra::RealTimeApp<Env>::IExternalComponent
        , public virtual infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>
    {
    private:
        friend class InnerHandler;
        class InnerHandler : public infra::RealTimeAppComponents<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> {
        private:
            ChainWriter *parent_;
        public:
            InnerHandler(ChainWriter *parent) : infra::RealTimeAppComponents<Env>::template ThreadedHandler<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>>(), parent_(parent) {
            }
            virtual ~InnerHandler() {}
            virtual void idleWork() override final {
                static auto foldInPlaceChecker = boost::hana::is_valid(
                    [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
                );
                std::optional<typename Chain::ItemType> nextItem = parent_->chain_->fetchNext(parent_->currentItem_);
                if (nextItem) {
                    parent_->currentItem_ = std::move(*nextItem);
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (typename Chain::ItemType const *) nullptr
                    )) {
                        parent_->folder_.foldInPlace(parent_->currentState_, parent_->currentItem_);
                    } else {
                        parent_->currentState_ = parent_->folder_.fold(parent_->currentState_, parent_->currentItem_);
                    }
                }
            }
            virtual void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
                static auto checker = boost::hana::is_valid(
                    [](auto *h, auto *v) -> decltype((void) (h->discardUnattachedChainItem(*v))) {}
                );
                static auto foldInPlaceChecker = boost::hana::is_valid(
                    [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
                );
               
                auto id = data.timedData.value.id();
                while (true) {
                    while (true) {
                        std::optional<typename Chain::ItemType> nextItem = parent_->chain_->fetchNext(parent_->currentItem_);
                        if (!nextItem) {
                            break;
                        }
                        parent_->currentItem_ = std::move(*nextItem);
                        if constexpr (foldInPlaceChecker(
                            (ChainItemFolder *) nullptr
                            , (typename ChainItemFolder::ResultType *) nullptr
                            , (typename Chain::ItemType const *) nullptr
                        )) {
                            parent_->folder_.foldInPlace(parent_->currentState_, parent_->currentItem_);
                        } else {
                            parent_->currentState_ = parent_->folder_.fold(parent_->currentState_, parent_->currentItem_);
                        }
                    }
                    std::tuple<
                        typename InputHandler::ResponseType
                        , std::optional<typename Chain::ItemType>
                    > processResult = parent_->inputHandler_.handleInput(data.environment, std::move(data.timedData), parent_->currentState_);
                    if (std::get<1>(processResult)) {    
                        if (parent_->chain_->appendAfter(parent_->currentItem_, std::move(*std::get<1>(processResult)))) {
                            parent_->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                                id, std::move(std::get<0>(processResult))
                            }, true);
                            break;
                        } else {
                            //at this point processResult might be already empty (since it was passed
                            //as a right reference to appendAfter), discardUnattachedChainItem implementation needs
                            //to consider that
                            if constexpr (checker(
                                (InputHandler *) nullptr
                                , (typename Chain::ItemType *) nullptr
                            )) {
                                parent_->inputHandler_.discardUnattachedChainItem(*std::get<1>(processResult));
                            }
                        }
                    } else {
                        parent_->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    }
                }
            }
        };
        InnerHandler *innerHandler_;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        InputHandler inputHandler_;
        typename ChainItemFolder::ResultType currentState_;   
    public:
        ChainWriter(Chain *chain) : 
#ifndef _MSC_VER
            infra::RealTimeApp<Env>::IExternalComponent(), 
            infra::RealTimeApp<Env>::template AbstractOnOrderFacility<typename InputHandler::InputType, typename InputHandler::ResponseType>(), 
#endif
            innerHandler_(nullptr)
            , chain_(chain)
            , currentItem_()
            , folder_()
            , inputHandler_()
            , currentState_() 
        {}
        virtual ~ChainWriter() {
            if (innerHandler_) {
                delete innerHandler_;
            }
        }
        ChainWriter(ChainWriter const &) = delete;
        ChainWriter &operator=(ChainWriter const &) = delete;
        ChainWriter(ChainWriter &&) = default;
        ChainWriter &operator=(ChainWriter &&) = default;
        virtual void start(Env *env) override final {
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForValue(*v)))) {}
            );
            
            currentState_ = folder_.initialize(env);
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForValue(currentState_));
            } else {
                currentItem_ = chain_->head(env);
            }
            inputHandler_.initialize(env);
            innerHandler_ = new InnerHandler(this);
        }
        virtual void handle(typename infra::RealTimeApp<Env>::template InnerData<typename infra::RealTimeApp<Env>::template Key<typename InputHandler::InputType>> &&data) override final {
            if (innerHandler_) {
                innerHandler_->handle(std::move(data));
            }
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
            static auto checker = boost::hana::is_valid(
                [](auto *h, auto *v) -> decltype((void) (h->discardUnattachedChainItem(*v))) {}
            );
            static auto foldInPlaceChecker = boost::hana::is_valid(
                [](auto *f, auto *v, auto const *i) -> decltype((void) (f->foldInPlace(*v, *i))) {}
            );
            auto id = data.timedData.value.id();
            while (true) {
                while (true) {
                    std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                    if (!nextItem) {
                        break;
                    }
                    currentItem_ = std::move(*nextItem);
                    if constexpr (foldInPlaceChecker(
                        (ChainItemFolder *) nullptr
                        , (typename ChainItemFolder::ResultType *) nullptr
                        , (typename Chain::ItemType const *) nullptr
                    )) {
                        folder_.foldInPlace(currentState_, currentItem_);
                    } else {
                        currentState_ = folder_.fold(currentState_, currentItem_);
                    }
                }
                std::tuple<
                    typename InputHandler::ResponseType
                    , std::optional<typename Chain::ItemType>
                > processResult = inputHandler_.handleInput(data.environment, std::move(data.timedData), currentState_);
                if (std::get<1>(processResult)) {    
                    if (chain_->appendAfter(currentItem_, std::move(*std::get<1>(processResult)))) {
                        this->publish(data.environment, typename infra::RealTimeApp<Env>::template Key<typename InputHandler::ResponseType> {
                            id, std::move(std::get<0>(processResult))
                        }, true);
                        break;
                    } else {
                        //at this point processResult might be already empty (since it was passed
                        //as a right reference to appendAfter), discardUnattachedChainItem implementation needs
                        //to consider that
                        if constexpr (checker(
                            (InputHandler *) nullptr
                            , (typename Chain::ItemType *) nullptr
                        )) {
                            inputHandler_.discardUnattachedChainItem(*std::get<1>(processResult));
                        }
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
            static auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForValue(*v)))) {}
            );

            currentState_ = folder_.initialize(env);
            
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForValue(currentState_));
            } else {
                currentItem_ = chain_->head(env);
            }
            inputHandler_.initialize(env);
        }
    };
} } } } }

#endif