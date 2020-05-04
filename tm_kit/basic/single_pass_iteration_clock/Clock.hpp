#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_HPP_

#include <chrono>
#include <ctime>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {
    
    template <class TimePoint>
    class Clock {
    public:
        using TimePointType = TimePoint;
    private:
        TimePoint latestTime_;
    protected:
        void updateLatestTime(TimePoint const &t) {
            if (t > latestTime_) {
                latestTime_ = t;
            } 
        }
    public:
        TimePoint now() const {
            return latestTime_;
        }
        TimePoint currentTimeMillis() {
            return latestTime_;
        }

        template <class Duration>
        static uint64_t sinceMidnight(int64_t tp) {
            std::time_t t = (std::time_t) (tp/1000LL);
            std::tm *m = std::localtime(&t);
            m->tm_hour = 0;
            m->tm_min = 0;
            m->tm_sec = 0;
            return (tp - std::mktime(m)*1000LL);
        }
    };

} } } } }

#endif