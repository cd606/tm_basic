#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <vector>
#include <queue>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent<typename Env::TimePointType>, Env>, int> = 0>
    class ClockOnOrderFacility {
    public:
        using M = infra::SinglePassIterationApp<Env>;
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
                    TM_INFRA_FACILITY_TRACER(input.environment);
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
    };

} } } } }

#endif