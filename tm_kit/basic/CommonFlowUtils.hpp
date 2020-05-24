#ifndef TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_
#define TM_KIT_BASIC_COMMON_FLOW_UTILS_HPP_

#include <memory>
#include <optional>
#include <deque>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class M>
    class CommonFlowUtilComponents {
    public:
        using TheEnvironment = typename M::EnvironmentType;

        template <class T>
        static auto duplicateInput(infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) 
            -> std::shared_ptr<
                typename M::template Action<
                    T
                    , std::tuple<T,T>
                >
            >    
        {
            return M::template kleisli<T>(
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
                        , x.timedData.finalFlag
                    };
                }
                , liftParam
            );
        }
        template <class A, class B>
        static auto switchTupleOrder(infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) 
            -> std::shared_ptr<
                typename M::template Action<
                    std::tuple<A,B>
                    , std::tuple<B,A>
                >
            >    
        {
            return M::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> std::tuple<B,A>
                {
                    return {std::move(std::get<1>(x)), std::move(std::get<0>(x))};
                }
                , liftParam
            );
        }
        template <class A, class B>
        static auto dropLeft(infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) 
            -> std::shared_ptr<
                typename M::template Action<
                    std::tuple<A,B>
                    , B
                >
            >    
        {
            return M::template liftPure<std::tuple<A,B>>(
                [](std::tuple<A,B> &&x) -> B
                {
                    return std::get<1>(x);
                }
                , liftParam
            );
        }
        template <class A, class B, class F>
        static auto liftPurePreserveLeft(F &&f, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) 
            -> std::shared_ptr<
                typename M::template Action<
                    std::tuple<A,B>
                    , std::tuple<
                        A 
                        , decltype(f(std::move(* (B *) nullptr)))
                    >
                >
            >    
        {
            return M::template liftPure<std::tuple<A,B>>(
                [f=std::move(f)](std::tuple<A,B> &&x) -> std::tuple<
                            A 
                            , decltype(f(std::move(* (B *) nullptr)))
                        >
                {
                    return {std::move(std::get<0>(x)), f(std::move(std::get<1>(x)))};
                }
                , liftParam
            );
        }
        template <class A, class B, class F>
        static auto liftMaybePreserveLeft(F &&f, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) 
            -> std::shared_ptr<
                typename M::template Action<
                    std::tuple<A,B>
                    , std::tuple<
                        A 
                        , typename decltype(f(std::move(* (B *) nullptr)))::value_type
                    >
                >
            >    
        {
            return M::template liftMaybe<std::tuple<A,B>>(
                [f=std::move(f)](std::tuple<A,B> &&x) -> std::optional<std::tuple<
                            A 
                            , typename decltype(f(std::move(* (B *) nullptr)))::value_type
                        >>
                {
                    auto fRes = f(std::move(std::get<1>(x)));
                    if (fRes) {
                        return {std::move(std::get<0>(x)), std::move(*fRes)};
                    } else {
                        return std::nullopt;
                    }    
                }
                , liftParam
            );
        }
        template <class A, class B, class F>
        static auto enhancedMaybePreserveLeft(F &&f, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) 
            -> std::shared_ptr<
                typename M::template Action<
                    std::tuple<A,B>
                    , std::tuple<
                        A 
                        , typename decltype(f(
                            std::tuple<typename M::TimePoint, B>()
                        ))::value_type
                    >
                >
            >    
        {
            return M::template enhancedMaybe<std::tuple<A,B>>(
                [f=std::move(f)](std::tuple<typename M::TimePoint, std::tuple<A,B>> &&x) -> std::optional<std::tuple<
                            A 
                            , typename decltype(f(
                                std::tuple<typename M::TimePoint, B>()
                            ))::value_type
                        >>
                {
                    auto fRes = f({std::get<0>(x), std::move(std::get<1>(std::get<1>(x)))});
                    if (fRes) {
                        return {std::move(std::get<0>(std::get<1>(x))), std::move(*fRes)};
                    } else {
                        return std::nullopt;
                    }    
                }
                , liftParam
            );
        }
        
        template <class T, class F>
        static std::shared_ptr<typename M::template Action<T,T>> liftPureFilter(F &&f, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) {
            return M::template liftMaybe<T>([f=std::move(f)](T &&a) -> std::optional<T> {
                if (f(a)) {
                    return {std::move(a)};
                } else {
                    return std::nullopt;
                }
            }, liftParam);
        }

        template <class T, class F>
        static std::shared_ptr<typename M::template Action<T,T>> kleisliFilter(F &&f, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()) {
            return M::template kleisli<T>([f=std::move(f)](typename M::template InnerData<T> &&a) -> typename M::template Data<T> {
                if (f(a)) {
                    return {std::move(a)};
                } else {
                    return std::nullopt;
                }
            }, liftParam);
        }

        template <class Reducer, class StateReader>
        static auto liftPureReduce(Reducer &&reducer, StateReader &&stateReader, typename Reducer::StateType const &initState, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>())
            -> std::shared_ptr<typename M::template Action<
                    typename Reducer::InputType
                    , decltype(stateReader(initState))
                >>
        {
            class F {
            private:
                Reducer reducer_;
                StateReader stateReader_;
                typename Reducer::StateType state_;
            public:
                F(Reducer &&r, StateReader &&sr, typename Reducer::StatetType const &s) : reducer_(std::move(r)), stateReader_(std::move(sr)), state_(s) {}
                decltype(stateReader_(state_)) operator()(typename Reducer::InputType &&x) {
                    reducer_(&state_, std::move(x));
                    return stateReader_(state_);
                }
            };
            return M::template liftPure<typename Reducer::InputType>(F(std::move(reducer), std::move(stateReader), initState), liftParam);
        }

        template <class Reducer, class StateReader>
        static auto kleisliReduce(Reducer &&reducer, StateReader &&stateReader, typename Reducer::StateType const &initState, infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>())
            -> std::shared_ptr<typename M::template Action<
                    typename Reducer::InputType
                    , typename decltype(stateReader((TheEnvironment *) nullptr, initState))::value_type::ValueType
                >>
        {
            class F {
            private:
                Reducer reducer_;
                StateReader stateReader_;
                typename Reducer::StateType state_;
            public:
                F(Reducer &&r, StateReader &&sr, typename Reducer::OutputType const &s) : reducer_(std::move(r)), stateReader_(std::move(sr)), state_(s) {}
                decltype(stateReader_((TheEnvironment *) nullptr, state_)) operator()(typename M::template InnerData<Reducer::InputType> &&x) {
                    reducer_(&state_, std::move(x));
                    return stateReader_(x.environment, state_);
                }
            };
            return M::template kleisli<typename Reducer::InputType>(F(std::move(reducer), std::move(stateReader), initState), liftParam);
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
        class SimpleMovingTimeWindowAction {
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
            SimpleMovingTimeWindowAction(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
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
        template <class T, class PushPopHandler>
        class SimpleMovingTimeWindowActionWithInputAttached {
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
            SimpleMovingTimeWindowActionWithInputAttached(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
            typename M::template Data<std::tuple<T,Result>> operator()(typename M::template InnerData<T> &&a) {
                auto *p = a.environment;
                auto tp = a.timedData.timePoint;
                auto aCopy = infra::withtime_utils::makeCopy<typename M::TimePoint, T>(a.timedData);
                w_.handleData(&state_, handler_, std::tuple<typename M::TimePoint, T> {std::move(a.timedData.timePoint), std::move(a.timedData.value)});
                auto res = handler_.readResult(state_);
                if (res) {
                    return { typename M::template InnerData<std::tuple<T,Result>> {
                        p,
                        { p->resolveTime(tp), {std::move(aCopy.value), std::move(*res)}, a.timedData.finalFlag }
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
        class ComplexMovingTimeWindowAction {
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
            ComplexMovingTimeWindowAction(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
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
        template <class A, class B, class PushPopHandler>
        class ComplexMovingTimeWindowActionWithInputAttached {
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
            ComplexMovingTimeWindowActionWithInputAttached(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
            typename M::template Data<std::tuple<B,Result>> operator()(int which, typename M::template InnerData<A> &&a, typename M::template InnerData<B> &&b) {
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
                            auto bCopy = infra::withtime_utils::makeCopy<typename M::TimePoint, B>(b.timedData);
                            w_.handleTimePoint(&state_, handler_, tp);
                            auto res = handler_.readResult(state_, std::tuple<typename M::TimePoint, B> {std::move(b.timedData.timePoint), std::move(b.timedData.value)});
                            if (res) {
                                return { typename M::template InnerData<std::tuple<B,Result>> {
                                    b.environment,
                                    { b.environment->resolveTime(tp), {std::move(bCopy.value), std::move(*res)}, b.timedData.finalFlag }
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
        static auto simpleMovingTimeWindowAction(
            PushPopHandler &&handler
            , typename MovingTimeWindow<T,PushPopHandler>::Duration const &duration
            , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()
        ) -> std::shared_ptr<typename M::template Action<T, typename SimpleMovingTimeWindowAction<T,PushPopHandler>::Result>>
        {
            return M::template kleisli<T>(
                SimpleMovingTimeWindowAction<T,PushPopHandler>(std::move(handler), duration)
                , liftParam
            );
        }
        template <class A, class B, class PushPopHandler>
        static auto complexMovingTimeWindowAction(
            PushPopHandler &&handler
            , typename MovingTimeWindow<A,PushPopHandler>::Duration const &duration
            , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()
        ) -> std::shared_ptr<typename M::template Action<std::variant<A,B>, typename ComplexMovingTimeWindowAction<A,B,PushPopHandler>::Result>>
        {
            return M::template kleisli<std::variant<A,B>>(
                ComplexMovingTimeWindowAction<A,B,PushPopHandler>(std::move(handler), duration)
                , liftParam
            );
        }
        template <class T, class PushPopHandler>
        static auto simpleMovingTimeWindowActionWithInputAttached(
            PushPopHandler &&handler
            , typename MovingTimeWindow<T,PushPopHandler>::Duration const &duration
            , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()
        ) -> std::shared_ptr<typename M::template Action<T, std::tuple<T,typename SimpleMovingTimeWindowAction<T,PushPopHandler>::Result>>>
        {
            return M::template kleisli<T>(
                SimpleMovingTimeWindowActionWithInputAttached<T,PushPopHandler>(std::move(handler), duration)
                , liftParam
            );
        }
        template <class A, class B, class PushPopHandler>
        static auto complexMovingTimeWindowActionWithInputAttached(
            PushPopHandler &&handler
            , typename MovingTimeWindow<A,PushPopHandler>::Duration const &duration
            , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()
        ) -> std::shared_ptr<typename M::template Action<std::variant<A,B>, std::tuple<B,typename ComplexMovingTimeWindowAction<A,B,PushPopHandler>::Result>>>
        {
            return M::template kleisli<std::variant<A,B>>(
                ComplexMovingTimeWindowActionWithInputAttached<A,B,PushPopHandler>(std::move(handler), duration)
                , liftParam
            );
        }
    private:
        //Requirement for whole history handler:
        //void add(std::tuple<typename M::TimePoint, T> const &);
        //std::optional<...> readResult()
        template <class T, class WholeHistoryHandler>
        class WholeHistoryAction {
        public:
            using Result = typename decltype(((WholeHistoryHandler *) nullptr)->readResult())::value_type;
        private:
            WholeHistoryHandler handler_;
        public:
            WholeHistoryAction(WholeHistoryHandler &&handler) : handler_(std::move(handler)) {}
            WholeHistoryAction(WholeHistoryAction &&) = default;
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
        template <class T, class WholeHistoryHandler>
        class WholeHistoryActionWithInputAttached {
        public:
            using Result = typename decltype(((WholeHistoryHandler *) nullptr)->readResult())::value_type;
        private:
            WholeHistoryHandler handler_;
        public:
            WholeHistoryActionWithInputAttached(WholeHistoryHandler &&handler) : handler_(std::move(handler)) {}
            WholeHistoryActionWithInputAttached(WholeHistoryActionWithInputAttached &&) = default;
            typename M::template Data<std::tuple<T,Result>> operator()(typename M::template InnerData<T> &&a) {
                auto tp = a.timedData.timePoint;
                auto aCopy = infra::withtime_utils::makeCopy<typename M::TimePoint, T>(a.timedData);
                handler_.add(std::tuple<typename M::TimePoint, T> {std::move(a.timedData.timePoint), std::move(a.timedData.value)});
                auto res = handler_.readResult();
                if (res) {
                    return { typename M::template InnerData<std::tuple<T,Result>> {
                        a.environment,
                        { a.environment->resolveTime(tp), {std::move(aCopy.value), std::move(*res)}, a.timedData.finalFlag }
                    } };
                } else {
                    return std::nullopt;
                }
            }
        };
    public:
        template <class T, class WholeHistoryHandler>
        static auto wholeHistoryAction(
            WholeHistoryHandler &&handler
            , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()
        ) -> std::shared_ptr<typename M::template Action<T, typename WholeHistoryAction<T,WholeHistoryHandler>::Result>>
        {
            return M::template kleisli<T>(
                WholeHistoryAction<T,WholeHistoryHandler>(std::move(handler))
                , liftParam
            );
        }
        template <class T, class WholeHistoryHandler>
        static auto wholeHistoryActionWithInputAttached(
            WholeHistoryHandler &&handler
            , infra::LiftParameters<typename M::TimePoint> const &liftParam = infra::LiftParameters<typename M::TimePoint>()
        ) -> std::shared_ptr<typename M::template Action<T, std::tuple<T,typename WholeHistoryAction<T,WholeHistoryHandler>::Result>>>
        {
            return M::template kleisli<T>(
                WholeHistoryActionWithInputAttached<T,WholeHistoryHandler>(std::move(handler))
                , liftParam
            );
        }
    };

} } } }

#endif