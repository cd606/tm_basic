#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_COMPONENT_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_COMPONENT_HPP_

#include <functional>
#include <memory>
#include <tm_kit/basic/single_pass_iteration_clock/Clock.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {
    template <class TimePoint, bool AllowLocalTimeOverride=false>
    class ClockComponent : public Clock<TimePoint, AllowLocalTimeOverride> {
    public:
        using TimePointType = TimePoint;
        using DurationType = decltype(TimePoint()-TimePoint());
        static constexpr bool PreserveInputRelativeOrder = true;
        static constexpr bool CanBeActualTimeClock = false;
        
        TimePointType resolveTime() {
            return Clock<TimePoint, AllowLocalTimeOverride>::now();
        }
        TimePointType resolveTime(TimePointType const &triggeringInputTime) {
            Clock<TimePoint, AllowLocalTimeOverride>::updateLatestTime(triggeringInputTime);
            return triggeringInputTime;
        }
    };

} } } } }

#endif