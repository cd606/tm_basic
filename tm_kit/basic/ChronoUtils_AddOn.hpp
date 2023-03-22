#ifndef TM_KIT_BASIC_CHRONO_UTILS_ADD_ON_HPP_
#define TM_KIT_BASIC_CHRONO_UTILS_ADD_ON_HPP_

#include <tm_kit/infra/ChronoUtils.hpp>

//This is "add on" to the infra withtime_utils files, so the functions
//stay in the tm::infra namespace
namespace dev { namespace cd606 { namespace tm { namespace infra {
    namespace withtime_utils {
        //The implementation may break down on dst-changing Sundays
        extern std::chrono::system_clock::time_point parseZonedTime(int year, int month, int day, int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName);
        extern std::chrono::system_clock::time_point parseZonedTime(std::string_view const &timeString, std::string_view const &timeZoneName);
        template <class Duration>
        inline int64_t sinceZonedMidnight(std::chrono::system_clock::time_point const &tp, std::string_view const &timeZoneName) {
            std::time_t t = std::chrono::system_clock::to_time_t(tp);
            std::tm *m = std::localtime(&t);
            auto midnight = parseZonedTime(
                m->tm_year+1900, m->tm_mon+1, m->tm_mday, 0, 0, 0, 0, timeZoneName
            );
            return static_cast<int64_t>(std::chrono::duration_cast<Duration>(tp-midnight).count());
        }
    }
} } } }

#endif