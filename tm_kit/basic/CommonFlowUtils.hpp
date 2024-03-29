#ifndef TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_
#define TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_

#include <memory>
#include <optional>
#include <deque>
#include <type_traits>

#include <tm_kit/infra/KleisliUtils.hpp>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/infra/GenericLift.hpp>
#include <tm_kit/infra/AppClassifier.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/NotConstructibleStruct.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/CounterComponent.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/PrintHelper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    namespace common_flow_util_helper {
        template <class M, class K>
        class KeyedSynchronizer2DefaultHash {
        public:
            using TheType = std::hash<K>;
        };
        template <class M>
        class KeyedSynchronizer2DefaultHash<M, typename M::EnvironmentType::IDType> {
        public:
            using TheType = typename M::EnvironmentType::IDHash;
        };
    }

    template <class M>
    class CommonFlowUtilComponents {
    public:
        using TheEnvironment = typename M::EnvironmentType;

        template <class T>
        static auto idFunc() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> T
                {
                    return std::move(x);
                }
            );
        }

        template <class T>
        static auto bufferingNode() {
            return M::template kleisli<T>(idFunc<T>(), infra::LiftParameters<typename M::TimePoint>().SuggestThreaded(true));
        }

        template <class T>
        static auto filterOnOptional() {
            return infra::KleisliUtils<M>::template liftMaybe<std::optional<T>>(
                [](std::optional<T> &&x) -> std::optional<T>
                {
                    return std::move(x);
                }
            );
        }

        template <class T>
        static auto dispatchOneByOne() {
            return M::template liftMulti<std::vector<T>>(
                [](std::vector<T> &&x) -> std::vector<T>
                {
                    return std::move(x);
                }
            );
        }
        template <class T>
        static auto dispatchInSegments(std::size_t segmentSize) {
            return M::template liftMulti<std::vector<T>>(
                [segmentSize](std::vector<T> &&x) -> std::vector<std::vector<T>>
                {
                    auto ll = x.size();
                    if (ll <= segmentSize) {
                        return {std::move(x)};
                    } else {
                        std::vector<std::vector<T>> segments;
                        for (std::size_t ii=0; ii<ll; ii+=segmentSize) {
                            segments.push_back({});
                            auto &b = segments.back();
                            for (std::size_t jj=ii; jj<ii+segmentSize && jj<ll; ++jj) {
                                b.push_back(std::move(x[jj]));
                            }
                        }
                        return segments;
                    }
                }
            );
        }
        
        //this is used when we want to convert an 
        //input to some empty type, so it effectively becomes
        //a triggering input
        template <class T1, class T2, typename=std::enable_if_t<
            std::is_empty_v<T2>
        >>
        static auto discardContent() {
            return infra::KleisliUtils<M>::template liftPure<T1>(
                [](T1 &&) -> T2 {
                    return T2 {};
                }
            );
        }

        //These are also used for type safety, converting between
        //wrapped and unwrapped types
        template <class T, class Wrapped=SingleLayerWrapper<T>, typename=std::enable_if_t<
            std::is_same_v<typename Wrapped::value_type, T>
        >>
        static auto wrap() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> Wrapped {
                    return Wrapped {std::move(x)};
                }
            );
        }
        template <class T, class Wrapped=SingleLayerWrapper<T>, typename=std::enable_if_t<
            std::is_same_v<typename Wrapped::value_type, T>
        >>
        static auto unwrap() {
            return infra::KleisliUtils<M>::template liftPure<Wrapped>(
                [](Wrapped &&x) -> T {
                    return std::move(x.value);
                }
            );
        }
        template <class Wrapped1, class Wrapped2, typename=std::enable_if_t<
            std::is_same_v<typename Wrapped1::value_type, typename Wrapped2::value_type>
        >>
        static auto changeWrap() {
            return infra::KleisliUtils<M>::template liftPure<Wrapped1>(
                [](Wrapped1 &&x) -> Wrapped2 {
                    return Wrapped2 {std::move(x.value)};
                }
            );
        }

        template <class T>
        static auto printExporter() {
            return infra::GenericLift<M>::lift(
                [](typename M::template InnerData<T> &&data) {
                    std::ostringstream oss;
                    oss << "{timestamp=";
                    PrintHelper<decltype(data.timedData.timePoint)>::print(oss, data.timedData.timePoint);
                    oss << ", value=";
                    PrintHelper<T>::print(oss, data.timedData.value);
                    oss << "}";
                    if (data.timedData.finalFlag) {
                        oss << " [F]";
                    }
                    data.environment->log(infra::LogLevel::Info, oss.str());
                }
            );
        }

    private:
        //This is the "reverse" of dispatchOneByOne
        template <class T, class Buncher=void>
        static auto bunch_(
            std::conditional_t<std::is_same_v<Buncher, void>, bool, Buncher> &&buncher
            = std::conditional_t<std::is_same_v<Buncher, void>, bool, Buncher> {}
        ) -> typename std::shared_ptr<typename M::template Action<T
            , std::decay_t<decltype((*((
                std::conditional_t<
                    std::is_same_v<Buncher, void>
                    , std::function<std::vector<T>(std::vector<T> &&)>
                    , Buncher
                >
            *) nullptr))(std::move(* ((std::vector<T> *) nullptr))))>
        >> {
            using B = std::conditional_t<std::is_same_v<Buncher, void>, bool, Buncher>;
            using C = std::decay_t<decltype((*((
                    std::conditional_t<
                        std::is_same_v<Buncher, void>
                        , std::function<std::vector<T>(std::vector<T> &&)>
                        , Buncher
                    > 
            *) nullptr))(std::move(* ((std::vector<T> *) nullptr))))>;
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                class BunchAction : public infra::IRealTimeAppPossiblyThreadedNode, public infra::RealTimeAppComponents<TheEnvironment>::template AbstractAction<T,C> {
                private:
                    B buncher_;
                    typename infra::RealTimeAppComponents<TheEnvironment>::template TimeChecker<false, T> timeChecker_;
                    std::mutex mutex_;
                    std::condition_variable cond_;
                    std::thread th_;
                    std::atomic<bool> running_;
                    std::list<typename M::template InnerData<T>> incoming_, processing_;  

                    void stopThread() {
                        running_ = false;
                    }
                    void runThread() {
                        while (running_) {
                            {
                                std::unique_lock<std::mutex> lock(mutex_);
                                cond_.wait_for(lock, std::chrono::milliseconds(1));
                                if (!running_) {
                                    lock.unlock();
                                    return;
                                }
                                if (incoming_.empty()) {
                                    lock.unlock();
                                    continue;
                                }
                                processing_.splice(processing_.end(), incoming_);
                                lock.unlock();
                            }
                            std::vector<T> output;
                            TheEnvironment *env = nullptr;
                            bool isFinal = false;
                            while (!processing_.empty()) {
                                auto &data = processing_.front();
                                if (timeChecker_(data)) {
                                    env = data.environment;
                                    isFinal = (isFinal || data.timedData.finalFlag);
                                    output.push_back(std::move(data.timedData.value));
                                }
                                processing_.pop_front();
                            }
                            if constexpr (std::is_same_v<Buncher, void>) {
                                if (!output.empty()) {
                                    this->publish(typename M::template InnerData<std::vector<T>> {
                                        env 
                                        , {
                                            env->resolveTime()
                                            , std::move(output)
                                            , isFinal
                                        }
                                    });
                                    output.clear();
                                }
                            } else {
                                if (!output.empty()) {
                                    this->publish(typename M::template InnerData<C> {
                                        env 
                                        , {
                                            env->resolveTime()
                                            , buncher_(std::move(output))
                                            , isFinal
                                        }
                                    });
                                    output.clear();
                                }
                            }
                        }  
                    }
                    void putData(typename M::template InnerData<T> &&data) {
                        if (running_) {
                            {
                                std::lock_guard<std::mutex> _(mutex_);
                                incoming_.push_back(std::move(data));
                            }
                            cond_.notify_one();
                        }                    
                    }
                public:
                    BunchAction(B &&buncher) : buncher_(std::move(buncher)), timeChecker_(), mutex_(), cond_(), th_(), running_(false), incoming_(), processing_() {
                        running_ = true;
                        th_ = std::thread(&BunchAction::runThread, this);
                        th_.detach();
                    }
                    virtual ~BunchAction() {
                        stopThread();
                    }
                    virtual bool isThreaded() const override final {
                        return true;
                    }
                    virtual bool isOneTimeOnly() const override final {
                        return false;
                    }
                    virtual void handle(typename M::template InnerData<T> &&data) override final {
                        putData(std::move(data));   
                    }
                    virtual std::optional<std::thread::native_handle_type> threadHandle() override final {
                        return th_.native_handle();
                    }
                    virtual void setIdleWorker(std::function<void(void *)> worker) override final {
                    }
                    virtual void setStartWaiter(std::function<void()> waiter) override final {
                    }
                };
                return M::template fromAbstractAction<T, C>(new BunchAction(std::move(buncher)));
            } else if constexpr (std::is_same_v<M, infra::SinglePassIterationApp<TheEnvironment>>) {
                struct State {
                    std::optional<typename M::TimePoint> currentBunchTime;
                    std::vector<T> currentBunch;
                    bool endReached;
                    bool repeatCallForThisInput;
                    State() : currentBunchTime(std::nullopt), currentBunch(), endReached(false), repeatCallForThisInput(false) {}
                    State(State const &) = default;
                    State(State &&) = default;
                    State &operator=(State const &) = default;
                    State &operator=(State &&) = default;
                    ~State() = default;
                    std::optional<typename M::TimePoint> nextTimePoint() const {
                        return currentBunchTime;
                    }
                };
                auto cont = [buncher=std::move(buncher)](typename M::template InnerData<T> &&input, State &state, std::function<void(typename M::template InnerData<C> &&)> handler) {                
                    if (state.repeatCallForThisInput) {
                        if (state.currentBunchTime && state.endReached) {
                            if constexpr (std::is_same_v<Buncher, void>) {
                                handler(
                                    typename M::template InnerData<std::vector<T>> {
                                        input.environment 
                                        , {
                                            *state.currentBunchTime
                                            , std::move(state.currentBunch)
                                            , true
                                        }
                                    }
                                );
                            } else {
                                handler(
                                    typename M::template InnerData<C> {
                                        input.environment 
                                        , {
                                            *state.currentBunchTime
                                            , buncher(std::move(state.currentBunch))
                                            , true
                                        }
                                    }
                                );
                            }
                            state.currentBunchTime = std::nullopt;
                            state.currentBunch.clear();
                        } else {
                            //do nothing
                        }
                        state.repeatCallForThisInput = false;
                    } else {                    
                        if (!state.currentBunchTime) {
                            if (input.timedData.finalFlag) {
                                if constexpr (std::is_same_v<Buncher, void>) {
                                    handler(
                                        typename M::template InnerData<std::vector<T>> {
                                            input.environment 
                                            , {
                                                input.timedData.timePoint
                                                , std::vector<T> { std::move(input.timedData.value) }
                                                , true
                                            }
                                        }
                                    );
                                } else {
                                    handler(
                                        typename M::template InnerData<C> {
                                            input.environment 
                                            , {
                                                input.timedData.timePoint
                                                , buncher(std::vector<T> { std::move(input.timedData.value) })
                                                , true
                                            }
                                        }
                                    );
                                }
                                state.repeatCallForThisInput = true;
                            } else {
                                state.currentBunchTime = input.timedData.timePoint;
                                state.currentBunch.push_back(std::move(input.timedData.value));
                            }
                        } else {
                            if (*(state.currentBunchTime) >= input.timedData.timePoint) {
                                state.currentBunch.push_back(std::move(input.timedData.value));
                                if (input.timedData.finalFlag) {
                                    if constexpr (std::is_same_v<Buncher, void>) {
                                        handler(
                                            typename M::template InnerData<std::vector<T>> {
                                                input.environment 
                                                , {
                                                    *state.currentBunchTime
                                                    , std::move(state.currentBunch)
                                                    , true
                                                }
                                            }
                                        );
                                    } else {
                                        handler(
                                            typename M::template InnerData<C> {
                                                input.environment 
                                                , {
                                                    *state.currentBunchTime
                                                    , buncher(std::move(state.currentBunch))
                                                    , true
                                                }
                                            }
                                        );
                                    }
                                    state.repeatCallForThisInput = true;
                                    state.currentBunchTime = std::nullopt;
                                    state.currentBunch.clear();
                                } else {
                                    //do nothing
                                }
                            } else {
                                if constexpr (std::is_same_v<Buncher, void>) {
                                    handler(
                                        typename M::template InnerData<std::vector<T>> {
                                            input.environment 
                                            , {
                                                *state.currentBunchTime
                                                , std::move(state.currentBunch)
                                                , false
                                            }
                                        }
                                    );
                                } else {
                                    handler(
                                        typename M::template InnerData<C> {
                                            input.environment 
                                            , {
                                                *state.currentBunchTime
                                                , buncher(std::move(state.currentBunch))
                                                , false
                                            }
                                        }
                                    );
                                }
                                state.repeatCallForThisInput = true;
                                state.currentBunchTime = input.timedData.timePoint;
                                state.currentBunch.clear();
                                state.currentBunch.push_back(std::move(input.timedData.value));
                                state.endReached = input.timedData.finalFlag;
                            }
                        }
                    }
                };
                return M::template continuationAction<T,C,State>(cont);
            } else if constexpr (std::is_same_v<M, infra::TopDownSinglePassIterationApp<TheEnvironment>>) {
                class BunchAction : public M::template AbstractAction<T,C> {
                private:
                    B buncher_;
                    typename M::TimePoint currentBunchStart_;
                    std::vector<T> currentBunch_;
                public:
                    BunchAction(B &&buncher) : buncher_(std::move(buncher)), currentBunchStart_(), currentBunch_() {
                    }
                    virtual ~BunchAction() = default;
                    virtual bool isOneTimeOnly() const override final {
                        return false;
                    }
                    virtual void handle(typename M::template InnerData<T> &&data) override final {
                        if (currentBunch_.empty()) {
                            if (data.timedData.finalFlag) {
                                if constexpr (std::is_same_v<Buncher, void>) {
                                    this->publish(typename M::template InnerData<C> {
                                        data.environment 
                                        , {
                                            data.timedData.timePoint 
                                            , {std::move(data.timedData.value)}
                                            , true
                                        }
                                    });
                                } else {
                                    this->publish(typename M::template InnerData<C> {
                                        data.environment 
                                        , {
                                            data.timedData.timePoint 
                                            , buncher_(std::vector<T> {std::move(data.timedData.value)})
                                            , true
                                        }
                                    });
                                }
                            } else {
                                currentBunchStart_ = data.timedData.timePoint;
                                currentBunch_.push_back(std::move(data.timedData.value));
                            }
                        } else {
                            if (data.timedData.timePoint != currentBunchStart_) {
                                if constexpr (std::is_same_v<Buncher, void>) {
                                    this->publish(typename M::template InnerData<C> {
                                        data.environment 
                                        , {
                                            currentBunchStart_
                                            , std::move(currentBunch_)
                                            , false
                                        }
                                    });
                                } else {
                                    this->publish(typename M::template InnerData<C> {
                                        data.environment 
                                        , {
                                            currentBunchStart_ 
                                            , buncher_(std::move(currentBunch_))
                                            , false
                                        }
                                    });
                                }
                                currentBunch_.clear();
                                if (data.timedData.finalFlag) {
                                    if constexpr (std::is_same_v<Buncher, void>) {
                                        this->publish(typename M::template InnerData<C> {
                                            data.environment 
                                            , {
                                                data.timedData.timePoint 
                                                , {std::move(data.timedData.value)}
                                                , true
                                            }
                                        });
                                    } else {
                                        this->publish(typename M::template InnerData<C> {
                                            data.environment 
                                            , {
                                                data.timedData.timePoint 
                                                , buncher_(std::vector<T> {std::move(data.timedData.value)})
                                                , true
                                            }
                                        });
                                    }
                                } else {
                                    currentBunchStart_ = data.timedData.timePoint;
                                    currentBunch_.push_back(std::move(data.timedData.value));
                                }
                            } else {
                                currentBunch_.push_back(std::move(data.timedData.value));
                                if (data.timedData.finalFlag) {
                                    if constexpr (std::is_same_v<Buncher, void>) {
                                        this->publish(typename M::template InnerData<C> {
                                            data.environment 
                                            , {
                                                currentBunchStart_
                                                , std::move(currentBunch_)
                                                , true
                                            }
                                        });
                                    } else {
                                        this->publish(typename M::template InnerData<C> {
                                            data.environment 
                                            , {
                                                currentBunchStart_ 
                                                , buncher_(std::move(currentBunch_))
                                                , true
                                            }
                                        });
                                    }
                                    currentBunch_.clear();
                                }
                            }
                        }
                    }
                };
                return M::template fromAbstractAction<T, C>(new BunchAction(std::move(buncher)));
            }
        }
    public:
        template <class T>
        static auto bunch() {
            return bunch_<T, void>();
        }
        template <class T, class Buncher>
        static auto bunchWithProcessing(Buncher &&buncher = Buncher()) {
            return bunch_<T,Buncher>(std::move(buncher));
        }

        template <class T>
        static auto keyify() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> infra::Key<T,TheEnvironment>
                {
                    return infra::withtime_utils::keyify<T,TheEnvironment>(std::move(x));
                }
            );
        }

        template <class T, class CounterMarkType=NotConstructibleStruct>
        static auto keyifyThroughCounterFunc(TheEnvironment *env, T &&x) 
            -> typename M::template Key<T>
        {
            auto counter = env->getCounterValue((CounterMarkType *) nullptr);
            if constexpr (sizeof(counter) == TheEnvironment::IDByteRepresentationSize) {
                return typename M::template Key<T> {
                    env->id_from_bytes(
                        basic::ByteDataView { std::string_view {
                            reinterpret_cast<char const *>(&counter)
                            , sizeof(counter)
                        }}
                    )
                    , std::move(x)
                };
            } else {
                char buf[TheEnvironment::IDByteRepresentationSize];
                std::memset(buf, 0, TheEnvironment::IDByteRepresentationSize);
                std::memcpy(
                    buf
                    , reinterpret_cast<char const *>(&counter)
                    , std::min(sizeof(counter), TheEnvironment::IDByteRepresentationSize)
                );
                return typename M::template Key<T> {
                    env->id_from_bytes(
                        basic::ByteDataView { std::string_view {
                            buf
                            , TheEnvironment::IDByteRepresentationSize
                        }}
                    )
                    , std::move(x)
                };
            }
        }

        template <class T, class CounterMarkType=NotConstructibleStruct>
        static auto keyifyThroughCounter() {
            if constexpr (std::is_same_v<CounterMarkType, void>) {
                return keyify<T>();
            } else {
                return infra::KleisliUtils<M>::template kleisli<T>(
                    [](typename M::template InnerData<T> &&x) -> typename M::template Data<infra::Key<T,TheEnvironment>>
                    {
                        auto counter = x.environment->getCounterValue((CounterMarkType *) nullptr);
                        if constexpr (sizeof(counter) == TheEnvironment::IDByteRepresentationSize) {
                            return typename M::template InnerData<infra::Key<T,TheEnvironment>> {
                                x.environment 
                                , {
                                    x.environment->resolveTime(x.timedData.timePoint)
                                    , {
                                        x.environment->id_from_bytes(
                                            basic::ByteDataView { std::string_view {
                                                reinterpret_cast<char const *>(&counter)
                                                , sizeof(counter)
                                            }}
                                        )
                                        , std::move(x.timedData.value)
                                    }
                                    , x.timedData.finalFlag
                                }
                            };
                        } else {
                            char buf[TheEnvironment::IDByteRepresentationSize];
                            std::memset(buf, 0, TheEnvironment::IDByteRepresentationSize);
                            std::memcpy(
                                buf
                                , reinterpret_cast<char const *>(&counter)
                                , std::min(sizeof(counter), TheEnvironment::IDByteRepresentationSize)
                            );
                            return typename M::template InnerData<infra::Key<T,TheEnvironment>> {
                                x.environment 
                                , {
                                    x.environment->resolveTime(x.timedData.timePoint)
                                    , {
                                        x.environment->id_from_bytes(
                                            basic::ByteDataView { std::string_view {
                                                buf
                                                , TheEnvironment::IDByteRepresentationSize
                                            }}
                                        )
                                        , std::move(x.timedData.value)
                                    }
                                    , x.timedData.finalFlag
                                }
                            };
                        }
                    }
                );
            }
        }

        template <class T>
        static auto keyifyWithProvidedID() {
            class LocalA {
            private:
                std::optional<typename M::EnvironmentType::IDType> id_;
                std::deque<T> notProcessed_;
            public:
                LocalA() : id_(std::nullopt), notProcessed_() {}
                std::vector<typename M::template Key<T>> operator()(
                    std::variant<T,typename M::EnvironmentType::IDType> &&input
                ) {
                    return std::visit([this](auto &&x) -> std::vector<typename M::template Key<T>> {
                        using X = std::decay_t<decltype(x)>;
                        if constexpr (std::is_same_v<X, T>) {
                            if (id_) {
                                return {typename M::template Key<T> {
                                    *id_, std::move(x)
                                }};
                            } else {
                                notProcessed_.push_back(std::move(x));
                                return {};
                            }
                        } else {
                            id_ = x;
                            std::vector<typename M::template Key<T>> v;
                            while (!notProcessed_.empty()) {
                                v.push_back(typename M::template Key<T> {
                                    *id_, std::move(notProcessed_.front())
                                });
                                notProcessed_.pop_front();
                            }
                            return v;
                        }
                    }, std::move(input));
                }
            };
            return M::template liftMulti2<T,typename M::EnvironmentType::IDType>(LocalA());
        }

        template <class T>
        static auto shareBetweenDownstream() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> std::shared_ptr<T const>
                {
                    return std::make_shared<T const>(std::move(x));
                }
            );
        }

        template <class A, class B>
        static auto extractDataFromKeyedData() {
            return infra::KleisliUtils<M>::template liftPure<infra::KeyedData<A,B,TheEnvironment>>(
                [](infra::KeyedData<A,B,TheEnvironment> &&x) -> B
                {
                    return std::move(x.data);
                }
            );
        }

        template <class A, class B>
        static auto extractIDAndDataFromKeyedData() {
            return infra::KleisliUtils<M>::template liftPure<infra::KeyedData<A,B,TheEnvironment>>(
                [](infra::KeyedData<A,B,TheEnvironment> &&x) -> infra::Key<B,TheEnvironment>
                {
                    return {std::move(x.key.id()), std::move(x.data)};
                }
            );
        }

        template <class A, class B>
        static auto extractIDStringAndDataFromKeyedData() {
            return infra::KleisliUtils<M>::template liftPure<infra::KeyedData<A,B,TheEnvironment>>(
                [](infra::KeyedData<A,B,TheEnvironment> &&x) -> std::tuple<std::string, B>
                {
                    return {TheEnvironment::id_to_string(x.key.id()), std::move(x.data)};
                }
            );
        }

        template <class T>
        static auto duplicateInput() {
            return infra::KleisliUtils<M>::template kleisli<T>(
                [](typename M::template InnerData<T> &&x) -> typename M::template Data<std::tuple<T,T>>
                {
                    auto xCopy = infra::withtime_utils::makeCopy<typename M::TimePoint, T>(x.timedData);
                    return typename M::template InnerData<std::tuple<T,T>> {
                        x.environment
                        , {
                            x.timedData.timePoint
                            , {std::move(x.timedData.value), std::move(xCopy.value)}
                            , x.timedData.finalFlag
                        }
                    };
                }
            );
        }
        template <class A, class B>
        static auto switchTupleOrder() {
            return infra::KleisliUtils<M>::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> std::tuple<B,A>
                {
                    return {std::move(std::get<1>(x)), std::move(std::get<0>(x))};
                }
            );
        }
        template <class A, class B>
        static auto dropLeft() {
            return infra::KleisliUtils<M>::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> B
                {
                    return std::get<1>(x);
                }
            );
        }
        template <class A, class B>
        static auto dropRight() {
            return infra::KleisliUtils<M>::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> A
                {
                    return std::get<0>(x);
                }
            );
        }
    private:
        template <class A, class B, class F>
        class PreserveLeft {
        private:
            F f_;
        public:
            using InputType = std::tuple<A,B>;

            PreserveLeft(F &&f) : f_(std::move(f)) {}

            using C = typename decltype(f_(std::move(* (typename M::template InnerData<B> *)nullptr)))::value_type::ValueType;
            typename M::template Data<std::tuple<A,C>> operator()(typename M::template InnerData<std::tuple<A,B>> &&x) {
                auto aValue = std::get<0>(x.timedData.value);
                auto bData = M::template pureInnerDataLift<std::tuple<A,B>>([](std::tuple<A,B> &&t) -> B {
                    return std::get<1>(std::move(t));
                }, std::move(x));
                auto cData = f_(std::move(bData));
                if (cData) {
                    return M::template pureInnerDataLift<C>([aValue=std::move(aValue)](C &&c) -> std::tuple<A,C> {
                        return {std::move(aValue), std::move(c)};
                    }, std::move(*cData));
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class A, class B, class F>
        static PreserveLeft<A,B,F> preserveLeft(F &&f) {
            return PreserveLeft<A,B,F>(std::move(f));
        }
    private:
        template <class A, class B, class F>
        class PreserveRight {
        private:
            F f_;
        public:
            using InputType = std::tuple<A,B>;

            PreserveRight(F &&f) : f_(std::move(f)) {}

            using C = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;
            typename M::template Data<std::tuple<C,B>> operator()(typename M::template InnerData<std::tuple<A,B>> &&x) {
                auto bValue = std::get<1>(x.timedData.value);
                auto aData = M::template pureInnerDataLift<std::tuple<A,B>>([](std::tuple<A,B> &&t) -> A {
                    return std::get<0>(std::move(t));
                }, std::move(x));
                auto cData = f_(std::move(aData));
                if (cData) {
                    return M::template pureInnerDataLift<C>([bValue=std::move(bValue)](C &&c) -> std::tuple<C,B> {
                        return {std::move(c), std::move(bValue)};
                    }, std::move(*cData));
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class A, class B, class F>
        static PreserveRight<A,B,F> preserveRight(F &&f) {
            return PreserveRight<A,B,F>(std::move(f));
        }
    private:
        template <class A, class B, class F, class G>
        class ParallelArrow {
        private:
            F f_;
            G g_;
        public:
            using InputType = std::tuple<A,B>;

            ParallelArrow(F &&f, G &&g) : f_(std::move(f)), g_(std::move(g)) {}

            using C = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;
            using D = typename decltype(g_(std::move(* (typename M::template InnerData<B> *)nullptr)))::value_type::ValueType;

            typename M::template Data<std::tuple<C,D>> operator()(typename M::template InnerData<std::tuple<A,B>> &&x) {
                auto aData = M::template pureInnerDataLift<std::tuple<A,B>>([](std::tuple<A,B> &&t) -> A {
                    return std::move(std::get<0>(t));
                }, std::move(x));
                auto bData = M::template pureInnerDataLift<std::tuple<A,B>>([](std::tuple<A,B> &&t) -> B {
                    return std::move(std::get<1>(t));
                }, std::move(x));
                auto cData = f_(std::move(aData));
                auto dData = g_(std::move(bData));
                if (cData && dData) {
                    return {typename M::template InnerData<std::tuple<C,D>> {
                        x.environment
                        , {std::max(cData->timedData.timePoint, dData->timedData.timePoint)
                            , std::tuple<C,D> {std::move(cData->timedData.value), std::move(dData->timedData.value)}
                            , (cData->timedData.finalFlag && dData->timedData.finalFlag)
                        }
                    }};
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        //WARNING: This is not a "parallel" processing component, it is simply
        //a way to express two "parallel" arrows in a graph. For actually parallelizing
        //some operation with multiple threads, please refer to "parallel_withSeparatorNode"
        //and "parallel_withoutSeparatorNode" in AppRunnerUtils.hpp
        template <class A, class B, class F, class G>
        static ParallelArrow<A,B,F,G> parallelArrow(F &&f, G &&g) {
            return ParallelArrow<A,B,F,G>(std::move(f), std::move(g));
        }
    private:
        template <class A, class F, class G>
        class FanOut {
        private:
            F f_;
            G g_;
        public:
            using InputType = A;

            FanOut(F &&f, G &&g) : f_(std::move(f)), g_(std::move(g)) {}

            using B = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;
            using C = typename decltype(g_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;

            typename M::template Data<std::tuple<B,C>> operator()(typename M::template InnerData<A> &&x) {
                auto aCopy = typename M::template InnerData<A> {
                    x.environment, infra::withtime_utils::makeCopy(x.timedData)
                };
                auto bData = f_(std::move(x));
                auto cData = g_(std::move(aCopy));
                if (bData && cData) {
                    return {typename M::template InnerData<std::tuple<B,C>> {
                        x.environment
                        , {std::max(bData->timedData.timePoint, cData->timedData.timePoint)
                            , std::tuple<B,C> {std::move(bData->timedData.value), std::move(cData->timedData.value)}
                            , (bData->timedData.finalFlag && cData->timedData.finalFlag)
                        }
                    }};
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class A, class F, class G>
        static FanOut<A,F,G> fanOut(F &&f, G &&g) {
            return FanOut<A,F,G>(std::move(f), std::move(g));
        }

    private:
        template <class A, class F>
        class WithKey {
        private:
            F f_;
        public:
            using InputType = typename M::template Key<A>;

            WithKey(F &&f) : f_(std::move(f)) {}

            using B = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;

            typename M::template Data<typename M::template Key<B>> operator()(typename M::template InnerData<typename M::template Key<A>> &&x) {
                auto a = typename M::template InnerData<A> {
                    x.environment, {x.timedData.timePoint, x.timedData.value.key(), x.timedData.finalFlag}
                };
                auto id = x.timedData.value.id();
                auto b = f_(std::move(a));
                if (b) {
                    return {typename M::template InnerData<typename M::template Key<B>> {
                        b->environment
                        , {b->timedData.timePoint
                            , typename M::template Key<B> {std::move(id), std::move(b->timedData.value)}
                            , b->timedData.finalFlag 
                        }
                    }};
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class A, class F>
        static WithKey<A,F> withKey(F &&innerFunc) {
            return WithKey<A,F>(std::move(innerFunc));
        }

    private:
        template <class K, class A, class F>
        class WithKeyAndInput {
        private:
            F f_;
        public:
            using InputType = typename M::template KeyedData<K,A>;

            WithKeyAndInput(F &&f) : f_(std::move(f)) {}

            using B = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;

            typename M::template Data<typename M::template KeyedData<K,B>> operator()(typename M::template InnerData<typename M::template KeyedData<K,A>> &&x) {
                auto a = typename M::template InnerData<A> {
                    x.environment, {x.timedData.timePoint, std::move(x.timedData.value.data), x.timedData.finalFlag}
                };
                auto input = std::move(x.timedData.value.key);
                auto b = f_(std::move(a));
                if (b) {
                    return {typename M::template InnerData<typename M::template KeyedData<K,B>> {
                        b->environment
                        , {b->timedData.timePoint
                            , typename M::template KeyedData<K,B> {std::move(input), std::move(b->timedData.value)}
                            , b->timedData.finalFlag 
                        }
                    }};
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class K, class A, class F>
        static WithKeyAndInput<K,A,F> withKeyAndInput(F &&innerFunc) {
            return WithKeyAndInput<K,A,F>(std::move(innerFunc));
        }

    private:
        template <class V, class A, class F, class Cmp=std::less<V>>
        class WithVersion {
        private:
            F f_;
        public:
            using InputType = infra::VersionedData<V,A,Cmp>;

            WithVersion(F &&f) : f_(std::move(f)) {}

            using B = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;

            typename M::template Data<infra::VersionedData<V,B,Cmp>> operator()(typename M::template InnerData<infra::VersionedData<V,A,Cmp>> &&x) {
                auto a = typename M::template InnerData<A> {
                    x.environment, {x.timedData.timePoint, std::move(x.timedData.value.data), x.timedData.finalFlag}
                };
                auto v = std::move(x.timedData.value.version);
                auto b = f_(std::move(a));
                if (b) {
                    return {typename M::template InnerData<infra::VersionedData<V,B,Cmp>> {
                        b->environment
                        , {b->timedData.timePoint
                            , infra::VersionedData<V,B,Cmp> {std::move(v), std::move(b->timedData.value)}
                            , b->timedData.finalFlag 
                        }
                    }};
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class V, class A, class F, class Cmp=std::less<V>>
        static WithVersion<V,A,F,Cmp> withVersion(F &&innerFunc) {
            return WithVersion<V,A,F,Cmp>(std::move(innerFunc));
        }

    private:
        template <class G, class V, class A, class F, class Cmp=std::less<V>>
        class WithGroupAndVersion {
        private:
            F f_;
        public:
            using InputType = infra::GroupedVersionedData<G,V,A,Cmp>;

            WithGroupAndVersion(F &&f) : f_(std::move(f)) {}

            using B = typename decltype(f_(std::move(* (typename M::template InnerData<A> *)nullptr)))::value_type::ValueType;

            typename M::template Data<infra::GroupedVersionedData<G,V,B,Cmp>> operator()(typename M::template InnerData<infra::GroupedVersionedData<G,V,A,Cmp>> &&x) {
                auto a = typename M::template InnerData<A> {
                    x.environment, {x.timedData.timePoint, std::move(x.timedData.value.data), x.timedData.finalFlag}
                };
                auto g = std::move(x.timedData.value.groupID);
                auto v = std::move(x.timedData.value.version);
                auto b = f_(std::move(a));
                if (b) {
                    return {typename M::template InnerData<infra::GroupedVersionedData<G,V,B,Cmp>> {
                        b->environment
                        , {b->timedData.timePoint
                            , infra::GroupedVersionedData<G,V,B,Cmp> {std::move(g), std::move(v), std::move(b->timedData.value)}
                            , b->timedData.finalFlag 
                        }
                    }};
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class G, class V, class A, class F, class Cmp=std::less<V>>
        static WithGroupAndVersion<G,V,A,F,Cmp> withGroupAndVersion(F &&innerFunc) {
            return WithGroupAndVersion<G,V,A,F,Cmp>(std::move(innerFunc));
        }
        
    private:
        template <class A, class F>
        class PureFilter {
        private:
            F f_;
        public:
            using InputType = A;

            PureFilter(F &&f) : f_(std::move(f)) {}

            typename M::template Data<A> operator()(typename M::template InnerData<A> &&x) {
                if (f_(x.timedData.value)) {
                    return std::move(x);
                } else {
                    return std::nullopt;
                }
            }
        };
        template <class A, class F>
        class EnhancedPureFilter {
        private:
            F f_;
        public:
            using InputType = A;

            EnhancedPureFilter(F &&f) : f_(std::move(f)) {}

            typename M::template Data<A> operator()(typename M::template InnerData<A> &&x) {
                if (f_(std::tuple<typename M::TimePoint, A> {x.timedData.timePoint, x.timedData.value})) {
                    return std::move(x);
                } else {
                    return std::nullopt;
                }
            }
        };
        template <class A, class F>
        class PureTwoWayFilter {
        private:
            F f_;
        public:
            using InputType = A;

            PureTwoWayFilter(F &&f) : f_(std::move(f)) {}

            typename M::template Data<std::variant<A,A>> operator()(typename M::template InnerData<A> &&x) {
                if (f_(x.timedData.value)) {
                    return std::move(x).mapMove([](A &&a) {
                        return std::variant<A,A> {std::in_place_index<0>, std::move(a)};
                    });
                } else {
                    return std::move(x).mapMove([](A &&a) {
                        return std::variant<A,A> {std::in_place_index<1>, std::move(a)};
                    });
                }
            }
        };
        template <class A, class F>
        class EnhancedPureTwoWayFilter {
        private:
            F f_;
        public:
            using InputType = A;

            EnhancedPureTwoWayFilter(F &&f) : f_(std::move(f)) {}

            typename M::template Data<A> operator()(typename M::template InnerData<A> &&x) {
                if (f_(std::tuple<typename M::TimePoint, A> {x.timedData.timePoint, x.timedData.value})) {
                    return std::move(x).mapMove([](A &&a) {
                        return std::variant<A,A> {std::in_place_index<0>, std::move(a)};
                    });
                } else {
                    return std::move(x).mapMove([](A &&a) {
                        return std::variant<A,A> {std::in_place_index<1>, std::move(a)};
                    });
                }
            }
        };
    public:
        template <class A, class F>
        static PureFilter<A,F> pureFilter(F &&f) {
            return PureFilter<A,F>(std::move(f));
        }
        template <class A, class F>
        static EnhancedPureFilter<A,F> enhancedPureFilter(F &&f) {
            return EnhancedPureFilter<A,F>(std::move(f));
        }
        template <class A, class F>
        static PureTwoWayFilter<A,F> pureTwoWayFilter(F &&f) {
            return PureTwoWayFilter<A,F>(std::move(f));
        }
        template <class A, class F>
        static EnhancedPureTwoWayFilter<A,F> enhancedPureTwoWayFilter(F &&f) {
            return EnhancedPureTwoWayFilter<A,F>(std::move(f));
        }
        
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> simpleFinalizer() {
            return M::template liftPure<T>([](T &&t) {
                return std::move(t);
            }, infra::LiftParameters<typename M::TimePoint>().FireOnceOnly(true));
        }

    private:
        template <class T, class PushPopHandler>
        class MovingTimeWindow {
        public:
            using Duration = decltype(typename M::TimePoint {}-typename M::TimePoint {});
        private:
            Duration duration_;
        public:
            using InputType = T;
            using StateType = std::deque<std::tuple<typename M::TimePoint, T>>;
            MovingTimeWindow(Duration const &duration) : duration_(duration) {}
            void handleData(StateType *state, PushPopHandler const &handler, std::tuple<typename M::TimePoint, T> &&data) {
                handleTimePoint(std::get<0>(data.timePoint));
                state->push_back(std::move(data));
                handler.handlePush(state->back());
            }
            void handleTimePoint(StateType *state, PushPopHandler const &handler, typename M::TimePoint const &tp) {
                while (!state->empty() && state->front().timePoint+duration_ < tp) {
                    handler.handlePop(state->front());
                    state->pop_front();
                }
            }
        };
        
        //Requirement for push/pop handler:
        //void handlePush(std::tuple<typename M::TimePoint, T> const &);
        //void handlePop(std::tuple<typename M::TimePoint, T> const &);
        //std::optional<...> readResult(std::deque<std::tuple<typename M::TimePoint, T>> const &)
        template <class T, class PushPopHandler>
        class SimpleMovingTimeWindowKleisli {
        private:
            using MTW = MovingTimeWindow<T,PushPopHandler>;
            using State = typename MTW::StateType;
        public:
            using Duration = typename MTW::Duration;
            using Result = typename decltype(((PushPopHandler *) nullptr)->readResult(
                (*(State *) nullptr)
            ))::value_type;
        private:
            PushPopHandler handler_;
            MTW w_;
            State state_;
        public:
            using InputType = T;

            SimpleMovingTimeWindowKleisli(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
            typename M::template Data<Result> operator()(typename M::template InnerData<T> &&a) {
                auto *p = a.environment;
                auto tp = a.timedData.timePoint;
                w_.handleData(&state_, handler_, std::tuple<typename M::TimePoint, T> {std::move(a.timedData.timePoint), std::move(a.timedData.value)});
                auto res = handler_.readResult(state_);
                if (res) {
                    return { typename M::template InnerData<Result> {
                        p,
                        { p->resolveTime(tp), *res, a.timedData.finalFlag }
                    } };
                } else {
                    return std::nullopt;
                }
            }
        };
        
        //Requirement for push/pop handler:
        //void handlePush(std::tuple<typename M::TimePoint, A> const &);
        //void handlePop(std::tuple<typename M::TimePoint, A> const &);
        //std::optional<...> readResult(std::deque<std::tuple<typename M::TimePoint, A>> const &, std::tuple<typename M::TimePoint, B> &&)
        template <class A, class B, class PushPopHandler>
        class ComplexMovingTimeWindowKleisli {
        private:
            using MTW = MovingTimeWindow<A,PushPopHandler>;
            using State = typename MTW::StateType;
        public:
            using Duration = typename MTW::Duration;
            using Result = typename decltype(((PushPopHandler *) nullptr)->readResult(
                (*(State *) nullptr)
                , std::move(* ((std::tuple<typename M::TimePoint, B> *) nullptr))
            ))::value_type;
        private:
            PushPopHandler handler_;
            MTW w_;
            State state_;
        public:
            using InputType = std::variant<A,B>;

            ComplexMovingTimeWindowKleisli(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
            typename M::template Data<Result> operator()(typename M::template InnerData<std::variant<A,B>> &&x) {
                int which = x.index();
                switch (which) {
                    case 0:
                        {
                            w_.handleData(&state_, handler_, std::tuple<typename M::TimePoint, A> {std::move(x.timedData.timePoint), std::move(std::get<0>(x.timedData.value))});
                            return std::nullopt;
                        }
                        break;
                    case 1:
                        {
                            auto tp = x.timedData.timePoint;
                            w_.handleTimePoint(&state_, handler_, tp);
                            auto res = handler_.readResult(state_, std::tuple<typename M::TimePoint, B> {std::move(x.timedData.timePoint), std::move(std::get<1>(x.timedData.value))});
                            if (res) {
                                return { typename M::template InnerData<Result> {
                                    x.environment,
                                    { x.environment->resolveTime(tp), *res, x.timedData.finalFlag }
                                } };
                            } else {
                                return std::nullopt;
                            }
                        }
                        break;
                    default:
                        return std::nullopt;
                }
            }
        };

    public:
        template <class T, class PushPopHandler>
        static SimpleMovingTimeWindowKleisli<T,PushPopHandler> simpleMovingTimeWindowKleisli(
            PushPopHandler &&handler
            , typename MovingTimeWindow<T,PushPopHandler>::Duration const &duration
        ) {
            return SimpleMovingTimeWindowKleisli<T,PushPopHandler>(std::move(handler), duration);
        }
        template <class A, class B, class PushPopHandler>
        static auto complexMovingTimeWindowActionKleisli(
            PushPopHandler &&handler
            , typename MovingTimeWindow<A,PushPopHandler>::Duration const &duration
        ) {
            return ComplexMovingTimeWindowKleisli<A,B,PushPopHandler>(std::move(handler), duration);
        }
    private:
        //Requirement for whole history handler:
        //void add(std::tuple<typename M::TimePoint, T> const &);
        //std::optional<...> readResult()
        template <class T, class WholeHistoryHandler>
        class WholeHistoryFold {
        public:
            using Result = typename decltype(((WholeHistoryHandler *) nullptr)->readResult())::value_type;
        private:
            WholeHistoryHandler handler_;
        public:
            using InputType = T;

            WholeHistoryFold(WholeHistoryHandler &&handler) : handler_(std::move(handler)) {}
            typename M::template Data<Result> operator()(typename M::template InnerData<T> &&a) {
                auto tp = a.timedData.timePoint;
                handler_.add(std::tuple<typename M::TimePoint, T> {std::move(a.timedData.timePoint), std::move(a.timedData.value)});
                auto res = handler_.readResult();
                if (res) {
                    return { typename M::template InnerData<Result> {
                        a.environment,
                        { a.environment->resolveTime(tp), *res, a.timedData.finalFlag }
                    } };
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class T, class WholeHistoryHandler>
        static auto wholeHistoryFold(WholeHistoryHandler &&handler) {
            return WholeHistoryFold<T,WholeHistoryHandler>(std::move(handler));
        }
    public:
        template <class Input, class Output, class ToSupplyDefaultValue>
        static std::shared_ptr<typename M::template OnOrderFacility<
            Input,Output>
        > wrapTuple2OnOrderFacilityBySupplyingDefaultValue(
            typename M::template OnOrderFacility<
                std::tuple<ToSupplyDefaultValue, Input>, Output
            > &&toBeWrapped
            , ToSupplyDefaultValue const &v = ToSupplyDefaultValue()
        ) {
            ToSupplyDefaultValue vCopy = v;
            return M::template wrappedOnOrderFacility<
                Input, Output
                , std::tuple<ToSupplyDefaultValue, Input>, Output
            >(
                std::move(toBeWrapped)
                , std::move(*(M::template kleisli<typename M::template Key<Input>>(
                    withKey<Input>(
                        infra::KleisliUtils<M>::template liftPure<Input>(
                            [vCopy](Input &&x) -> std::tuple<ToSupplyDefaultValue, Input> {
                                return {vCopy, std::move(x)};
                            }
                        )
                    )
                )))
                , std::move(*(M::template kleisli<typename M::template Key<Output>>(
                    idFunc<typename M::template Key<Output>>()
                )))
            );
        }
    public:
        template <class VariantType, class OneOfVariants>
        static auto pickOne() {
            return infra::KleisliUtils<M>::template liftMaybe<VariantType>(
                [](VariantType &&x) -> std::optional<OneOfVariants>
                {
                    return std::visit([](auto &&y) -> std::optional<OneOfVariants> {
                        using T = std::decay_t<decltype(y)>;
                        if constexpr (std::is_same_v<T, OneOfVariants>) {
                            return std::optional<OneOfVariants> {
                                std::move(y)
                            };
                        } else {
                            return std::nullopt;
                        }
                    }, std::move(x));
                }
            );
        }
        template <class WrappedVariantType, class OneOfVariants>
        static auto pickOneFromWrappedValue() {
            return infra::KleisliUtils<M>::template liftMaybe<WrappedVariantType>(
                [](WrappedVariantType &&x) -> std::optional<OneOfVariants>
                {
                    return std::visit([](auto &&y) -> std::optional<OneOfVariants> {
                        using T = std::decay_t<decltype(y)>;
                        if constexpr (std::is_same_v<T, OneOfVariants>) {
                            return std::optional<OneOfVariants> {
                                std::move(y)
                            };
                        } else {
                            return std::nullopt;
                        }
                    }, std::move(x.value));
                }
            );
        }
    private:
        template <class A, class B, class F, bool MutexProtected=M::PossiblyMultiThreaded>
        class Synchronizer2 {
        private:
            F f_;
            std::deque<A> a_;
            std::deque<B> b_;
            std::conditional_t<
                (MutexProtected && M::PossiblyMultiThreaded)
                , std::mutex
                , bool
            > mutex_;
        public:
            Synchronizer2(F &&f) : f_(std::move(f)), a_(), b_(), mutex_() {}
            Synchronizer2(Synchronizer2 &&s) : f_(std::move(s.f_)), a_(std::move(s.a_)), b_(std::move(s.b_)), mutex_() {}
            Synchronizer2 &operator=(Synchronizer2 &&) = delete;
            Synchronizer2(Synchronizer2 const &) = delete;
            Synchronizer2 &operator=(Synchronizer2 const &) = delete;

            using C = decltype(f_(std::move(*((A *) nullptr)), std::move(*((B *) nullptr))));
            std::optional<C> operator()(std::variant<A,B> &&data) {
                if (data.index() == 0) {
                    std::conditional_t<
                        (MutexProtected && M::PossiblyMultiThreaded)
                        , std::lock_guard<std::mutex>
                        , bool
                    > _(mutex_);
                    if (b_.empty()) {
                        a_.push_back(std::move(std::get<0>(data)));
                        return std::nullopt;
                    } else {
                        C c = f_(std::move(std::get<0>(data)), std::move(b_.front()));
                        b_.pop_front();
                        return {std::move(c)};
                    }
                } else {
                    std::conditional_t<
                        (MutexProtected && M::PossiblyMultiThreaded)
                        , std::lock_guard<std::mutex>
                        , bool
                    > _(mutex_);
                    if (a_.empty()) {
                        b_.push_back(std::move(std::get<1>(data)));
                        return std::nullopt;
                    } else {
                        C c = f_(std::move(a_.front()), std::move(std::get<1>(data)));
                        a_.pop_front();
                        return {std::move(c)};
                    }
                }
            }
        };
        template <class A, class B, class KeyExtractorA, class KeyExtractorB, class F>
        class KeyedSynchronizer2 {
        private:
            KeyExtractorA keyExtractorA_;
            KeyExtractorB keyExtractorB_;
            F f_;
            using Key = decltype((* ((KeyExtractorA *) nullptr))(std::declval<A>()));
            struct Item {
                std::deque<A> a_;
                std::deque<B> b_;
            };
            using Hash = typename common_flow_util_helper::KeyedSynchronizer2DefaultHash<M,decltype((* ((KeyExtractorA *) nullptr))(std::declval<A>()))>::TheType;
            std::unordered_map<Key, Item, Hash> data_;
            std::conditional_t<
                M::PossiblyMultiThreaded
                , std::mutex
                , bool
            > mutex_;
        public:
            KeyedSynchronizer2(KeyExtractorA &&keyExtractorA, KeyExtractorB &&keyExtractorB, F &&f) : keyExtractorA_(std::move(keyExtractorA)), keyExtractorB_(std::move(keyExtractorB)), f_(std::move(f)), data_(), mutex_() {}
            KeyedSynchronizer2(KeyedSynchronizer2 &&s) : keyExtractorA_(std::move(s.keyExtractorA_)), keyExtractorB_(std::move(s.keyExtractorB_)), f_(std::move(s.f_)), data_(std::move(s.data_)), mutex_() {}
            KeyedSynchronizer2 &operator=(KeyedSynchronizer2 &&) = delete;
            KeyedSynchronizer2(KeyedSynchronizer2 const &) = delete;
            KeyedSynchronizer2 &operator=(KeyedSynchronizer2 const &) = delete;

            using C = decltype(f_(std::move(*((A *) nullptr)), std::move(*((B *) nullptr))));
            std::optional<C> operator()(std::variant<A,B> &&data) {
                if (data.index() == 0) {
                    std::conditional_t<
                        M::PossiblyMultiThreaded
                        , std::lock_guard<std::mutex>
                        , bool
                    > _(mutex_);
                    auto k = keyExtractorA_(std::get<0>(data));
                    auto &rec = data_[k];
                    if (rec.b_.empty()) {
                        rec.a_.push_back(std::move(std::get<0>(data)));
                        return std::nullopt;
                    } else {
                        C c = f_(std::move(std::get<0>(data)), std::move(rec.b_.front()));
                        rec.b_.pop_front();
                        return {std::move(c)};
                    }
                } else {
                    std::conditional_t<
                        M::PossiblyMultiThreaded
                        , std::lock_guard<std::mutex>
                        , bool
                    > _(mutex_);
                    auto k = keyExtractorB_(std::get<1>(data));
                    auto &rec = data_[k];
                    if (rec.a_.empty()) {
                        rec.b_.push_back(std::move(std::get<1>(data)));
                        return std::nullopt;
                    } else {
                        C c = f_(std::move(rec.a_.front()), std::move(std::get<1>(data)));
                        rec.a_.pop_front();
                        return {std::move(c)};
                    }
                }
            }
        };
    public:
        template <class A, class B, class F>
        static std::shared_ptr<typename M::template Action<
            std::variant<A,B>, typename Synchronizer2<A,B,F,M::PossiblyMultiThreaded>::C
        >> synchronizer2(F &&f, bool useMutex=M::PossiblyMultiThreaded) {
            if (useMutex) {
                return M::template liftMaybe2<A,B>(Synchronizer2<A,B,F,true>(std::move(f)));
            } else {
                return M::template liftMaybe2<A,B>(Synchronizer2<A,B,F,false>(std::move(f)));
            }
        }
        template <class A, class B, class KeyExtractorA, class KeyExtractorB, class F>
        static std::shared_ptr<typename M::template Action<
            std::variant<A,B>, typename KeyedSynchronizer2<A,B,KeyExtractorA,KeyExtractorB,F>::C
        >> keyedSynchronizer2(KeyExtractorA &&keyExtractorA, KeyExtractorB &&keyExtractorB, F &&f) {
            return M::template liftMaybe2<A,B>(KeyedSynchronizer2<A,B,KeyExtractorA,KeyExtractorB,F>(std::move(keyExtractorA), std::move(keyExtractorB), std::move(f)));
        }

    public:
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> delayer(
            decltype(typename M::TimePoint {}-typename M::TimePoint {}) duration
        ) {
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                class Delayer : public M::template AbstractAction<T,T> {
                private:
                    using DurationType = decltype(typename M::TimePoint {}-typename M::TimePoint {});
                    DurationType duration_;

                    std::deque<typename M::template InnerData<T>> data_;
                    std::deque<typename M::template InnerData<T>> toSend_;
                    std::mutex mutex_;
                    std::thread th_;
                    std::atomic<bool> running_;
                    typename M::EnvironmentType *env_;

                    void run() {
                        while (running_) {
                            std::optional<typename M::TimePoint> nextTp = std::nullopt;
                            typename M::EnvironmentType *env = nullptr;
                            {
                                std::lock_guard<std::mutex> _(mutex_);
                                env = env_;
                                if (!data_.empty()) {
                                    auto now = env_->now();
                                    while (!data_.empty()) {
                                        auto tp = data_.front().timedData.timePoint;
                                        if (now >= tp+duration_) {
                                            toSend_.push_back(typename M::template InnerData<T> {
                                                env 
                                                , {
                                                    now 
                                                    , std::move(data_.front().timedData.value)
                                                    , data_.front().timedData.finalFlag
                                                }
                                            });
                                            data_.pop_front();
                                        } else {
                                            nextTp = tp+duration_;
                                            break;
                                        }
                                    }
                                }
                            }
                            while (!toSend_.empty()) {
                                this->publish(std::move(toSend_.front()));
                                toSend_.pop_front();
                            }
                            if (!env) {
                                std::this_thread::sleep_for(std::chrono::microseconds(100));
                            } else {
                                auto n = env->now();
                                if (nextTp && *nextTp > n) {
                                    env->sleepFor(*nextTp-n);
                                } else {
                                    env->sleepFor(duration_);
                                }
                            }
                        }
                    }
                public:
                    Delayer(DurationType duration) : duration_(duration), data_(), toSend_(), mutex_(), th_(), running_(true), env_(nullptr) {
                        th_ = std::thread([this]() {
                            run();
                        });
                        th_.detach();
                    }
                    virtual ~Delayer() {
                        running_ = false;
                        try {
                            if (th_.joinable()) {
                                th_.join();
                            }
                        } catch (std::exception const &) {}
                    }
                    virtual bool isThreaded() const override final {
                        return true;
                    }
                    virtual bool isOneTimeOnly() const override final {
                        return false;
                    }
                    virtual void setIdleWorker(std::function<void(void *)> worker) override final {}
                    virtual void setStartWaiter(std::function<void()> waiter) override final {}
                    virtual void handle(typename M::template InnerData<T> &&data) override {
                        std::lock_guard<std::mutex> _(mutex_);
                        env_ = data.environment;
                        data_.push_back(std::move(data));
                    }
                };
                return M::template fromAbstractAction<T,T>(new Delayer(duration));
            } else {
                return M::template kleisli<T>(
                    idFunc<T>()
                    , infra::LiftParameters<typename M::TimePoint>()
                        .DelaySimulator([duration](int, typename M::TimePoint const &) -> decltype(typename M::TimePoint()-typename M::TimePoint()) {
                            return duration;
                        })
                );
            }
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> expediter(
            decltype(typename M::TimePoint {}-typename M::TimePoint {}) duration
        ) {
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                return M::template passThroughAction<T>();
            } else {
                return CommonFlowUtilComponents<M>::template delayer<T>(-duration);
            }
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> rightShift(
            std::size_t steps
        ) {
            return M::template kleisli<T>(
                [steps](typename M::template InnerData<T> &&input) -> typename M::template Data<T> {
                    static std::deque<typename M::template InnerData<T>> buffer;
                    if (buffer.size() < steps) {
                        buffer.push_back(std::move(input));
                        return std::nullopt;
                    } else {
                        auto tp = input.environment->resolveTime(input.timedData.timePoint);
                        bool final = input.timedData.finalFlag;
                        buffer.push_back(std::move(input));
                        typename M::template InnerData<T> ret = std::move(buffer.front());
                        buffer.pop_front();
                        ret.timedData.timePoint = tp;
                        ret.timedData.finalFlag = final;
                        return ret;
                    }
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> leftShift(
            std::size_t steps
        ) {
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                //left shift does not make sense in real time app
                return M::template kleisli<T>(
                    idFunc<T>()
                );
            } else if constexpr (std::is_same_v<M, infra::SinglePassIterationApp<TheEnvironment>>) {
                struct State {
                    std::size_t steps;
                    std::deque<typename M::template InnerData<T>> buffer;
                    bool repeatCall;
                    std::optional<typename M::TimePoint> nextTimePoint() const {
                        if (buffer.empty()) {
                            return std::nullopt;
                        }
                        return buffer.front().timedData.timePoint;
                    }
                    State(std::size_t s) : steps(s), buffer(), repeatCall(false) {}
                    State(State const &) = default;
                    State(State &&) = default;
                    State &operator=(State const &) = default;
                    State &operator=(State &&) = default;
                    ~State() = default;
                };
                auto cont = [](typename M::template InnerData<T> &&input, State &state, std::function<void(typename M::template InnerData<T> &&)> handler) {                
                    if (state.repeatCall) {
                        state.repeatCall = false;
                        return;
                    }
                    state.buffer.push_back(std::move(input));
                    if (state.buffer.size() <= state.steps) {
                        return;
                    }
                    typename M::TimePoint tp = state.buffer.front().timedData.timePoint;
                    state.buffer.pop_front();
                    handler(
                        typename M::template InnerData<T> {
                            state.buffer.back().environment 
                            , {
                                tp
                                , std::move(state.buffer.back().timedData.value)
                                , state.buffer.back().timedData.finalFlag
                            }
                        }
                    );
                    state.repeatCall = true;
                };
                return M::template continuationAction<T,T,State>(cont, State(steps));
            } else if constexpr (std::is_same_v<M, infra::TopDownSinglePassIterationApp<TheEnvironment>>) {
                class LeftShiftAction : public M::template AbstractAction<T,T> {
                private:
                    std::deque<typename M::TimePoint> tps_;
                    std::size_t steps_;
                public:
                    LeftShiftAction(std::size_t steps) : tps_(), steps_(steps) {}
                    virtual ~LeftShiftAction() = default;
                    virtual bool isOneTimeOnly() const override final {
                        return false;
                    }
                    virtual void handle(typename M::template InnerData<T> &&data) override final {
                        if (tps_.size() < steps_) {
                            tps_.push_back(data.timedData.timePoint);
                        } else {
                            this->publish(typename M::template InnerData<T> {
                                data.environment 
                                , {
                                    tps_.front()
                                    , std::move(data.timedData.value)
                                    , data.timedData.finalFlag
                                }
                            });
                            tps_.pop_front();
                            tps_.push_back(data.timedData.timePoint);
                        }
                    }
                };
                return M::template fromAbstractAction<T, T>(new LeftShiftAction(steps));
            }
        }
    private:
        template <class A, class B, class F>
        class CollectAndPassDownOnRight {
        private:
            F f_;
            std::vector<A> a_;
        public:
            CollectAndPassDownOnRight(F &&f) : f_(std::move(f)), a_() {}
            CollectAndPassDownOnRight(CollectAndPassDownOnRight &&s) : f_(std::move(s.f_)), a_(std::move(s.a_)) {}
            CollectAndPassDownOnRight &operator=(CollectAndPassDownOnRight &&) = delete;
            CollectAndPassDownOnRight(CollectAndPassDownOnRight const &) = delete;
            CollectAndPassDownOnRight &operator=(CollectAndPassDownOnRight const &) = delete;

            using C = decltype(f_(std::move(*((std::vector<A> *) nullptr)), std::move(*((B *) nullptr))));
            std::optional<C> operator()(std::variant<A,B> &&data) {
                if (data.index() == 0) {
                    a_.push_back(std::get<0>(data));
                    return std::nullopt;
                } else {
                    if (!a_.empty()) {
                        auto c = f_(std::move(a_), std::move(std::get<1>(data)));
                        a_.clear();
                        return c;
                    } else {
                        return std::nullopt;
                    }
                }
            }
        };
    public:
        template <class A, class B, class F>
        static std::shared_ptr<typename M::template Action<
            std::variant<A,B>, typename CollectAndPassDownOnRight<A,B,F>::C
        >> collectAndPassDownOnRight(F &&f, bool suggestThreaded=false) {
            return M::template liftMaybe2<A,B>(
                CollectAndPassDownOnRight<A,B,F>(std::move(f))
                , infra::LiftParameters<typename M::TimePoint>()
                    .SuggestThreadedcollectAn(suggestThreaded)
            );
        }
    private:
        template <class A, class B, class FoldedType, class Folder, class Outputter>
        class FoldAndPassDownOnRight {
        private:
            Folder f_;
            Outputter o_;
            FoldedType t_;
        public:
            using C = decltype(o_(*((FoldedType *) nullptr), std::move(*((B *) nullptr))));
            
            FoldAndPassDownOnRight(Folder &&f, Outputter &&o) : f_(std::move(f)), o_(std::move(o)), t_() {}
            FoldAndPassDownOnRight(FoldAndPassDownOnRight &&s) : f_(std::move(s.f_)), o_(std::move(s.o_)), t_(std::move(s.t_)) {}
            FoldAndPassDownOnRight &operator=(FoldAndPassDownOnRight &&) = delete;
            FoldAndPassDownOnRight(FoldAndPassDownOnRight const &) = delete;
            FoldAndPassDownOnRight &operator=(FoldAndPassDownOnRight const &) = delete;

            std::optional<C> operator()(std::variant<A,B> &&data) {
                if (data.index() == 0) {
                    f_(t_, std::move(std::get<0>(data)));
                    return std::nullopt;
                } else {
                    return o_(t_, std::move(std::get<1>(data)));
                }
            }
        };
    public:
        template <class A, class B, class FoldedType, class Folder, class Outputter>
        static std::shared_ptr<typename M::template Action<
            std::variant<A,B>, typename FoldAndPassDownOnRight<A,B,FoldedType,Folder,Outputter>::C
        >> foldAndPassDownOnRight(Folder &&f, Outputter &&o, bool suggestThreaded=false) {
            return M::template liftMaybe2<A,B>(
                FoldAndPassDownOnRight<A,B,FoldedType,Folder,Outputter>(std::move(f), std::move(o))
                , infra::LiftParameters<typename M::TimePoint>()
                    .SuggestThreaded(suggestThreaded)
            );
        }
    private:
        template <class A, class B, class F>
        class SnapshotOnRight {
        private:
            F f_;
            std::optional<A> a_;
        public:
            SnapshotOnRight(F &&f) : f_(std::move(f)), a_(std::nullopt) {}
            SnapshotOnRight(SnapshotOnRight &&s) : f_(std::move(s.f_)), a_(std::move(s.a_)) {}
            SnapshotOnRight &operator=(SnapshotOnRight &&) = delete;
            SnapshotOnRight(SnapshotOnRight const &) = delete;
            SnapshotOnRight &operator=(SnapshotOnRight const &) = delete;

            using C = decltype(f_(std::move(*((A *) nullptr)), std::move(*((B *) nullptr))));
            std::optional<C> operator()(std::variant<A,B> &&data) {
                if (data.index() == 0) {
                    a_ = std::move(std::get<0>(data));
                    return std::nullopt;
                } else {
                    if (a_) {
                        return f_(infra::withtime_utils::makeValueCopy(*a_), std::move(std::get<1>(data)));
                    } else {
                        return std::nullopt;
                    }
                }
            }
        };
    public:
        template <class A, class B, class F>
        static std::shared_ptr<typename M::template Action<
            std::variant<A,B>, typename SnapshotOnRight<A,B,F>::C
        >> snapshotOnRight(F &&f, bool suggestThreaded=false) {
            return M::template liftMaybe2<A,B>(
                SnapshotOnRight<A,B,F>(std::move(f))
                , infra::LiftParameters<typename M::TimePoint>()
                    .SuggestThreaded(suggestThreaded)
            );
        }

    private:
        template <class T, class Comparer=void>
        static auto mergedImporter_(
            std::list<typename M::template AbstractImporter<T> *> const &underlyingImporters
            , std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> const &comparer = std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer>()
        )
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            if constexpr (std::is_same_v<M, infra::BasicWithTimeApp<TheEnvironment>>) {
                return M::template vacuousImporter<T>();
            } else if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                class LocalI : public M::template AbstractImporter<T>, public M::template IHandler<T> {
                private:
                    std::list<typename M::template AbstractImporter<T> *> underlyingImporters_;
                public:
                    LocalI(std::list<typename M::template AbstractImporter<T> *> const &u) : 
#ifndef _MSC_VER
                        M::template AbstractImporter<T>(),
                        M::template IHandler<T>(),
#endif
                        underlyingImporters_(u) 
                    {
                    }
                    virtual void start(TheEnvironment *env) override final {
                        for (auto *importer : underlyingImporters_) {
                            importer->addHandler(this);
                            importer->start(env);
                        }
                    }
                    virtual void handle(typename M::template InnerData<T> &&data) override final {
                        this->publish(std::move(data));
                    }
                };
                return M::template importer<T>(new LocalI(underlyingImporters));
            } else if constexpr (std::is_same_v<M, infra::SinglePassIterationApp<TheEnvironment>>) {
                class LocalI : public M::template AbstractImporter<T> {
                private:
                    std::list<typename M::template AbstractImporter<T> *> underlyingImporters_;
                    using QueueItem = std::tuple<typename M::template AbstractImporter<T> *, typename M::template Data<T>>;
                    class ActualComparer {
                    private:
                        std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> comparer_;
                    public:
                        ActualComparer() : comparer_() {}
                        ActualComparer(std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> const &comparer) : comparer_(comparer) {}
                        bool operator()(QueueItem &a, QueueItem const &b) const {
                            if (!std::get<1>(a)) {
                                return true;
                            }
                            if (!std::get<1>(b)) {
                                return false;
                            }
                            if constexpr (std::is_same_v<Comparer, void>) {
                                return (std::get<1>(a)->timedData.timePoint > std::get<1>(b)->timedData.timePoint);
                            } else {
                                auto tp1 = std::get<1>(a)->timedData.timePoint;
                                auto tp2 = std::get<1>(b)->timedData.timePoint;
                                if (tp1 > tp2) {
                                    return true;
                                }
                                if (tp1 < tp2) {
                                    return false;
                                }
                                return comparer_(std::get<1>(a)->timedData.value, std::get<1>(b)->timedData.value);
                            }
                        }
                    };
                    std::priority_queue<QueueItem, std::vector<QueueItem>, ActualComparer> queue_;
                public:
                    LocalI(std::list<typename M::template AbstractImporter<T> *> const &u, std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> const &comparer) : 
#ifndef _MSC_VER
                        M::template AbstractImporter<T>(),
#endif
                        underlyingImporters_(u) 
                        , queue_(ActualComparer(comparer))
                    {
                    }
                    virtual void start(TheEnvironment *env) override final {
                        for (auto *importer : underlyingImporters_) {
                            importer->start(env);
                            queue_.push({importer, importer->generate((T const *) nullptr)});
                        }
                    }
                    virtual typename M::template Data<T> generate(T const *notUsed=nullptr) override final {
                        if (underlyingImporters_.empty()) {
                            return std::nullopt;
                        }
                        QueueItem ret = std::move(queue_.top());
                        queue_.pop();
                        auto *importer = std::get<0>(ret);
                        bool maybeFinal = (queue_.empty() || !std::get<1>(queue_.top()));
                        if (!(std::get<1>(ret)) || !(std::get<1>(ret)->timedData.finalFlag)) {
                            queue_.push({importer, importer->generate((T const *) nullptr)});
                        }
                        if (!maybeFinal) {
                            std::get<1>(ret)->timedData.finalFlag = false;
                        }
                        return std::move(std::get<1>(ret));
                    }
                };
                return M::template importer<T>(new LocalI(underlyingImporters, comparer));
            } else if constexpr (std::is_same_v<M, infra::TopDownSinglePassIterationApp<TheEnvironment>>) {
                class LocalI : public M::template AbstractImporter<T> {
                private:
                    std::list<typename M::template AbstractImporter<T> *> underlyingImporters_;
                    using QueueItem = std::tuple<typename M::template AbstractImporter<T> *, typename M::template Data<T>>;
                    class ActualComparer {
                    private:
                        std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> comparer_;
                    public:
                        ActualComparer() : comparer_() {}
                        ActualComparer(std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> const &comparer) : comparer_(comparer) {}
                        bool operator()(QueueItem &a, QueueItem const &b) const {
                            if (!std::get<1>(a)) {
                                return true;
                            }
                            if (!std::get<1>(b)) {
                                return false;
                            }
                            if constexpr (std::is_same_v<Comparer, void>) {
                                return (std::get<1>(a)->timedData.timePoint > std::get<1>(b)->timedData.timePoint);
                            } else {
                                auto tp1 = std::get<1>(a)->timedData.timePoint;
                                auto tp2 = std::get<1>(b)->timedData.timePoint;
                                if (tp1 > tp2) {
                                    return true;
                                }
                                if (tp1 < tp2) {
                                    return false;
                                }
                                return comparer_(std::get<1>(a)->timedData.value, std::get<1>(b)->timedData.value);
                            }
                        }
                    };
                    std::priority_queue<QueueItem, std::vector<QueueItem>, ActualComparer> queue_;
                public:
                    LocalI(std::list<typename M::template AbstractImporter<T> *> const &u, std::conditional_t<std::is_same_v<Comparer, void>, bool, Comparer> const &comparer) : 
#ifndef _MSC_VER
                        M::template AbstractImporter<T>(),
#endif
                        underlyingImporters_(u) 
                        , queue_(ActualComparer(comparer))
                    {
                    }
                    virtual void start(TheEnvironment *env) override final {
                        for (auto *importer : underlyingImporters_) {
                            importer->start(env);
                            queue_.push({importer, importer->generate((T const *) nullptr)});
                        }
                    }
                    virtual std::tuple<bool, typename M::template Data<T>> generate(T const *notUsed=nullptr) override final {
                        if (underlyingImporters_.empty()) {
                            return {false, std::nullopt};
                        }
                        QueueItem ret = std::move(queue_.top());
                        queue_.pop();
                        auto *importer = std::get<0>(ret);
                        bool maybeFinal = (queue_.empty() || !std::get<1>(queue_.top()));
                        if (!(std::get<1>(ret)) || !(std::get<1>(ret)->timedData.finalFlag)) {
                            queue_.push({importer, importer->generate((T const *) nullptr)});
                        }
                        if (!maybeFinal) {
                            if (std::get<1>(ret)) {
                                std::get<1>(ret)->timedData.finalFlag = false;
                            }
                        }
                        if (std::get<1>(ret)) {
                            return {true, std::nullopt};
                        } else {
                            return {!(std::get<1>(ret)->timedData.finalFlag), std::move(std::get<1>(ret))};
                        }
                    }
                };
                return M::template importer<T>(new LocalI(underlyingImporters, comparer));
            } else {
                return std::shared_ptr<typename M::template Importer<T>>();
            }
        }

    public:
        template <class T>
        static auto mergedImporter(std::list<std::shared_ptr<typename M::template Importer<T>>> const &underlyingImporters)
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            std::list<typename M::template AbstractImporter<T> *> l;
            for (auto const &x : underlyingImporters) {
                auto p = x->getUnderlyingPointers();
                if (!p.empty()) {
                    l.push_back((typename M::template AbstractImporter<T> *) *(p.begin()));
                }
            }
            return mergedImporter_<T, void>(l);
        }
        template <class T, class Comparer>
        static auto mergedImporterWithComparer(std::list<std::shared_ptr<typename M::template Importer<T>>> const &underlyingImporters, Comparer const &comparer=Comparer())
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            std::list<typename M::template AbstractImporter<T> *> l;
            for (auto const &x : underlyingImporters) {
                auto p = x->getUnderlyingPointers();
                if (!p.empty()) {
                    l.push_back((typename M::template AbstractImporter<T> *) *(p.begin()));
                }
            }
            return mergedImporter_<T, Comparer>(l, comparer);
        }
    
        template <class T>
        static auto shiftedImporter(
            std::shared_ptr<typename M::template Importer<T>> const &underlyingImporter
            , typename M::Duration const &shiftBy
        )
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            if constexpr (std::is_same_v<M, infra::BasicWithTimeApp<TheEnvironment>>) {
                return M::template vacuousImporter<T>();
            } else if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                throw std::runtime_error("shiftedImporter is not supported in RealTimeApp");
            } else if constexpr (std::is_same_v<M, infra::SinglePassIterationApp<TheEnvironment>>) {
                class LocalI : public M::template AbstractImporter<T> {
                private:
                    std::shared_ptr<typename M::template Importer<T>> underlyingImporter_;
                    typename M::Duration shiftBy_;
                public:
                    LocalI(std::shared_ptr<typename M::template Importer<T>> const &underlyingImporter, typename M::Duration const &shiftBy) : 
#ifndef _MSC_VER
                        M::template AbstractImporter<T>(),
#endif
                        underlyingImporter_(underlyingImporter) 
                        , shiftBy_(shiftBy)
                    {
                    }
                    virtual void start(TheEnvironment *env) override final {
                        underlyingImporter_->start(env);
                    }
                    virtual typename M::template Data<T> generate(T const *notUsed=nullptr) override final {
                        auto item = underlyingImporter_->generate((T const *) nullptr);
                        if (item) {
                            item->timedData.timePoint += shiftBy_;
                        }
                        return item;
                    }
                };
                return M::template importer<T>(new LocalI(underlyingImporter, shiftBy));
            } else if constexpr (std::is_same_v<M, infra::TopDownSinglePassIterationApp<TheEnvironment>>) {
                class LocalI : public M::template AbstractImporter<T> {
                private:
                    std::shared_ptr<typename M::template Importer<T>> underlyingImporter_;
                    typename M::Duration shiftBy_;
                public:
                    LocalI(std::shared_ptr<typename M::template Importer<T>> const &underlyingImporter, typename M::Duration const &shiftBy) : 
#ifndef _MSC_VER
                        M::template AbstractImporter<T>(),
#endif
                        underlyingImporter_(underlyingImporter) 
                        , shiftBy_(shiftBy)
                    {
                    }
                    virtual void start(TheEnvironment *env) override final {
                        underlyingImporter_->start(env);
                    }
                    virtual std::tuple<bool, typename M::template Data<T>> generate(T const *notUsed=nullptr) override final {
                        auto item = underlyingImporter_->generate((T const *) nullptr);
                        if (item) {
                            item->timedData.timePoint += shiftBy_;
                            return {!(item->timedData.finalFlag), std::move(item)};
                        } else {
                            return {true, std::move(item)};
                        }
                    }
                };
                return M::template importer<T>(new LocalI(underlyingImporter, shiftBy));
            } else {
                return std::shared_ptr<typename M::template Importer<T>>();
            }
        }

        template <class... T>
        static auto collect() {
            return infra::KleisliUtils<M>::template liftPure<std::variant<T...>>(
                [](std::variant<T...> &&x) -> SingleLayerWrapper<std::variant<T...>>
                {
                    return {std::move(x)};
                }
            );
        }
        template <class... T>
        static auto dispatch() {
            return infra::KleisliUtils<M>::template liftPure<SingleLayerWrapper<std::variant<T...>>>(
                [](SingleLayerWrapper<std::variant<T...>> &&x) -> std::variant<T...>
                {
                    return std::move(x.value);
                }
            );
        }

        template <int32_t ID, class T>
        static auto tagWithID() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> SingleLayerWrapperWithID<ID, T>
                {
                    return {std::move(x)};
                }
            );
        }
        template <class Mark, class T>
        static auto tagWithTypeMark() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> SingleLayerWrapperWithTypeMark<Mark, T>
                {
                    return {std::move(x)};
                }
            );
        }

        template <class Input, class Output, class State, class StateUpdater, class OutputProducer, class StateResetter>
        static auto actionWithBufferedState(StateUpdater &&stateUpdater = StateUpdater{}, OutputProducer &&outputProducer = OutputProducer{}, StateResetter &&stateResetter = StateResetter{}, State &&state = State {})
            -> std::shared_ptr<typename M::template Action<Input, Output>> 
        {
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                class LocalAction 
                    : public infra::IRealTimeAppPossiblyThreadedNode, public M::template SingleEntryAbstractAction<Input,Output> 
                {
                private:
                    StateUpdater stateUpdater_;
                    OutputProducer outputProducer_;
                    StateResetter stateResetter_;
                    State state_;
                    typename infra::RealTimeAppComponents<TheEnvironment>::template TimeChecker<true, Input> timeChecker_;
                    TheEnvironment *env_;
                    std::thread thread_;
                    std::mutex mutex_;
                    std::condition_variable cond_;
                    std::atomic<bool> running_;

                    void run() {
                        while (running_) {
                            std::unique_lock<std::mutex> lock(mutex_);
                            cond_.wait_for(lock, std::chrono::milliseconds(100));
                            if (!running_) {
                                lock.unlock();
                                break;
                            }
                            
                            if constexpr (std::is_invocable_v<
                                OutputProducer &
                                , State &
                                , bool *
                            >) {
                                bool needReset = true;
                                auto ret = outputProducer_(state_, &needReset);
                                if (needReset) {
                                    stateResetter_(state_);
                                }
                                lock.unlock();
                                if (!running_) {
                                    break;
                                }
                                using T = std::decay_t<decltype(ret)>;
                                if constexpr (std::is_same_v<T, Output>) {
                                    this->publish(
                                        typename M::template InnerData<Output> {
                                            env_
                                            , {
                                                env_->resolveTime()
                                                , std::move(ret)
                                                , false
                                            }
                                        }
                                    );
                                } else if constexpr (std::is_same_v<T, std::optional<Output>>) {
                                    if (ret) {
                                        this->publish(
                                            typename M::template InnerData<Output> {
                                                env_
                                                , {
                                                    env_->resolveTime()
                                                    , std::move(*ret)
                                                    , false
                                                }
                                            }
                                        );
                                    }
                                } else if constexpr (std::is_same_v<T, std::vector<Output>>) {
                                    for (auto &item : ret) {
                                        this->publish(
                                            typename M::template InnerData<Output> {
                                                env_
                                                , {
                                                    env_->resolveTime()
                                                    , std::move(item)
                                                    , false
                                                }
                                            }
                                        );
                                    }
                                }
                            } else {
                                auto ret = outputProducer_(state_);
                                stateResetter_(state_);
                                lock.unlock();
                                if (!running_) {
                                    break;
                                }
                                using T = std::decay_t<decltype(ret)>;
                                if constexpr (std::is_same_v<T, Output>) {
                                    this->publish(
                                        typename M::template InnerData<Output> {
                                            env_
                                            , {
                                                env_->resolveTime()
                                                , std::move(ret)
                                                , false
                                            }
                                        }
                                    );
                                } else if constexpr (std::is_same_v<T, std::optional<Output>>) {
                                    if (ret) {
                                        this->publish(
                                            typename M::template InnerData<Output> {
                                                env_
                                                , {
                                                    env_->resolveTime()
                                                    , std::move(*ret)
                                                    , false
                                                }
                                            }
                                        );
                                    }
                                } else if constexpr (std::is_same_v<T, std::vector<Output>>) {
                                    for (auto &item : ret) {
                                        this->publish(
                                            typename M::template InnerData<Output> {
                                                env_
                                                , {
                                                    env_->resolveTime()
                                                    , std::move(item)
                                                    , false
                                                }
                                            }
                                        );
                                    }
                                }
                            }
                        }
                    }
                public:
                    LocalAction(
                        StateUpdater &&stateUpdater
                        , OutputProducer &&outputProducer
                        , StateResetter &&stateResetter
                        , State &&state
                    ) : M::template SingleEntryAbstractAction<Input,Output>()
                        , stateUpdater_(std::move(stateUpdater))
                        , outputProducer_(std::move(outputProducer))
                        , stateResetter_(std::move(stateResetter))
                        , state_(std::move(state))
                        , timeChecker_()
                        , env_(nullptr)
                        , thread_()
                        , mutex_()
                        , cond_()
                        , running_(true)
                    {
                        thread_ = std::thread(&LocalAction::run, this);
                        thread_.detach();
                    }
                    ~LocalAction() {
                        running_ = false;
                        cond_.notify_one();
                        try {
                            if (thread_.joinable()) {
                                thread_.join();
                            }
                        } catch (std::exception const &) {
                        }
                    }
                    virtual void handle(typename M::template InnerData<Input> &&data) override final {
                        if (running_ && timeChecker_(data)) {
                            std::lock_guard<std::mutex> _(mutex_);
                            env_ = data.environment;
                            stateUpdater_(std::move(data.timedData.value), state_);
                            cond_.notify_one();
                        }
                    }
                    virtual std::optional<std::thread::native_handle_type> threadHandle() override final {
                        return thread_.native_handle();
                    }
                    virtual bool isThreaded() const override final {
                        return true;
                    }
                    virtual bool isOneTimeOnly() const override final {
                        return false;
                    }
                    virtual void setIdleWorker(std::function<void(void *)> worker) override final {
                    }
                    virtual void setStartWaiter(std::function<void()> waiter) override final {
                    }
                };
                return M::template fromAbstractAction<Input,Output>(new LocalAction(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
            } else {
                if constexpr (std::is_invocable_v<
                                OutputProducer &
                                , State &
                                , bool *
                            >) {
                    using T = decltype(
                        (* ((OutputProducer *) nullptr))((* ((State *) nullptr)), (bool *) nullptr)
                    );
                    class LocalF {
                    private:
                        StateUpdater stateUpdater_;
                        OutputProducer outputProducer_;
                        StateResetter stateResetter_;
                        State state_;
                    public:
                        LocalF(StateUpdater &&stateUpdater, OutputProducer &&outputProducer, StateResetter &&stateResetter, State &&state)
                            : stateUpdater_(std::move(stateUpdater)), outputProducer_(std::move(outputProducer)), stateResetter_(std::move(stateResetter)), state_(std::move(state))
                        {}
                        T operator()(Input &&data) {
                            stateUpdater_(std::move(data), state_);
                            bool needReset = true;
                            auto ret = outputProducer_(state_, &needReset);
                            if (needReset) {
                                stateResetter_(state_);
                            }
                            return ret;
                        }
                    };
                    if constexpr (std::is_same_v<std::decay_t<T>, Output>) {
                        return M::template liftPure<Input>(LocalF(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
                    } else if constexpr (std::is_same_v<std::decay_t<T>, std::optional<Output>>) {
                        return M::template liftMaybe<Input>(LocalF(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
                    } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<Output>>) {
                        return M::template liftMulti<Input>(LocalF(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
                    } else {
                        throw std::runtime_error("Unknown return type of OutputProducer in actionWithBufferedState");
                    }
                } else {
                    using T = decltype(
                        (* ((OutputProducer *) nullptr))((* ((State *) nullptr)))
                    );
                    class LocalF {
                    private:
                        StateUpdater stateUpdater_;
                        OutputProducer outputProducer_;
                        StateResetter stateResetter_;
                        State state_;
                    public:
                        LocalF(StateUpdater &&stateUpdater, OutputProducer &&outputProducer, StateResetter &&stateResetter, State &&state)
                            : stateUpdater_(std::move(stateUpdater)), outputProducer_(std::move(outputProducer)), stateResetter_(std::move(stateResetter)), state_(std::move(state))
                        {}
                        T operator()(Input &&data) {
                            stateUpdater_(std::move(data), state_);
                            auto ret = outputProducer_(state_);
                            stateResetter_(state_);
                            return ret;
                        }
                    };
                    if constexpr (std::is_same_v<std::decay_t<T>, Output>) {
                        return M::template liftPure<Input>(LocalF(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
                    } else if constexpr (std::is_same_v<std::decay_t<T>, std::optional<Output>>) {
                        return M::template liftMaybe<Input>(LocalF(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
                    } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<Output>>) {
                        return M::template liftMulti<Input>(LocalF(std::move(stateUpdater), std::move(outputProducer), std::move(stateResetter), std::move(state)));
                    } else {
                        throw std::runtime_error("Unknown return type of OutputProducer in actionWithBufferedState");
                    }
                }
            }
        }

        template <class A>
        static std::shared_ptr<typename M::template OnOrderFacility<A,VoidStruct>> facilityBridge() {
            class LocalF : public M::template AbstractOnOrderFacility<A,VoidStruct> {
            public:
                virtual void handle(typename M::template InnerData<typename M::template Key<A>> &&x) override final {
                    this->publish(x.environment, typename M::template Key<VoidStruct> {x.timedData.value.id(), VoidStruct {}}, true);
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF());
        }

        //to use this, taskBody has to be truly parallel and no thread protection should be needed in taskBody
        template <class A, class B, class F>
        static auto simpleTaskSpawningFacility(
            F &&taskBody
        ) -> std::shared_ptr<typename M::template OnOrderFacility<A,B>>
        {
            if constexpr (infra::app_classification_v<M> == infra::AppClassification::RealTime) {
                auto liftedK = infra::GenericLift<M>::liftKU(std::move(taskBody));
                using LF = std::decay_t<decltype(liftedK)>;

                class Facility : public M::template AbstractOnOrderFacility<A,B> {
                private:
                    LF liftedK_;
                public:
                    Facility(LF &&liftedK) : liftedK_(std::move(liftedK)) {}
                    virtual void handle(typename M::template InnerData<typename M::template Key<A>> &&data) {
                        std::thread th([this,data=std::move(data)]() mutable {
                            auto id = data.timedData.value.id();
                            auto res = liftedK_(std::move(data).mapMove([](typename M::template Key<A> &&k) -> A {
                                return k.key();
                            }, true));
                            if (res) {
                                this->publish(std::move(*res).mapMove([id=std::move(id)](B &&b) -> typename M::template Key<B> {
                                    return {id, std::move(b)};
                                }, true));
                            }
                        });
                        th.detach();
                    }
                };
                return M::fromAbstractOnOrderFacility(new Facility(std::move(liftedK)));
            } else {
                return infra::GenericLift<M>::liftFacility(std::move(taskBody));
            }
        }

    private:
        template <
            class InputData
            , class KeyExtractorF
            , class ValueExtractorF
            , class TriggerData
            , class KeyHash
            , bool NeedMutex
        >
        class KeyedUpdateGeneratorImpl {
        private:
            KeyExtractorF keyExtractorF_;
            ValueExtractorF valueExtractorF_;
            typename M::Duration resendDuration_;

            using Key = decltype(keyExtractorF_(*((InputData const *) nullptr)));
            using Value = decltype(valueExtractorF_(std::move(*((InputData *) nullptr))));

            using ValueList = std::list<std::tuple<typename M::TimePoint, std::unique_ptr<Key>, std::unique_ptr<Value>>>;
            ValueList valueList_;
            std::unordered_map<Key, typename ValueList::iterator, KeyHash> data_;
            infra::AppSpecificMutex<M> mutex_;

            void addResends_locked(
                typename M::TimePoint t
                , typename M::template InnerData<std::vector<std::tuple<Key,Value>>> &ret
            ) {
                while (!valueList_.empty()) {
                    auto &v = valueList_.front();
                    if (std::get<0>(v)+resendDuration_ >= t) {
                        break;
                    }
                    ret.timedData.value.push_back({
                        infra::withtime_utils::makeValueCopy(*std::get<1>(v))
                        , infra::withtime_utils::makeValueCopy(*std::get<2>(v))
                    });
                    std::get<0>(v) = t;
                    auto x = std::move(v);
                    valueList_.pop_front();
                    valueList_.emplace_back(std::move(x));
                    auto iter = valueList_.end();
                    --iter;
                    data_[*(std::get<1>(*iter))] = iter;
                }
            }

            typename M::template Data<std::vector<std::tuple<Key,Value>>> handleTrigger(
                TheEnvironment *env
                , typename M::TimePoint const &t
                , bool finalFlag
            ) {
                typename M::template InnerData<std::vector<std::tuple<Key,Value>>> ret {
                    env 
                    , {
                        {}
                        , {}
                        , finalFlag
                    }
                };

                if constexpr (NeedMutex) {
                    infra::AppSpecificLockGuard<M> _(mutex_);
                    addResends_locked(t, ret);
                } else {
                    addResends_locked(t, ret);
                }

                ret.timedData.timePoint = env->resolveTime(t);

                return {std::move(ret)};
            }
            typename M::template Data<std::vector<std::tuple<Key,Value>>> handleInput(
                TheEnvironment *env
                , typename M::TimePoint const &t
                , bool finalFlag
                , InputData &&input
            ) {
                typename M::template InnerData<std::vector<std::tuple<Key,Value>>> ret {
                    env 
                    , {
                        {}
                        , {}
                        , finalFlag
                    }
                };

                auto key = keyExtractorF_(input);

                if constexpr (NeedMutex) {
                    infra::AppSpecificLockGuard<M> _(mutex_);
                    auto iter = data_.find(key);
                    typename ValueList::iterator listIter;
                    if (iter == data_.end()) {
                        valueList_.emplace_back(
                            t
                            , std::make_unique<Key>(key)
                            , std::make_unique<Value>(valueExtractorF_(std::move(input)))
                        );
                        listIter = valueList_.end();
                        --listIter;
                        data_.insert({key, listIter});
                    } else {
                        listIter = iter->second;
                        std::get<0>(*listIter) = t;
                        *(std::get<2>(*listIter)) = valueExtractorF_(std::move(input));
                        auto x = std::move(*listIter);
                        valueList_.erase(listIter);
                        valueList_.emplace_back(std::move(x));
                        listIter = valueList_.end();
                        --listIter;
                        iter->second = listIter;
                    }
                    ret.timedData.value.push_back({infra::withtime_utils::makeValueCopy(key), infra::withtime_utils::makeValueCopy(*(std::get<2>(*listIter)))});
                    addResends_locked(t, ret);
                } else {
                    auto iter = data_.find(key);
                    typename ValueList::iterator listIter;
                    if (iter == data_.end()) {
                        valueList_.emplace_back(
                            t
                            , std::make_unique<Key>(key)
                            , std::make_unique<Value>(valueExtractorF_(std::move(input)))
                        );
                        listIter = valueList_.end();
                        --listIter;
                        data_.insert({key, listIter});
                    } else {
                        listIter = iter->second;
                        std::get<0>(*listIter) = t;
                        *(std::get<2>(*listIter)) = valueExtractorF_(std::move(input));
                        auto x = std::move(*listIter);
                        valueList_.erase(listIter);
                        valueList_.emplace_back(std::move(x));
                        listIter = valueList_.end();
                        --listIter;
                        iter->second = listIter;
                    }
                    ret.timedData.value.push_back({infra::withtime_utils::makeValueCopy(key), infra::withtime_utils::makeValueCopy(*(std::get<2>(*listIter)))});
                    addResends_locked(t, ret);
                }
                ret.timedData.timePoint = env->resolveTime(t);

                return {std::move(ret)};
            }
        public:
            KeyedUpdateGeneratorImpl(KeyExtractorF &&keyExtractorF, ValueExtractorF &&valueExtractorF, typename M::Duration const &resendDuration)
                : keyExtractorF_(std::move(keyExtractorF))
                , valueExtractorF_(std::move(valueExtractorF))
                , resendDuration_(resendDuration)
                , valueList_()
                , data_()
                , mutex_()
            {}
            KeyedUpdateGeneratorImpl(KeyedUpdateGeneratorImpl &&x) 
                : keyExtractorF_(std::move(x.keyExtractorF_))
                , valueExtractorF_(std::move(x.valueExtractorF_))
                , resendDuration_(x.resendDuration_)
                , valueList_()
                , data_()
                , mutex_()
            {}
            ~KeyedUpdateGeneratorImpl() = default;

            typename M::template Data<std::vector<std::tuple<Key,Value>>> operator()(
                typename M::template InnerData<std::variant<InputData, TriggerData>> &&data
            ) {
                auto *env = data.environment;
                auto t = data.timedData.timePoint;
                auto finalFlag = data.timedData.finalFlag;
                
                return std::visit([this,env,t,finalFlag](auto &&x) -> typename M::template Data<std::vector<std::tuple<Key,Value>>> {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, InputData>) {
                        return handleInput(env, t, finalFlag, std::move(x));
                    } else {
                        return handleTrigger(env, t, finalFlag);
                    }
                }, std::move(data.timedData.value));
            }

            static std::shared_ptr<typename M::template Action<
                std::variant<InputData,TriggerData>
                , std::vector<std::tuple<Key,Value>>
            >> createAction(
                KeyExtractorF &&keyExtractorF
                , ValueExtractorF &&valueExtractorF
                , typename M::Duration const &resendDuration
                , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint> {}
            ) {
                return M::template kleisli2<InputData,TriggerData>(
                    KeyedUpdateGeneratorImpl(std::move(keyExtractorF), std::move(valueExtractorF), resendDuration)
                    , liftParam
                );
            }
        };
    public:
        template <
            class InputData
            , class TriggerData = basic::VoidStruct
            , template <class X> class KeyHashTemplate = std::hash
        >
        class KeyedUpdateGenerator {
        public:
            template <
                class KeyExtractorF
                , class ValueExtractorF
            >
            static auto keyedUpdateGenerator(
                KeyExtractorF &&keyExtractorF
                , ValueExtractorF &&valueExtractorF
                , typename M::Duration const &resendDuration
                , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint> {}
            ) {
                using KeyHash = KeyHashTemplate<decltype(keyExtractorF(std::move(*((InputData *) nullptr))))>;
                if (liftParam.suggestThreaded) {
                    return KeyedUpdateGeneratorImpl<InputData,KeyExtractorF,ValueExtractorF,TriggerData,KeyHash,false>
                    ::createAction(std::move(keyExtractorF), std::move(valueExtractorF), resendDuration, liftParam);
                } else {
                    return KeyedUpdateGeneratorImpl<InputData,KeyExtractorF,ValueExtractorF,TriggerData,KeyHash,true>
                    ::createAction(std::move(keyExtractorF), std::move(valueExtractorF), resendDuration, liftParam);
                }
            }
        };
    private:
        template <
            class InputData
            , class KeyExtractorF
            , class ValueExtractorF
            , class KeyHash
            , class ValueEqCheck
            , bool NeedMutex
        >
        class RemoveDuplicatesImpl {
        private:
            KeyExtractorF keyExtractorF_;
            ValueExtractorF valueExtractorF_;

            using Key = decltype(keyExtractorF_(*((InputData const *) nullptr)));
            using Value = decltype(valueExtractorF_(std::move(*((InputData *) nullptr))));

            std::unordered_map<Key, Value, KeyHash> data_;
            infra::AppSpecificMutex<M> mutex_;
        public:
            RemoveDuplicatesImpl(KeyExtractorF &&keyExtractorF, ValueExtractorF &&valueExtractorF)
                : keyExtractorF_(std::move(keyExtractorF)), valueExtractorF_(std::move(valueExtractorF))
                , data_(), mutex_()
            {}
            RemoveDuplicatesImpl(RemoveDuplicatesImpl &&r)
                : keyExtractorF_(std::move(r.keyExtractorF_)), valueExtractorF_(std::move(r.valueExtractorF_))
                , data_(), mutex_()
            {}
            ~RemoveDuplicatesImpl() = default;
            std::optional<std::tuple<Key, Value>> operator()(InputData &&x) {
                auto k = keyExtractorF_(x);
                auto v = valueExtractorF_(std::move(x));
                if constexpr (NeedMutex) {
                    infra::AppSpecificLockGuard<M> _(mutex_);
                    auto iter = data_.find(k);
                    if (iter == data_.end()) {
                        data_.insert({infra::withtime_utils::makeValueCopy(k), infra::withtime_utils::makeValueCopy(v)});
                        return std::tuple<Key, Value> {std::move(k), std::move(v)};
                    } else {
                        if (!ValueEqCheck()(iter->second, v)) {
                            iter->second = infra::withtime_utils::makeValueCopy(v);
                            return std::tuple<Key, Value> {std::move(k), std::move(v)};
                        } else {
                            return std::nullopt;
                        }
                    }
                } else {
                    auto iter = data_.find(k);
                    if (iter == data_.end()) {
                        data_.insert({infra::withtime_utils::makeValueCopy(k), infra::withtime_utils::makeValueCopy(v)});
                        return std::tuple<Key, Value> {std::move(k), std::move(v)};
                    } else {
                        if (!ValueEqCheck()(iter->second, v)) {
                            iter->second = infra::withtime_utils::makeValueCopy(v);
                            return std::tuple<Key, Value> {std::move(k), std::move(v)};
                        } else {
                            return std::nullopt;
                        }
                    }
                }
            }
            static std::shared_ptr<typename M::template Action<
                InputData
                , std::tuple<Key,Value>
            >> createAction(
                KeyExtractorF &&keyExtractorF
                , ValueExtractorF &&valueExtractorF
                , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint> {}
            ) {
                return M::template liftMaybe<InputData>(
                    RemoveDuplicatesImpl(std::move(keyExtractorF), std::move(valueExtractorF))
                    , liftParam
                );
            }
        };
    public:
        template <
            class InputData
            , template <class X> class KeyHashTemplate = std::hash
            , template <class X> class EqCheckTemplate = std::equal_to
        >
        class RemoveDuplicates {
        public:
            template <
                class KeyExtractorF
                , class ValueExtractorF
            >
            static auto removeDuplicates(
                KeyExtractorF &&keyExtractorF
                , ValueExtractorF &&valueExtractorF
                , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint> {}
            ) {
                using KeyHash = KeyHashTemplate<decltype(keyExtractorF(std::move(*((InputData *) nullptr))))>;
                using EqCheck = EqCheckTemplate<decltype(valueExtractorF(std::move(*((InputData *) nullptr))))>;
                if (liftParam.suggestThreaded) {
                    return RemoveDuplicatesImpl<InputData,KeyExtractorF,ValueExtractorF,KeyHash,EqCheck,false>
                    ::createAction(std::move(keyExtractorF), std::move(valueExtractorF), liftParam);
                } else {
                    return RemoveDuplicatesImpl<InputData,KeyExtractorF,ValueExtractorF,KeyHash,EqCheck,true>
                    ::createAction(std::move(keyExtractorF), std::move(valueExtractorF), liftParam);
                }
            }
        };
    };

} } } }

#endif