#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <vector>
#include <queue>
#include <tm_kit/infra/SinglePassIterationMonad.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent<typename Env::TimePointType>, Env>, int> = 0>
    class ClockOnOrderFacility {
    public:
        using M = infra::SinglePassIterationMonad<Env>;
        using Duration = typename Env::DurationType;
        template <class S>
        struct FacilityInput {
            S inputData;
            std::vector<Duration> callbackDurations;
        };
        template <class S, class T>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createClockCallback(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter) {
            class LocalF final : public M::template AbstractOnOrderFacility<FacilityInput<S>,T> {
            private:
                std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter_;
            public:
                LocalF(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter)
                    : converter_(converter)
                {
                }
                virtual void handle(typename M::template InnerData<typename M::template Key<FacilityInput<S>>> &&input) override final {
                    auto now_tp = input.timedData.timePoint;
                    std::vector<Duration> filteredDurations;
                    for (auto const &d : input.timedData.value.key().callbackDurations) {
                        if (d >= Duration(0)) {
                            filteredDurations.push_back(d);
                        }
                    }
                    auto id = input.timedData.value.id();
                    if (filteredDurations.empty()) {
                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                            input.environment
                            , {
                                now_tp
                                , {id, converter_(now_tp, 0, 0)}
                                , true
                            }
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        for (size_t ii=0; ii<count; ++ii) {
                            auto fire_tp = now_tp + filteredDurations[ii];
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                input.environment
                                , {
                                    fire_tp
                                    , {id, converter_(fire_tp, ii, count)}
                                    , ii == count-1
                                }
                            });
                        }
                    }
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF(converter));
        }

        //This utility function is provided because in SinglePassIterationMonad
        //it is difficult to ensure a smooth exit. Even though we can pass down
        //finalFlag, the flag might get lost. An example is where an action happens
        //to return a std::nullopt at the input with the final flag, and at this point
        //the finalFlag is lost forever.
        //To help exit from SinglePassIterationMonad where the final flag may get lost
        //, we can set up this exit timer.
        template <class T>
        static void setupExitTimer(
            infra::MonadRunner<M> &r
            , std::chrono::system_clock::duration const &exitAfterThisDurationFromFirstInput
            , typename infra::MonadRunner<M>::template Source<T> &&inputSource
            , std::function<void(Env *)> wrapUpFunc
            , std::string const &componentNamePrefix) 
        {
            auto exitTimer = createClockCallback<basic::VoidStruct, basic::VoidStruct>(
                [](std::chrono::system_clock::time_point const &, std::size_t thisIdx, std::size_t totalCount) {
                    return basic::VoidStruct {};
                }
            );
            auto setupExitTimer = M::template liftMaybe<T>(
                [exitAfterThisDurationFromFirstInput](T &&) -> std::optional<typename M::template Key<FacilityInput<basic::VoidStruct>>> {
                    static bool timerSet { false };
                    if (!timerSet) {
                        timerSet = true;
                        return infra::withtime_utils::keyify<FacilityInput<basic::VoidStruct>,Env>(
                            FacilityInput<basic::VoidStruct> {
                                basic::VoidStruct {}
                                , {exitAfterThisDurationFromFirstInput}
                            }
                        );
                    } else {
                        return std::nullopt;
                    }
                }
            );
            using ClockFacilityOutput = typename M::template KeyedData<FacilityInput<basic::VoidStruct>, basic::VoidStruct>;
            auto doExit = M::template simpleExporter<ClockFacilityOutput>(
                [wrapUpFunc](typename M::template InnerData<ClockFacilityOutput> &&data) {
                    data.environment->resolveTime(data.timedData.timePoint);
                    wrapUpFunc(data.environment);
                    data.environment->exit();
                }
            );

            r.placeOrderWithFacility(
                r.execute(componentNamePrefix+"_setupExitTimer", setupExitTimer, std::move(inputSource))
                , componentNamePrefix+"_exitTimer", exitTimer
                , r.exporterAsSink(componentNamePrefix+"_doExit", doExit)
            );
        }           
    };

} } } } }

#endif