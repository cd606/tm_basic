#ifndef TM_KIT_BASIC_CHRONO_UTILS_ADD_ON_HPP_
#define TM_KIT_BASIC_CHRONO_UTILS_ADD_ON_HPP_

#include <tm_kit/infra/ChronoUtils.hpp>

//zoned_time support is not yet complete in gcc, so currently this is only enabled for Visual C++
#if __cplusplus < 202002L || (!defined(_MSC_VER) && !(defined(__GNUC__) && !defined(__llvm__) && (__GNUC__ >= 13)))
#include <date/tz.h>

//This is "add on" to the infra withtime_utils files, so the functions
//stay in the tm::infra namespace
namespace dev { namespace cd606 { namespace tm { namespace basic {
    namespace withtime_utils {
        //The implementation may break down on dst-changing Sundays
        extern std::chrono::system_clock::time_point parseZonedTime(int year, int month, int day, int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName);
        extern std::chrono::system_clock::time_point parseZonedTime(std::string_view const &timeString, std::string_view const &timeZoneName);
        extern std::chrono::system_clock::time_point parseZonedTodayActualTime(int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName);
        extern std::string zonedTimeString(std::chrono::system_clock::time_point const &tp, std::string_view const &timeZoneName, bool includeZoneName=false);
        
        template <class Duration>
        inline int64_t sinceZonedMidnight(std::chrono::system_clock::time_point const &tp, std::string_view const &timeZoneName) {
            auto zt = date::make_zoned(date::locate_zone(timeZoneName), tp);
            auto days = date::floor<date::days>(zt.get_local_time());
            auto ymd = date::year_month_day {days};
            auto zt1 = date::make_zoned(date::locate_zone(timeZoneName), date::local_days {ymd});
            return static_cast<int64_t>(std::chrono::duration_cast<Duration>(zt.get_local_time()-zt1.get_local_time()).count());
        }
        template <class Env
            , std::enable_if_t<std::is_same_v<typename Env::TimePointType, std::chrono::system_clock
::time_point>,int> = 0
            >
        class MemorizedZonedMidnight {
        private:
            std::chrono::system_clock::time_point midnight_;
        public:
            MemorizedZonedMidnight(Env *env, std::string_view const &timeZoneName) {
                std::chrono::system_clock::time_point tp = env->now();
                auto zt = date::make_zoned(date::locate_zone(timeZoneName), tp);
                auto days = date::floor<date::days>(zt.get_local_time());
                auto ymd = date::year_month_day {days};
                auto zt1 = date::make_zoned(date::locate_zone(timeZoneName), date::local_days {ymd});
                midnight_ = zt1.get_sys_time();
            }
            MemorizedZonedMidnight(int year, int month, int day, std::string_view const &timeZoneName) {
                midnight_ = parseZonedTime(year, month, day, 0, 0, 0, 0, timeZoneName);
            }
            template <class Duration>
            int64_t sinceMidnight(std::chrono::system_clock::time_point tp) const {
                return static_cast<int64_t>(std::chrono::duration_cast<Duration>(tp-midnight_).count());
            }
            template <class Duration>
            std::chrono::system_clock::time_point midnightDurationToTime(Duration const &d) const {
                return midnight_ + d;
            }
        };
    }
} } } }

namespace dev { namespace cd606 { namespace tm { namespace infra {
    namespace withtime_utils {
        inline std::chrono::system_clock::time_point parseZonedTime(int year, int month, int day, int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName) {
            return basic::withtime_utils::parseZonedTime(year, month, day, hour, minute, second, microseconds, timeZoneName);
        }
        inline std::chrono::system_clock::time_point parseZonedTime(std::string_view const &timeString, std::string_view const &timeZoneName) {
            return basic::withtime_utils::parseZonedTime(timeString, timeZoneName);
        }
        inline std::chrono::system_clock::time_point parseZonedTodayActualTime(int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName) {
            return basic::withtime_utils::parseZonedTodayActualTime(hour, minute, second, microseconds, timeZoneName);
        }
        inline std::string zonedTimeString(std::chrono::system_clock::time_point const &tp, std::string_view const &timeZoneName, bool includeZoneName=false) {
            return basic::withtime_utils::zonedTimeString(tp, timeZoneName, includeZoneName);
        }
        
        template <class Duration>
        inline int64_t sinceZonedMidnight(std::chrono::system_clock::time_point const &tp, std::string_view const &timeZoneName) {
            return basic::withtime_utils::sinceZonedMidnight<Duration>(tp, timeZoneName);
        }
        template <class Env>
        using MemorizedZonedMidnight = basic::withtime_utils::MemorizedZonedMidnight<Env>;
    }
} } } }

#endif

#endif