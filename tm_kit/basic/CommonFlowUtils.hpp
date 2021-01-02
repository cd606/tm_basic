#ifndef TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_
#define TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_

#include <memory>
#include <optional>
#include <deque>

#include <tm_kit/infra/KleisliUtils.hpp>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/basic/VoidStruct.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class M>
    class CommonFlowUtilComponents {
    public:
        using TheEnvironment = typename M::EnvironmentType;

        template <class T>
        static typename infra::KleisliUtils<M>::template KleisliFunction<T,T> idFunc() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> T
                {
                    return std::move(x);
                }
            );
        }

        template <class T>
        static typename infra::KleisliUtils<M>::template KleisliFunction<std::optional<T>,T> filterOnOptional() {
            return infra::KleisliUtils<M>::template liftMaybe<std::optional<T>>(
                [](std::optional<T> &&x) -> std::optional<T>
                {
                    return std::move(x);
                }
            );
        }

        template <class T>
        static typename infra::KleisliUtils<M>::template KleisliFunction<std::vector<T>,T> dispatchOneByOne() {
            return infra::KleisliUtils<M>::template liftMulti<std::vector<T>>(
                [](std::vector<T> &&x) -> std::vector<T>
                {
                    return std::move(x);
                }
            );
        }

        //This is the "reverse" of dispatchOneByOne
        //The RealTimeApp version is very simple because this action does not make too much sense there
        template <class T>
        static auto bunch() -> typename std::shared_ptr<typename M::template Action<T, std::vector<T>>> {
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                return M::template liftPure<T>(
                    [](T &&t) -> std::vector<T> {
                        return {std::move(t)};
                    }
                );
            } else {
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
                auto cont = [](typename M::template InnerData<T> &&input, State &state, std::function<void(typename M::template InnerData<std::vector<T>> &&)> handler) {                
                    if (state.repeatCallForThisInput) {
                        if (state.currentBunchTime && state.endReached) {
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
                            state.currentBunchTime = std::nullopt;
                            state.currentBunch.clear();
                        } else {
                            //do nothing
                        }
                        state.repeatCallForThisInput = false;
                    } else {                    
                        if (!state.currentBunchTime) {
                            if (input.timedData.finalFlag) {
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
                                state.repeatCallForThisInput = true;
                            } else {
                                state.currentBunchTime = input.timedData.timePoint;
                                state.currentBunch.push_back(std::move(input.timedData.value));
                            }
                        } else {
                            if (*(state.currentBunchTime) >= input.timedData.timePoint) {
                                state.currentBunch.push_back(std::move(input.timedData.value));
                                if (input.timedData.finalFlag) {
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
                                    state.repeatCallForThisInput = true;
                                    state.currentBunchTime = std::nullopt;
                                    state.currentBunch.clear();
                                } else {
                                    //do nothing
                                }
                            } else {
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
                                state.repeatCallForThisInput = true;
                                state.currentBunchTime = input.timedData.timePoint;
                                state.currentBunch.clear();
                                state.currentBunch.push_back(std::move(input.timedData.value));
                                state.endReached = input.timedData.finalFlag;
                            }
                        }
                    }
                };
                return M::template continuationAction<T,std::vector<T>,State>(cont);
            }
        }

        template <class T>
        static typename infra::KleisliUtils<M>::template KleisliFunction<T,infra::Key<T,TheEnvironment>> keyify() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> infra::Key<T,TheEnvironment>
                {
                    return infra::withtime_utils::keyify<T,TheEnvironment>(std::move(x));
                }
            );
        }

        template <class T>
        static typename infra::KleisliUtils<M>::template KleisliFunction<T,std::shared_ptr<T const>> shareBetweenDownstream() {
            return infra::KleisliUtils<M>::template liftPure<T>(
                [](T &&x) -> std::shared_ptr<T const>
                {
                    return std::make_shared<T const>(std::move(x));
                }
            );
        }

        template <class A, class B>
        static typename infra::KleisliUtils<M>::template KleisliFunction<infra::KeyedData<A,B,TheEnvironment>,B> extractDataFromKeyedData() {
            return infra::KleisliUtils<M>::template liftPure<infra::KeyedData<A,B,TheEnvironment>>(
                [](infra::KeyedData<A,B,TheEnvironment> &&x) -> B
                {
                    return std::move(x.data);
                }
            );
        }

        template <class A, class B>
        static typename infra::KleisliUtils<M>::template KleisliFunction<infra::KeyedData<A,B,TheEnvironment>,infra::Key<B,TheEnvironment>> extractIDAndDataFromKeyedData() {
            return infra::KleisliUtils<M>::template liftPure<infra::KeyedData<A,B,TheEnvironment>>(
                [](infra::KeyedData<A,B,TheEnvironment> &&x) -> infra::Key<B,TheEnvironment>
                {
                    return {std::move(x.key.id()), std::move(x.data)};
                }
            );
        }

        template <class A, class B>
        static typename infra::KleisliUtils<M>::template KleisliFunction<infra::KeyedData<A,B,TheEnvironment>,std::tuple<std::string,B>> extractIDStringAndDataFromKeyedData() {
            return infra::KleisliUtils<M>::template liftPure<infra::KeyedData<A,B,TheEnvironment>>(
                [](infra::KeyedData<A,B,TheEnvironment> &&x) -> std::tuple<std::string, B>
                {
                    return {TheEnvironment::id_to_string(x.key.id()), std::move(x.data)};
                }
            );
        }

        template <class T>
        static typename infra::KleisliUtils<M>::template KleisliFunction<T,std::tuple<T,T>> duplicateInput() {
            return
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
                };
        }
        template <class A, class B>
        static typename infra::KleisliUtils<M>::template KleisliFunction<std::tuple<A,B>,std::tuple<B,A>> switchTupleOrder() {
            return infra::KleisliUtils<M>::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> std::tuple<B,A>
                {
                    return {std::move(std::get<1>(x)), std::move(std::get<0>(x))};
                }
            );
        }
        template <class A, class B>
        static typename infra::KleisliUtils<M>::template KleisliFunction<std::tuple<A,B>,B> dropLeft() {
            return infra::KleisliUtils<M>::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> B
                {
                    return std::get<1>(x);
                }
            );
        }
        template <class A, class B>
        static typename infra::KleisliUtils<M>::template KleisliFunction<std::tuple<A,B>,A> dropRight() {
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
        class Parallel {
        private:
            F f_;
            G g_;
        public:
            Parallel(F &&f, G &&g) : f_(std::move(f)), g_(std::move(g)) {}

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
        template <class A, class B, class F, class G>
        static Parallel<A,B,F,G> parallel(F &&f, G &&g) {
            return Parallel<A,B,F,G>(std::move(f), std::move(g));
        }
    private:
        template <class A, class F, class G>
        class FanOut {
        private:
            F f_;
            G g_;
        public:
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
            return enhancedPureFilter<A,F>(std::move(f));
        }
        template <class A, class F>
        static PureTwoWayFilter<A,F> pureTwoWayFilter(F &&f) {
            return PureTwoWayFilter<A,F>(std::move(f));
        }
        template <class A, class F>
        static EnhancedPureTwoWayFilter<A,F> enhancedPureTwoWayFilter(F &&f) {
            return enhancedPureTwoWayFilter<A,F>(std::move(f));
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
            ComplexMovingTimeWindowKleisli(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
            typename M::template Data<Result> operator()(int which, typename M::template InnerData<A> &&a, typename M::template InnerData<B> &&b) {
                switch (which) {
                    case 0:
                        {
                            w_.handleData(&state_, handler_, std::tuple<typename M::TimePoint, A> {std::move(a.timedData.timePoint), std::move(a.timedData.value)});
                            return std::nullopt;
                        }
                        break;
                    case 1:
                        {
                            auto tp = b.timedData.timePoint;
                            w_.handleTimePoint(&state_, handler_, tp);
                            auto res = handler_.readResult(state_, std::tuple<typename M::TimePoint, B> {std::move(b.timedData.timePoint), std::move(b.timedData.value)});
                            if (res) {
                                return { typename M::template InnerData<Result> {
                                    b.environment,
                                    { b.environment->resolveTime(tp), *res, b.timedData.finalFlag }
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
        class WholeHistoryKleisli {
        public:
            using Result = typename decltype(((WholeHistoryHandler *) nullptr)->readResult())::value_type;
        private:
            WholeHistoryHandler handler_;
        public:
            WholeHistoryKleisli(WholeHistoryHandler &&handler) : handler_(std::move(handler)) {}
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
        static auto wholeHistoryKleisli(WholeHistoryHandler &&handler) {
            return WholeHistoryKleisli<T,WholeHistoryHandler>(std::move(handler));
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
        static typename infra::KleisliUtils<M>::template KleisliFunction<VariantType,OneOfVariants> pickOne() {
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
        static typename infra::KleisliUtils<M>::template KleisliFunction<WrappedVariantType,OneOfVariants> pickOneFromWrappedValue() {
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

    public:
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> delayer(
            decltype(typename M::TimePoint {}-typename M::TimePoint {}) duration
        ) {
            if constexpr (std::is_same_v<M, infra::RealTimeApp<TheEnvironment>>) {
                return M::template kleisli<T>(
                    [duration](typename M::template InnerData<T> &&t) -> typename M::template Data<T> {
                        t.environment->sleepFor(duration);
                        return std::move(t);
                    }
                    , infra::LiftParameters<typename M::TimePoint>().SuggestThreaded(true)
                );
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
                return M::template kleisli<T>(
                    idFunc<T>()
                );
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
            } else {
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
            }
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
    };

} } } }

#endif