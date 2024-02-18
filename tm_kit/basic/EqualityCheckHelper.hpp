#ifndef TM_KIT_BASIC_EQUALITY_CHECK_HELPER_HPP_
#define TM_KIT_BASIC_EQUALITY_CHECK_HELPER_HPP_

#include <ctime>
#include <valarray>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

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
    template <class T>
    class EqualityCheckHelper<std::valarray<T>> {
    public:
        static bool check(std::valarray<T> const &a, std::valarray<T> const &b) {
            if (a.size() == b.size()) {
                for (std::size_t ii=0; ii<a.size(); ++ii) {
                    if (!(a[ii] == b[ii])) {
                        return false;
                    }
                }
                return true;
            } else {
                return false;
            }
        }
    };
    
    template <class T>
    class ComparisonHelper {
    public:
        static bool check(T const &a, T const &b) {
            return (a < b);
        }
    };
    template <>
    class ComparisonHelper<std::tm> {
    public:
        static bool check(std::tm const &a, std::tm const &b) {
            if (a.tm_year < b.tm_year) {
                return true;
            }
            if (a.tm_year > b.tm_year) {
                return false;
            }
            if (a.tm_mon < b.tm_mon) {
                return true;
            }
            if (a.tm_mon > b.tm_mon) {
                return false;
            }
            if (a.tm_mday < b.tm_mday) {
                return true;
            }
            if (a.tm_mday > b.tm_mday) {
                return false;
            }
            if (a.tm_hour < b.tm_hour) {
                return true;
            }
            if (a.tm_hour > b.tm_hour) {
                return false;
            }
            if (a.tm_min < b.tm_min) {
                return true;
            }
            if (a.tm_min > b.tm_min) {
                return false;
            }
            if (a.tm_sec < b.tm_sec) {
                return true;
            }
            if (a.tm_sec > b.tm_sec) {
                return false;
            }
            return false;
        }
    };
    template <class T>
    class ComparisonHelper<std::valarray<T>> {
    public:
        static bool check(std::valarray<T> const &a, std::valarray<T> const &b) {
            for (std::size_t ii=0; ii<a.size() && ii<b.size(); ++ii) {
                auto b = ComparisonHelper<T>::check(a[ii], b[ii]);
                if (b) {
                    return true;
                }
                b = ComparisonHelper<T>::check(b[ii], a[ii]);
                if (b) {
                    return false;
                }
                
            }
            return (a.size() < b.size());
        }
    };
    template <class T, class Hash>
    class ComparisonHelper<std::unordered_set<T, Hash>> {
    public:
        static bool check(std::unordered_set<T, Hash> const &a, std::unordered_set<T, Hash> const &b) {
            for (auto const &x : a) {
                if (b.find(x) == b.end()) {
                    return false;
                }
            }
            return true;
        }
    };
    template <class T, class Cmp>
    class ComparisonHelper<std::set<T, Cmp>> {
    public:
        static bool check(std::set<T, Cmp> const &a, std::set<T, Cmp> const &b) {
            for (auto const &x : a) {
                if (b.find(x) == b.end()) {
                    return false;
                }
            }
            return true;
        }
    };
    template <class T, class U, class Hash>
    class ComparisonHelper<std::unordered_map<T, U, Hash>> {
    public:
        static bool check(std::unordered_map<T, U, Hash> const &a, std::unordered_map<T, U, Hash> const &b) {
            for (auto const &x : a) {
                auto iter = b.find(x.first);
                if (iter == b.end()) {
                    return false;
                }
                if (!ComparisonHelper<U>::check(x.second, iter->second)) {
                    return false;
                }
            }
            return true;
        }
    };
    template <class T, class U, class Cmp>
    class ComparisonHelper<std::map<T, U, Cmp>> {
    public:
        static bool check(std::map<T, U, Cmp> const &a, std::map<T, U, Cmp> const &b) {
            for (auto const &x : a) {
                auto iter = b.find(x.first);
                if (iter == b.end()) {
                    return false;
                }
                if (!ComparisonHelper<U>::check(x.second, iter->second)) {
                    return false;
                }
            }
            return true;
        }
    };
}}}}
#endif
