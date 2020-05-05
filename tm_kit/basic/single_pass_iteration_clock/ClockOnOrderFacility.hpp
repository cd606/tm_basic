#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <vector>
#include <queue>
#include <tm_kit/infra/SinglePassIterationMonad.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
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
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createClockCallback(std::function<T(typename Env::TimePointType const &)> converter) {
            class LocalF final : public M::template AbstractOnOrderFacility<FacilityInput<S>,T> {
            private:
                std::function<T(typename Env::TimePointType const &)> converter_;
            public:
                LocalF(std::function<T(typename Env::TimePointType const &)> converter)
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
                                , {id, converter_(now_tp)}
                                , true
                            }
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        for (size_t ii=0; ii<filteredDurations.size(); ++ii) {
                            auto fire_tp = now_tp + filteredDurations[ii];
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                input.environment
                                , {
                                    fire_tp
                                    , {id, converter_(fire_tp)}
                                    , ii == filteredDurations.size()-1
                                }
                            });
                        }
                    }
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF(converter));
        }
        template <class S, class T>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createConstClockCallback(T const &value) {
            return createClockCallback<S,T>(ConstGenerator<T,typename Env::TimePointType> {value});
        }
        template <class S, class T>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createConstClockCallback(T &&value) {
            return createClockCallback<S,T>(ConstGenerator<T,typename Env::TimePointType> {std::move(value)});
        }
    };

} } } } }

#endif