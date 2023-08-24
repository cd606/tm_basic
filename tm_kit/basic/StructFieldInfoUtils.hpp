#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_UTILS_HPP_

#include <string>
#include <sstream>
#include <array>
#include <list>
#include <vector>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <valarray>
#include <chrono>
#include <ctime>
#include <iomanip>

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/TimePointAsString.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/PrintHelper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {  
    template <class T>
    class StructFieldInfoBasedInitializer {
    private:
        template <int FieldCount, int FieldIndex>
        static void initialize_internal(T &t) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                StructFieldInfoBasedInitializer<
                    typename StructFieldTypeInfo<T,FieldIndex>::TheType
                >::initialize(StructFieldTypeInfo<T,FieldIndex>::access(t));
                if constexpr (FieldIndex < FieldCount-1) {
                    initialize_internal<FieldCount,FieldIndex+1>(t);
                }
            }
        }
    public:
        static void initialize(T &t) {
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                return initialize_internal<StructFieldInfo<T>::FIELD_NAMES.size(), 0>(t);
#ifdef _MSC_VER
            } else if constexpr (std::is_same_v<T, std::tm>) {
                //in MSVC, if we initialize std::tm to all 0
                //and then use std::put_time to output it, 
                //there will be a problem since this std::tm
                //value is not a valid date. The other compilers
                //do not seem to care about this.
                t = std::tm {};
                t.tm_mday = 1;
#endif
            } else {
                t = T {};
            }
        }
    };

    template <class T>
    inline void initialize(T &t) {
        StructFieldInfoBasedInitializer<T>::initialize(t);
    }

    template <class T>
    class StructFieldInfoBasedHash {
    private:
        template <int FieldCount, int FieldIndex>
        static std::size_t hash_internal(T const &t, std::size_t soFar) {
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                    auto fieldHash = StructFieldInfoBasedHash<
                        typename StructFieldTypeInfo<T,FieldIndex>::TheType
                    >()(StructFieldTypeInfo<T,FieldIndex>::constAccess(t));
                    std::size_t newVal = (soFar*16777619)^fieldHash;
                    if constexpr (FieldIndex < FieldCount-1) {
                        return hash_internal<FieldCount,FieldIndex+1>(t, newVal);
                    } else {
                        return newVal;
                    }
                } else {
                    return soFar;
                }
            } else {
                return 0;
            }
        }
    public:
        std::size_t operator()(T const &t) const {
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                return hash_internal<StructFieldInfo<T>::FIELD_NAMES.size(), 0>(t, 2166136261);
            } else {
                return std::hash<T>()(t);
            }
        }
    };
    template <class T, std::size_t N>
    class StructFieldInfoBasedHash<std::array<T,N>> {
    public:
        std::size_t operator()(std::array<T,N> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<T>()(item);
            }
            return ret;
        }
    };
    template <class T>
    class StructFieldInfoBasedHash<std::optional<T>> {
    public:
        std::size_t operator()(std::optional<T> const &t) const {
            if (!t) {
                return 2166136261;
            } else {
                return (2166136261*16777619)^StructFieldInfoBasedHash<T>()(*t);
            }
        }
    };
    template <class T>
    class StructFieldInfoBasedHash<std::list<T>> {
    public:
        std::size_t operator()(std::list<T> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<T>()(item);
            }
            return ret;
        }
    };
    template <class T>
    class StructFieldInfoBasedHash<std::valarray<T>> {
    public:
        std::size_t operator()(std::valarray<T> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<T>()(item);
            }
            return ret;
        }
    };
    template <class T>
    class StructFieldInfoBasedHash<std::vector<T>> {
    public:
        std::size_t operator()(std::vector<T> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<T>()(item);
            }
            return ret;
        }
    };
    template <class T>
    class StructFieldInfoBasedHash<std::deque<T>> {
    public:
        std::size_t operator()(std::deque<T> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<T>()(item);
            }
            return ret;
        }
    };
    template <class T, class Cmp>
    class StructFieldInfoBasedHash<std::set<T,Cmp>> {
    public:
        std::size_t operator()(std::set<T,Cmp> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<T>()(item);
            }
            return ret;
        }
    };
    template <class A, class B, class Cmp>
    class StructFieldInfoBasedHash<std::map<A,B,Cmp>> {
    private:
    public:
        std::size_t operator()(std::map<A,B,Cmp> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^StructFieldInfoBasedHash<A>()(item.first);
                ret = (ret*16777619)^StructFieldInfoBasedHash<B>()(item.second);
            }
            return ret;
        }
    };
    template <class T, class Hash>
    class StructFieldInfoBasedHash<std::unordered_set<T,Hash>> {
    public:
        std::size_t operator()(std::unordered_set<T,Hash> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^Hash()(item);
            }
            return ret;
        }
    };
    template <class A, class B, class Hash>
    class StructFieldInfoBasedHash<std::unordered_map<A,B,Hash>> {
    public:
        std::size_t operator()(std::unordered_map<A,B,Hash> const &t) const {
            std::size_t ret = 2166136261;
            for (auto const &item : t) {
                ret = (ret*16777619)^Hash()(item.first);
                ret = (ret*16777619)^StructFieldInfoBasedHash<B>()(item.second);
            }
            return ret;
        }
    };
    template <>
    class StructFieldInfoBasedHash<std::chrono::system_clock::time_point> {
    public:
        std::size_t operator()(std::chrono::system_clock::time_point const &t) const {
            return (std::size_t) (std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count());
        }
    };
    template <typename TimeZoneSpec>
    class StructFieldInfoBasedHash<TimePointAsString<TimeZoneSpec>> {
    public:
        std::size_t operator()(TimePointAsString<TimeZoneSpec> const &t) const {
            return (std::size_t) (std::chrono::duration_cast<std::chrono::milliseconds>(t.value().time_since_epoch()).count());
        }
    };
    template <>
    class StructFieldInfoBasedHash<std::tm> {
    public:
        std::size_t operator()(std::tm const &t) const {
            std::size_t ret = 2166136261;
            ret = (ret*16777619)^((std::size_t) t.tm_sec);
            ret = (ret*16777619)^((std::size_t) t.tm_min);
            ret = (ret*16777619)^((std::size_t) t.tm_hour);
            ret = (ret*16777619)^((std::size_t) t.tm_mday);
            ret = (ret*16777619)^((std::size_t) t.tm_mon);
            ret = (ret*16777619)^((std::size_t) t.tm_year);
            ret = (ret*16777619)^((std::size_t) t.tm_wday);
            ret = (ret*16777619)^((std::size_t) t.tm_yday);
            ret = (ret*16777619)^((std::size_t) t.tm_isdst);
            return ret;
        }
    };
    template <>
    class StructFieldInfoBasedHash<ByteData> {
    public:
        std::size_t operator()(ByteData const &t) const {
            return std::hash<std::string>()(t.content);
        }
    };
    template <>
    class StructFieldInfoBasedHash<ByteDataWithTopic> {
    public:
        std::size_t operator()(ByteDataWithTopic const &t) const {
            std::size_t ret = 2166136261;
            ret = (ret*16777619)^std::hash<std::string>()(t.topic);
            ret = (ret*16777619)^std::hash<std::string>()(t.content);
	    return ret;
        }
    };
    template <>
    class StructFieldInfoBasedHash<ByteDataWithID> {
    public:
        std::size_t operator()(ByteDataWithID const &t) const {
            std::size_t ret = 2166136261;
            ret = (ret*16777619)^std::hash<std::string>()(t.id);
            ret = (ret*16777619)^std::hash<std::string>()(t.content);
	    return ret;
        }
    };
}}}}}

#endif
