#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_HPP_

#include <chrono>
#include <ctime>
#include <optional>
#include <tm_kit/infra/ChronoUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {
    
    template <class TimePoint, bool AllowLocalTimeOverride=false>
    class Clock;
    
    template <class TimePoint>
    class Clock<TimePoint, true> {
    public:
        using TimePointType = TimePoint;
    private:
        TimePoint latestTime_;
        std::optional<TimePoint> localTime_;
    protected:
        void updateLatestTime(TimePoint const &t) {
            if (t > latestTime_) {
                latestTime_ = t;
            } 
        }
    public:
        TimePoint now() const {
            if (localTime_) {
                return *localTime_;
            }
            return latestTime_;
        }
        uint64_t currentTimeMillis() const {
            if constexpr (std::is_same_v<TimePoint, std::chrono::system_clock::time_point>) {
                return infra::withtime_utils::sinceEpoch<std::chrono::milliseconds>(now());
            } else {
                throw std::runtime_error("single_pass_iteration_clock::currentTimeMillis only makes sense when the time point type is system_clock's time type");
            }
        }
        void sleepFor(decltype(TimePoint()-TimePoint()) const &duration) {
            latestTime_ += duration;
        }
        void setLocalTime(TimePoint const &t) {
            localTime_ = t;
        }
        void resetLocalTime() {
            localTime_ = std::nullopt;
        }
    };
    template <class TimePoint>
    class Clock<TimePoint, false> {
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
        uint64_t currentTimeMillis() const {
            if constexpr (std::is_same_v<TimePoint, std::chrono::system_clock::time_point>) {
                return infra::withtime_utils::sinceEpoch<std::chrono::milliseconds>(now());
            } else {
                throw std::runtime_error("single_pass_iteration_clock::currentTimeMillis only makes sense when the time point type is system_clock's time type");
            }
        }
        void sleepFor(decltype(TimePoint()-TimePoint()) const &duration) {
            latestTime_ += duration;
        }
    };

} } } } }

#endif