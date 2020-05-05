#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeMonad.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent, Env>, int> = 0>
    class ClockOnOrderFacility {
    public:
        using M = infra::RealTimeMonad<Env>;
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
                            T t = converter_(now);
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                env, {now, {id, std::move(t)}, true}
                            });
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        for (size_t ii=0; ii<filteredDurations.size(); ++ii) {
                            bool isFinal = (ii == filteredDurations.size()-1);
                            env->createOneShotDurationTimer(filteredDurations[ii], [this,env,id,isFinal]() {
                                auto now = env->now();
                                T t = converter_(now);
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