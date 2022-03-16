#ifndef TM_KIT_BASIC_TIME_POINT_AS_STRING_HPP_
#define TM_KIT_BASIC_TIME_POINT_AS_STRING_HPP_

#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/SerializationHelperMacros_Proxy.hpp>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cmath>
#include <type_traits>

#include <stdint.h>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    enum class TimePointAsStringChoice {
        Local, Utc
    };
    template <TimePointAsStringChoice Choice=TimePointAsStringChoice::Local>
    class TimePointAsString {
    private:
        std::chrono::system_clock::time_point value_;
    public:
        TimePointAsString() : value_() {}
        explicit TimePointAsString(std::chrono::system_clock::time_point const &tp) : value_(tp) {}
        TimePointAsString(TimePointAsString const &) = default;
        TimePointAsString &operator=(TimePointAsString const &) = default;
        TimePointAsString(TimePointAsString &&) = default;
        TimePointAsString &operator=(TimePointAsString &&) = default;
        ~TimePointAsString() = default;
        
        bool operator==(TimePointAsString<Choice> const &another) const {
            return (value_ == another.value_);
        }
        bool operator!=(TimePointAsString<Choice> const &another) const {
            return (value_ != another.value_);
        }
        bool operator>=(TimePointAsString<Choice> const &another) const {
            return (value_ >= another.value_);
        }
        bool operator<=(TimePointAsString<Choice> const &another) const {
            return (value_ <= another.value_);
        }
        bool operator>(TimePointAsString<Choice> const &another) const {
            return (value_ > another.value_);
        }
        bool operator<(TimePointAsString<Choice> const &another) const {
            return (value_ < another.value_);
        }

        std::chrono::system_clock::time_point value() const {
            return value_;
        }
        explicit operator std::chrono::system_clock::time_point() const {
            return value_;
        }

        std::string asString() const {
            return infra::withtime_utils::utcTimeString(value_);
        }
    };

    template <TimePointAsStringChoice Choice>
    class ConvertibleWithString<TimePointAsString<Choice>> {
    public:
        static constexpr bool value = true;
        static std::string toString(TimePointAsString<Choice> const &d) {
            return d.asString();
        }
        static TimePointAsString<Choice> fromString(std::string_view const &s) {
            return TimePointAsString<Choice> {infra::withtime_utils::parseUtcTime(s)};
        }
    };
}}}}

#define TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS \
    ((dev::cd606::tm::basic::TimePointAsStringChoice, Choice)) 

TM_BASIC_TEMPLATE_PRINT_HELPER_THROUGH_STRING(TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS, dev::cd606::tm::basic::TimePointAsString);
TM_BASIC_CBOR_TEMPLATE_ENCDEC_THROUGH_STRING(TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS, dev::cd606::tm::basic::TimePointAsString);

#undef TM_BASIC_TIME_POINT_AS_STRING_TEMPLATE_ARGS

#endif