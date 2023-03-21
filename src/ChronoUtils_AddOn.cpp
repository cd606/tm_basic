#include "tm_kit/basic/ChronoUtils_AddOn.hpp"
#include <date/tz.h>

namespace dev { namespace cd606 { namespace tm { namespace infra {
    namespace withtime_utils {
        std::chrono::system_clock::time_point parseZonedTime(int year, int month, int day, int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName) {
#if __cplusplus >= 202002L
            std::chrono::zoned_time<
                std::chrono::system_clock::duration
            > zt (timeZoneName, std::chrono::local_days {std::chrono::year_month_day {std::chrono::year(year), std::chrono::month(month), std::chrono::day(day)}});
            return zt.get_sys_time()+std::chrono::hours(hour)+std::chrono::minutes(minute)+std::chrono::seconds(second)+std::chrono::microseconds(microseconds);
#else
            date::zoned_time<
                std::chrono::system_clock::duration
            > zt (timeZoneName, date::local_days {date::year_month_day {date::year(year), date::month(month), date::day(day)}});
            return zt.get_sys_time()+std::chrono::hours(hour)+std::chrono::minutes(minute)+std::chrono::seconds(second)+std::chrono::microseconds(microseconds);
#endif
        }
    }
} } } }