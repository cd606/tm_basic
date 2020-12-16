#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent, Env>, int> = 0>
    class ClockOnOrderFacility {
    public:
        using M = infra::RealTimeApp<Env>;
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
                    std::vector<Duration> filteredDurations;
                    for (auto const &d : input.timedData.value.key().callbackDurations) {
                        if (d >= Duration(0)) {
                            filteredDurations.push_back(d);
                        }
                    }
                    Env *env = input.environment;
                    typename Env::IDType id = input.timedData.value.id();
                    if (filteredDurations.empty()) {
                        env->createOneShotDurationTimer(Duration(0), [this,env,id]() {
                            auto now = env->now();
                            T t = converter_(now, 0, 0);
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                env, {now, {id, std::move(t)}, true}
                            });
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        for (size_t ii=0; ii<count; ++ii) {
                            bool isFinal = (ii == count-1);
                            env->createOneShotDurationTimer(filteredDurations[ii], [this,env,id,isFinal,ii,count]() {
                                auto now = env->now();
                                T t = converter_(now, ii, count);
                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                    env, {now, {id, std::move(t)}, isFinal}
                                });
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