#ifndef TM_KIT_BASIC_FIXED_PRECISION_SHORT_DECIMAL_HPP_
#define TM_KIT_BASIC_FIXED_PRECISION_SHORT_DECIMAL_HPP_

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
    template <std::size_t Precision, typename Underlying=int64_t>
    class FixedPrecisionShortDecimal {
    private:
        Underlying value_;
        static constexpr double conversionFactor() {
            double ret = 1.0;
            for (std::size_t x = 0; x < Precision; ++x) {
                ret *= 0.1;
            }
            return ret;
        }
        static constexpr double s_conversionFactor = conversionFactor();
        static constexpr Underlying divisionFactor() {
            Underlying ret = 1;
            for (std::size_t x = 0; x < Precision; ++x) {
                ret *= 10;
            }
            return ret;
        }
        static constexpr Underlying s_divisionFactor = divisionFactor();
    public:
        FixedPrecisionShortDecimal() : value_(0) {}
        explicit FixedPrecisionShortDecimal(Underlying u) : value_(u) {}
        explicit FixedPrecisionShortDecimal(double d) : value_(std::round(d/s_conversionFactor)) {}
        explicit FixedPrecisionShortDecimal(std::string_view const &s) : value_(0) {
            auto pos = s.find_first_of('.');
            if (pos == std::string::npos) {
                try {
                    value_ = boost::lexical_cast<Underlying>(s)*s_divisionFactor;
                } catch (boost::bad_lexical_cast const &) {
                    value_ = 0;
                }
            } else {
                try {
                    double d = boost::lexical_cast<double>(s);
                    value_ = static_cast<Underlying>(std::round(d/s_conversionFactor));
                } catch (boost::bad_lexical_cast const &) {
                    value_ = 0;
                }
            }
        }
        FixedPrecisionShortDecimal(FixedPrecisionShortDecimal const &) = default;
        FixedPrecisionShortDecimal(FixedPrecisionShortDecimal &&) = default;
        FixedPrecisionShortDecimal &operator=(FixedPrecisionShortDecimal const &) = default;
        FixedPrecisionShortDecimal &operator=(FixedPrecisionShortDecimal &&) = default;
        ~FixedPrecisionShortDecimal() = default;

        static FixedPrecisionShortDecimal zero() {
            return FixedPrecisionShortDecimal();
        }
        static FixedPrecisionShortDecimal max() {
            return FixedPrecisionShortDecimal(std::numeric_limits<Underlying>::max());
        }
        static FixedPrecisionShortDecimal min() {
            return FixedPrecisionShortDecimal(std::numeric_limits<Underlying>::min());
        }

        bool operator==(FixedPrecisionShortDecimal const &another) const {
            return (value_ == another.value_);
        }
        bool operator!=(FixedPrecisionShortDecimal const &another) const {
            return (value_ != another.value_);
        }
        bool operator>=(FixedPrecisionShortDecimal const &another) const {
            return (value_ >= another.value_);
        }
        bool operator<=(FixedPrecisionShortDecimal const &another) const {
            return (value_ <= another.value_);
        }
        bool operator>(FixedPrecisionShortDecimal const &another) const {
            return (value_ > another.value_);
        }
        bool operator<(FixedPrecisionShortDecimal const &another) const {
            return (value_ < another.value_);
        }

        FixedPrecisionShortDecimal operator-() const {
            if constexpr (std::is_signed_v<Underlying>) {
                return FixedPrecisionShortDecimal {-value_};
            } else {
                throw std::runtime_error("Trying to take negative on an unsigned value");
                return FixedPrecisionShortDecimal {};
            }
        }
        FixedPrecisionShortDecimal operator+(FixedPrecisionShortDecimal const &another) const {
            return FixedPrecisionShortDecimal {value_+another.value_};
        }
        FixedPrecisionShortDecimal operator-(FixedPrecisionShortDecimal const &another) const {
            return FixedPrecisionShortDecimal {value_-another.value_};
        }
        FixedPrecisionShortDecimal &operator+=(FixedPrecisionShortDecimal const &another) {
            value_ += another.value_;
            return *this;
        }
        FixedPrecisionShortDecimal &operator-=(FixedPrecisionShortDecimal const &another) {
            value_ -= another.value_;
            return *this;
        }

        Underlying value() const {
            return value_;
        }
        explicit operator double() const {
            return s_conversionFactor*value_;
        }
        explicit operator float() const {
            return ((float) s_conversionFactor)*((float) value_);
        }
        std::string asString() const {
            std::ostringstream oss;
            oss << value_/s_divisionFactor;
            if constexpr (std::is_unsigned_v<Underlying>) {
                Underlying d = value_%s_divisionFactor;
                if (d != 0) {
                    std::size_t length = Precision;
                    while (d%10 == 0) {
                        d /= 10;
                        --length;
                    }
                    oss << '.' << std::setw(length) << std::setfill('0') << d;
                }
                return oss.str();
            } else {
                Underlying d = ((value_>=0)?value_:-value_)%s_divisionFactor;
                if (d != 0) {
                    std::size_t length = Precision;
                    while (d%10 == 0) {
                        d /= 10;
                        --length;
                    }
                    oss << '.' << std::setw(length) << std::setfill('0') << d;
                }
                return oss.str();
            }
        }
    };

    template <std::size_t Precision, typename Underlying>
    class ConvertibleWithString<FixedPrecisionShortDecimal<Precision,Underlying>> {
    public:
        static constexpr bool value = true;
        static std::string toString(FixedPrecisionShortDecimal<Precision,Underlying> const &d) {
            return d.asString();
        }
        static FixedPrecisionShortDecimal<Precision,Underlying> fromString(std::string_view const &s) {
            return FixedPrecisionShortDecimal<Precision,Underlying> {s};
        }
    };
}}}}

#define TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS \
    ((std::size_t, Precision)) \
    ((typename, Underlying))

TM_BASIC_TEMPLATE_PRINT_HELPER_THROUGH_STRING(TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS, dev::cd606::tm::basic::FixedPrecisionShortDecimal);
TM_BASIC_CBOR_TEMPLATE_ENCDEC_THROUGH_STRING(TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS, dev::cd606::tm::basic::FixedPrecisionShortDecimal);

#undef TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS

#endif