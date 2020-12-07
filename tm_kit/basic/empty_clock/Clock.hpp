#ifndef TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_HPP_
#define TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_HPP_

#include <chrono>
#include <ctime>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace empty_clock {
    
    template <class TimePoint=std::chrono::system_clock::time_point>
    class Clock {
    public:
        using TimePointType = TimePoint;
    public:
        TimePoint now() const {
            return TimePoint {};
        }
        void sleepFor(decltype(TimePoint()-TimePoint()) const &duration) {
        }
    };

} } } } }

#endif