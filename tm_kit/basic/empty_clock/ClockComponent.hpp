#ifndef TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_COMPONENT_HPP_
#define TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_COMPONENT_HPP_

#include <functional>
#include <memory>
#include <tm_kit/basic/empty_clock/Clock.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace empty_clock {
    template <class TimePoint=std::chrono::system_clock::time_point>
    class ClockComponent : public Clock<TimePoint> {
    public:
        using TimePointType = TimePoint;
        using DurationType = decltype(TimePoint()-TimePoint());
        static constexpr bool PreserveInputRelativeOrder = true;
        static constexpr bool CanBeActualTimeClock = false;
        
        TimePointType resolveTime() {
            return Clock<TimePoint>::now();
        }
        TimePointType resolveTime(TimePointType const &triggeringInputTime) {
            return triggeringInputTime;
        }
    };

} } } } }

#endif