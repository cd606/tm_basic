#ifndef TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <vector>
#include <queue>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/empty_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace empty_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent<typename Env::TimePointType>, Env>, int> = 0>
    class ClockOnOrderFacility {
    public:
        using M = infra::BasicWithTimeApp<Env>;
        using Duration = typename Env::DurationType;
        template <class S>
        struct FacilityInput {
            S inputData;
            std::vector<Duration> callbackDurations;
        };
        template <class S, class T>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createClockCallback(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter) {
            return std::make_shared<typename M::template OnOrderFacility<FacilityInput<S>,T>>();
        }
        template <class S, class T, class F>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createGenericClockCallback(F &&converter) {
            return std::make_shared<typename M::template OnOrderFacility<FacilityInput<S>,T>>();
        }  
    };

} } } } }

#endif