#ifndef TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_
#define TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_

#include <memory>
#include <optional>
#include <deque>

#include <tm_kit/infra/KleisliUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class M>
    class CommonFlowUtilComponents {
    public:
        using TheEnvironment = typename M::EnvironmentType;

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
    public:
        template <class A, class F>
        static PureFilter<A,F> pureFilter(F &&f) {
            return PureFilter<A,F>(std::move(f));
        }
        template <class A, class F>
        static EnhancedPureFilter<A,F> enhancedPureFilter(F &&f) {
            return enhancedPureFilter<A,F>(std::move(f));
        }
        
        template <class T>
        static std::shared_ptr<typename M::template Action<T,T>> simpleFinalizer() {
            return M::template kleisli<T>([](typename M::template InnerData<T> &&d) -> typename M::template Data<T> {
                d.timedData.finalFlag = true;
                return {std::move(d)};
            });
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
    };

} } } }

#endif