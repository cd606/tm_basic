#ifndef TM_KIT_BASIC_EQUALITY_CHECK_HELPER_HPP_
#define TM_KIT_BASIC_EQUALITY_CHECK_HELPER_HPP_

#include <ctime>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T>
    class EqualityCheckHelper {
    public:
        static bool check(T const &a, T const &b) {
            return (a == b);
        }
    };
    template <>
    class EqualityCheckHelper<std::tm> {
    public:
        static bool check(std::tm const &a, std::tm const &b) {
            return (a.tm_year == b.tm_year && a.tm_mon == b.tm_mon && a.tm_mday == b.tm_mday
                && a.tm_hour == b.tm_hour && a.tm_min == b.tm_min && a.tm_sec == b.tm_sec);
        }
    };
    
}}}}
#endif