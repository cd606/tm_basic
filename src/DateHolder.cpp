#include "tm_kit/basic/DateHolder.hpp"
#include <date/tz.h>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    DateHolder dateHolderFromZonedTimePoint(std::chrono::system_clock::time_point const &tp, std::string_view const &timeZoneName) {
        auto zt = date::make_zoned(date::locate_zone(timeZoneName), tp);
        auto days = date::floor<date::days>(zt.get_local_time());
        auto ymd = date::year_month_day {days};
        return DateHolder {(uint16_t) ((int) ymd.year()), (uint8_t) ((unsigned) ymd.month()), (uint8_t) ((unsigned) ymd.day())};
    }
    extern int operator-(DateHolder const &d1, DateHolder const &d2) {
        date::year_month_day ymd1 {date::year {d1.year}, date::month {d1.month}, date::day {d1.day}};
        date::year_month_day ymd2 {date::year {d2.year}, date::month {d2.month}, date::day {d2.day}};
        return (((date::sys_days) ymd1)-((date::sys_days) ymd2)).count();
    }
    extern DateHolder operator+(DateHolder const &d, int offset) {
        date::year_month_day ymd {date::year {d.year}, date::month {d.month}, date::day {d.day}};
        date::sys_days d2 = ((date::sys_days) ymd)+date::days(offset);
        date::year_month_day ymd2 {d2};
        return DateHolder {(uint16_t) ((int) ymd2.year()), (uint8_t) ((unsigned) ymd2.month()), (uint8_t) ((unsigned) ymd2.day())};
    }
    extern DateHolder operator-(DateHolder const &d, int offset) {
        return d+(-offset);
    }
    extern DateHolder operator+(int offset, DateHolder const &d) {
        return d+offset;
    }
} } } }