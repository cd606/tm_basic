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
        //The format is fixed as "yyyy-MM-ddTHH:mm:ss.mmmmmm" (the microsecond part can be omitted)
        std::chrono::system_clock::time_point parseZonedTime(std::string_view const &s, std::string_view const &timeZoneName) {
            if (s.length() == 8) {
                int year = (s[0]-'0')*1000+(s[1]-'0')*100+(s[2]-'0')*10+(s[3]-'0');
                int mon = (s[4]-'0')*10+(s[5]-'0');
                int day = (s[6]-'0')*10+(s[7]-'0');
                return parseZonedTime(year, mon, day, 0, 0, 0, 0, timeZoneName);
            } else {
                int year = (s[0]-'0')*1000+(s[1]-'0')*100+(s[2]-'0')*10+(s[3]-'0');
                int mon = (s[5]-'0')*10+(s[6]-'0');
                int day = (s[8]-'0')*10+(s[9]-'0');
                int hour = 0;
                int min = 0;
                int sec = 0;
                int microsec = 0;
                if (s.length() >= 16) {
                    hour = (s[11]-'0')*10+(s[12]-'0');
                    min = (s[14]-'0')*10+(s[15]-'0');
                    if (s.length() >= 19) {
                        sec = (s[17]-'0')*10+(s[18]-'0');
                    }
                    if (s.length() > 20 && s[19] == '.') {
                        int unit = 100000;
                        for (std::size_t ii=0; ii<6; ++ii,unit/=10) {
                            if (s.length() > (20+ii)) {
                                microsec += (s[20+ii]-'0')*unit;
                            } else {
                                break;
                            }
                        }
                    }
                }
                return parseZonedTime(
                    year, mon, day, hour, min, sec, microsec, timeZoneName
                );
            }
        }
    }
} } } }