#ifndef TM_KIT_BASIC_DATE_HOLDER_HPP_
#define TM_KIT_BASIC_DATE_HOLDER_HPP_

#include <iostream>
#include <iomanip>
#include <functional>
#include <chrono>
#include <ctime>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    struct DateHolder {
        uint16_t year;
        uint8_t month;
        uint8_t day;
    };

    inline std::ostream &operator<<(std::ostream &os, DateHolder const &d) {
        os << std::setw(4) << std::setfill('0') << (int) d.year 
            << '-'
            << std::setw(2) << std::setfill('0') << (int) d.month
            << '-'
            << std::setw(2) << std::setfill('0') << (int) d.day;
        return os;
    }

    inline bool operator==(DateHolder const &d1, DateHolder const &d2) {
        return (d1.year==d2.year && d1.month==d2.month && d1.day==d2.day);
    }
    
    inline bool operator<(DateHolder const &d1, DateHolder const &d2) {
        if (d1.year < d2.year) {
            return true;
        }
        if (d1.year > d2.year) {
            return false;
        }
        if (d1.month < d2.month) {
            return true;
        }
        if (d1.month > d2.month) {
            return false;
        }
        return (d1.day < d2.day);
    }

    inline bool operator>(DateHolder const &d1, DateHolder const &d2) {
        return (d2 < d1);
    }
    inline bool operator<=(DateHolder const &d1, DateHolder const &d2) {
        return !(d2 < d1);
    }
    inline bool operator>=(DateHolder const &d1, DateHolder const &d2) {
        return !(d1 < d2);
    }
    inline bool operator!=(DateHolder const &d1, DateHolder const &d2) {
        return !(d1 == d2);
    }

    inline DateHolder dateHolderFromTimePoint(std::chrono::system_clock::time_point const &tp) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm *m = std::localtime(&t);
        return DateHolder {
            static_cast<uint16_t>(m->tm_year+1900)
            , static_cast<uint8_t>(m->tm_mon+1)
            , static_cast<uint8_t>(m->tm_mday)
        };
    }
} } } }

namespace std {
    template <>
    class hash<dev::cd606::tm::basic::DateHolder> {
    public:
        size_t operator()(dev::cd606::tm::basic::DateHolder const &d) const {
            std::size_t ret = 2166136261;
            ret = (ret*16777619)^((std::size_t) d.year);
            ret = (ret*16777619)^((std::size_t) d.month);
            ret = (ret*16777619)^((std::size_t) d.day);
            return ret;
        }
    };
    template <>
    class equal_to<dev::cd606::tm::basic::DateHolder> {
    public:
        bool operator()(dev::cd606::tm::basic::DateHolder const &d1, dev::cd606::tm::basic::DateHolder const &d2) const {
            return (d1==d2);
        }
    };
    template <>
    class less<dev::cd606::tm::basic::DateHolder> {
    public:
        bool operator()(dev::cd606::tm::basic::DateHolder const &d1, dev::cd606::tm::basic::DateHolder const &d2) const {
            return (d1<d2);
        }
    };
}

#endif