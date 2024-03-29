#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_COMPONENT_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_COMPONENT_HPP_

#include <functional>
#include <memory>
#include <tm_kit/basic/real_time_clock/Clock.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {
    class ClockComponentImpl;

    class ClockComponent : public Clock {
    private:
        std::unique_ptr<ClockComponentImpl> impl_;
    public:
        using TimePointType = std::chrono::system_clock::time_point;
        using DurationType = std::chrono::system_clock::duration;
        static constexpr bool PreserveInputRelativeOrder = false;
        static constexpr bool CanBeActualTimeClock = true;

        ClockComponent();
        ClockComponent(Clock::ClockSettings const &);
        ClockComponent(ClockComponent &&);
        ClockComponent &operator=(ClockComponent &&);
        ~ClockComponent();
        
        TimePointType resolveTime() {
            return Clock::now();
        }
        TimePointType resolveTime(TimePointType const &/*_triggeringInputTime*/) {
            return Clock::now();
        }
        //the functions returned are cancellors
        std::function<void()> createOneShotTimer(TimePointType const &fireAtTime, std::function<void()> callback);
        std::function<void()> createOneShotDurationTimer(DurationType const &fireAfterDuration, std::function<void()> callback);
        std::function<void()> createRecurringTimer(TimePointType const &firstFireAtTime, TimePointType const &lastFireAtTime, DurationType const &period, std::function<void()> callback);
        std::function<void()> createVariableDurationRecurringTimer(TimePointType const &firstFireAtTime, TimePointType const &lastFireAtTime, std::function<DurationType(TimePointType const &)> periodCalc, std::function<void()> callback);
    };

    class SimplifiedClockComponent : public Clock {
    private:
        std::unique_ptr<ClockComponentImpl> impl_;
    public:
        using TimePointType = std::chrono::system_clock::time_point;
        using DurationType = std::chrono::system_clock::duration;
        static constexpr bool PreserveInputRelativeOrder = false;
        static constexpr bool CanBeActualTimeClock = true;

        SimplifiedClockComponent();
        SimplifiedClockComponent(SimplifiedClockComponent &&);
        SimplifiedClockComponent &operator=(SimplifiedClockComponent &&);
        ~SimplifiedClockComponent();
        
        static TimePointType resolveTime() {
            return TimePointType {};
        }
        static TimePointType resolveTime(TimePointType const &triggeringInputTime) {
            return triggeringInputTime;
        }
        //the functions returned are cancellors
        std::function<void()> createOneShotTimer(TimePointType const &fireAtTime, std::function<void()> callback);
        std::function<void()> createOneShotDurationTimer(DurationType const &fireAfterDuration, std::function<void()> callback);
        std::function<void()> createRecurringTimer(TimePointType const &firstFireAtTime, TimePointType const &lastFireAtTime, DurationType const &period, std::function<void()> callback);
        std::function<void()> createVariableDurationRecurringTimer(TimePointType const &firstFireAtTime, TimePointType const &lastFireAtTime, std::function<DurationType(TimePointType const &)> periodCalc, std::function<void()> callback);
    };

} } } } }

#endif