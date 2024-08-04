#ifndef TM_KIT_BASIC_TIME_POINT_AS_STRING_HPP_
#define TM_KIT_BASIC_TIME_POINT_AS_STRING_HPP_

#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/SerializationHelperMacros_Proxy.hpp>
#include <tm_kit/basic/ChronoUtils_AddOn.hpp>
#include <tm_kit/basic/ConstStringType.hpp>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cmath>
#include <type_traits>
#include <string_view>

#include <stdint.h>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    namespace time_zone_spec {
        struct Utc {};
        struct Local {};
        template <typename TimeZoneName>
        struct Named {
            static constexpr std::string_view TheName = TimeZoneName::VALUE;
        };

        template <class T>
        class IsValidTimeZoneSpec {
        public:
            static constexpr bool Value = false;
        };
        template <>
        class IsValidTimeZoneSpec<Utc> {
        public:
            static constexpr bool Value = true;
        };
        template <>
        class IsValidTimeZoneSpec<Local> {
        public:
            static constexpr bool Value = true;
        };
        template <typename TimeZoneName>
        class IsValidTimeZoneSpec<Named<TimeZoneName>> {
        public:
            static constexpr bool Value = true;
        };
    }
    template <class T>
    class IsTimePointAsString {
    public:
        static constexpr bool Value = false;
    };
    template <typename TimeZoneSpec=time_zone_spec::Utc, typename=std::enable_if_t<time_zone_spec::IsValidTimeZoneSpec<TimeZoneSpec>::Value>>
    class TimePointAsString {
    private:
        std::chrono::system_clock::time_point value_;
    public:
        using TheTimeZoneSpec = TimeZoneSpec;
        TimePointAsString() : value_() {}
        explicit TimePointAsString(std::chrono::system_clock::time_point const &tp) : value_(tp) {}
        ~TimePointAsString() = default;

        template <typename TZ>
        TimePointAsString(TimePointAsString<TZ> const &t) : value_(t.value()) {
        }
        template <typename TZ>
        TimePointAsString<TimeZoneSpec> &operator=(TimePointAsString<TZ> const &t) {
            if constexpr (std::is_same_v<TimeZoneSpec,TZ>) {
                if (this == &t) {
                    return *this;
                }
            }
            value_ = t.value();
            return *this;
        }
        template <typename TZ>
        TimePointAsString(TimePointAsString<TZ> &&t) {
            if constexpr (std::is_same_v<TimeZoneSpec,TZ>) {
                value_ = std::move(t.value_);
            } else {
                value_ = t.value();
                t = TimePointAsString<TZ>();
            }
        }
        template <typename TZ>
        TimePointAsString<TimeZoneSpec> &operator=(TimePointAsString<TZ> &&t) {
            if constexpr (std::is_same_v<TimeZoneSpec,TZ>) {
                if (this == &t) {
                    return *this;
                } else {
                    value_ = std::move(t.value_);
                    return *this;
                }
            } else {
                value_ = t.value();
                t = TimePointAsString<TZ>();
                return *this;
            }
        }
        
        bool operator==(TimePointAsString<TimeZoneSpec> const &another) const {
            return (value_ == another.value_);
        }
        bool operator!=(TimePointAsString<TimeZoneSpec> const &another) const {
            return (value_ != another.value_);
        }
        bool operator>=(TimePointAsString<TimeZoneSpec> const &another) const {
            return (value_ >= another.value_);
        }
        bool operator<=(TimePointAsString<TimeZoneSpec> const &another) const {
            return (value_ <= another.value_);
        }
        bool operator>(TimePointAsString<TimeZoneSpec> const &another) const {
            return (value_ > another.value_);
        }
        bool operator<(TimePointAsString<TimeZoneSpec> const &another) const {
            return (value_ < another.value_);
        }

        std::chrono::system_clock::time_point value() const {
            return value_;
        }
        explicit operator std::chrono::system_clock::time_point() const {
            return value_;
        }

        std::string asString() const {
            if constexpr (std::is_same_v<TimeZoneSpec, time_zone_spec::Utc>) {
                return infra::withtime_utils::utcTimeString(value_);
            } else if constexpr (std::is_same_v<TimeZoneSpec, time_zone_spec::Local>) {
                return infra::withtime_utils::localTimeString(value_);
            } else {
                return infra::withtime_utils::zonedTimeString(value_, TimeZoneSpec::TheName);
            }
        }
    };

    template <typename TimeZoneSpec>
    class IsTimePointAsString<TimePointAsString<TimeZoneSpec>> {
    public:
        static constexpr bool Value = true;
    };

    template <typename TimeZoneSpec>
    class ConvertibleWithString<TimePointAsString<TimeZoneSpec>> {
    public:
        static constexpr bool value = true;
        static std::string toString(TimePointAsString<TimeZoneSpec> const &d) {
            return d.asString();
        }
        static TimePointAsString<TimeZoneSpec> fromString(std::string_view const &s) {
            if constexpr (std::is_same_v<TimeZoneSpec, time_zone_spec::Utc>) {
                return TimePointAsString<TimeZoneSpec> {infra::withtime_utils::parseUtcTime(s)};
            } else if constexpr (std::is_same_v<TimeZoneSpec, time_zone_spec::Local>) {
                return TimePointAsString<TimeZoneSpec> {infra::withtime_utils::parseLocalTime(s)};
            } else {
                return TimePointAsString<TimeZoneSpec> {infra::withtime_utils::parseZonedTime(s, TimeZoneSpec::TheName)};
            }
        }
    };

    class TimePointAsStringWithZoneOffset {
    private:
        std::chrono::system_clock::time_point value_;
    public:
        TimePointAsStringWithZoneOffset() : value_() {}
        explicit TimePointAsStringWithZoneOffset(std::chrono::system_clock::time_point const &tp) : value_(tp) {}
        ~TimePointAsStringWithZoneOffset() = default;

        TimePointAsStringWithZoneOffset(TimePointAsStringWithZoneOffset const &t) : value_(t.value_) {
        }
        TimePointAsStringWithZoneOffset &operator=(TimePointAsStringWithZoneOffset const &t) {
            if (this == &t) {
                return *this;
            }
            value_ = t.value_;
            return *this;
        }
        bool operator==(TimePointAsStringWithZoneOffset const &another) const {
            return (value_ == another.value_);
        }
        bool operator!=(TimePointAsStringWithZoneOffset const &another) const {
            return (value_ != another.value_);
        }
        bool operator>=(TimePointAsStringWithZoneOffset const &another) const {
            return (value_ >= another.value_);
        }
        bool operator<=(TimePointAsStringWithZoneOffset const &another) const {
            return (value_ <= another.value_);
        }
        bool operator>(TimePointAsStringWithZoneOffset const &another) const {
            return (value_ > another.value_);
        }
        bool operator<(TimePointAsStringWithZoneOffset const &another) const {
            return (value_ < another.value_);
        }

        std::chrono::system_clock::time_point value() const {
            return value_;
        }
        explicit operator std::chrono::system_clock::time_point() const {
            return value_;
        }

        std::string asString() const {
            auto s = infra::withtime_utils::utcTimeString(value_);
            return s.substr(0, s.length()-1)+"+00:00";
        }
    };

    template <>
    class ConvertibleWithString<TimePointAsStringWithZoneOffset> {
    public:
        static constexpr bool value = true;
        static std::string toString(TimePointAsStringWithZoneOffset const &d) {
            return d.asString();
        }
        static TimePointAsStringWithZoneOffset fromString(std::string_view const &s) {
            return TimePointAsStringWithZoneOffset {infra::withtime_utils::parseZonedTime(s)};
        }
    };
}}}}

#define TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS \
    ((typename, TimeZoneSpec)) 

TM_BASIC_TEMPLATE_PRINT_HELPER_THROUGH_STRING(TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS, dev::cd606::tm::basic::TimePointAsString);
TM_BASIC_CBOR_TEMPLATE_ENCDEC_THROUGH_STRING(TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS, dev::cd606::tm::basic::TimePointAsString);
TM_BASIC_PRINT_HELPER_THROUGH_STRING(dev::cd606::tm::basic::TimePointAsStringWithZoneOffset);
TM_BASIC_CBOR_ENCDEC_THROUGH_STRING(dev::cd606::tm::basic::TimePointAsStringWithZoneOffset);

#undef TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS

#endif