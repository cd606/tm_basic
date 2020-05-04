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
        
        template <class T, class F>
        static std::shared_ptr<typename M::template Action<T,T>> liftPureFilter(F &&f, bool suggestThreaded=false) {
            return M::template liftMaybe<T>([f=std::move(f)](T &&a) -> std::optional<T> {
                if (f(a)) {
                    return {std::move(a)};
                } else {
                    return std::nullopt;
                }
            }, suggestThreaded);
        }

        template <class T, class F>
        static std::shared_ptr<typename M::template Action<T,T>> kleisliFilter(F &&f, bool suggestThreaded=false) {
            return M::template kleisli<T>([f=std::move(f)](typename M::template InnerData<T> &&a) -> typename M::template Data<T> {
                if (f(a)) {
                    return {std::move(a)};
                } else {
                    return std::nullopt;
                }
            }, suggestThreaded);
        }

        template <class Reducer, class StateReader>
        static auto liftPureReduce(Reducer &&reducer, StateReader &&stateReader, typename Reducer::StateType const &initState, bool suggestThreaded=false)
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
            return M::template liftPure<typename Reducer::InputType>(F(std::move(reducer), std::move(stateReader), initState), suggestThreaded);
        }

        template <class Reducer, class StateReader>
        static auto kleisliReduce(Reducer &&reducer, StateReader &&stateReader, typename Reducer::StateType const &initState, bool suggestThreaded=false)
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
            return M::template kleisli<typename Reducer::InputType>(F(std::move(reducer), std::move(stateReader), initState), suggestThreaded);
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
            using StateType = std::deque<typename M::template TimedDataType<T>>;
            MovingTimeWindow(Duration const &duration) : duration_(duration) {}
            void handleData(StateType *state, PushPopHandler const &handler, typename M::template InnerData<T> &&data) {
                handleTimePoint(data.timedData.timePoint);
                state->push_back(std::move(data.timedData));
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
        //void handlePush(M::TimedDataType<A> const &);
        //void handlePop(M::TimedDataType<A> const &);
        //std::optional<...> readResult(M::EnvironmentType *, std::deque<M::TimedDataType<A>> const &, M::TimedDataType<B> &&)
        template <class A, class B, class PushPopHandler>
        class MovingTimeWindowAction {
        private:
            using MTW = MovingTimeWindow<A,PushPopHandler>;
            using State = typename MTW::StateType;
        public:
            using Duration = typename MTW::Duration;
            using Result = typename decltype(((PushPopHandler *) nullptr)->readResult(
                (typename M::EnvironmentType *) nullptr
                , (*(State *) nullptr)
                , std::move(* ((typename M::template TimedDataType<B> *) nullptr))
            ))::value_type;
        private:
            PushPopHandler handler_;
            MTW w_;
            State state_;
        public:
            MovingTimeWindowAction(PushPopHandler &&handler, Duration const &d) : handler_(std::move(handler)), w_(d), state_() {}
            typename M::template Data<Result> operator()(int which, typename M::template InnerData<A> &&a, typename M::template InnerData<B> &&b) {
                switch (which) {
                    case 0:
                        {
                            w_.handleData(&state_, handler_, std::move(a));
                            return std::nullopt;
                        }
                        break;
                    case 1:
                        {
                            auto tp = b.timedData.timePoint;
                            w_.handleTimePoint(&state_, handler_, tp);
                            auto res = handler_.readResult(b.environment, state_, std::move(b.timedData));
                            if (res) {
                                return { typename M::template InnerData<Result> {
                                    b.environment,
                                    { b.environment->resolveTime(tp), *res, b.finalFlag }
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
        template <class A, class B, class PushPopHandler>
        static auto movingTimeWindowAction(PushPopHandler &&handler, typename MovingTimeWindow<A,PushPopHandler>::Duration const &duration)
            -> std::shared_ptr<typename M::template Action<std::variant<A,B>, typename MovingTimeWindowAction<A,B,PushPopHandler>::Result>>
        {
            return M::template kleisli<std::variant<A,B>>(
                MovingTimeWindowAction<A,B,PushPopHandler>(std::move(handler), duration)
            );
        }
    };

} } } }

#endif