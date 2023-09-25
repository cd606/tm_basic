#ifndef TM_KIT_BASIC_FIXED_PRECISION_SHORT_DECIMAL_HPP_
#define TM_KIT_BASIC_FIXED_PRECISION_SHORT_DECIMAL_HPP_

#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/EncodableThroughProxy.hpp>
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
    template <typename T>
    class IsFixedPrecisionShortDecimal {
    public:
        static constexpr bool value = false;
    };
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
        using UnderlyingType = Underlying;
        static constexpr std::size_t PrecisionValue = Precision;
        FixedPrecisionShortDecimal() : value_(0) {}
        explicit FixedPrecisionShortDecimal(Underlying u) : value_(u) {}
        explicit FixedPrecisionShortDecimal(double d) : value_(static_cast<Underlying>(std::llrint(d/s_conversionFactor))) {}
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
                    value_ = static_cast<Underlying>(std::llrint(d/s_conversionFactor));
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

        template <class T, typename=std::enable_if_t<std::is_integral_v<T>>>
        static FixedPrecisionShortDecimal fromIntegral(T u) {
            return FixedPrecisionShortDecimal(static_cast<Underlying>(u)*s_divisionFactor);
        }

        template <std::size_t NewPrecision, typename NewUnderlying=Underlying>
        FixedPrecisionShortDecimal<NewPrecision, NewUnderlying> changePrecision() const {
            if constexpr (NewPrecision == Precision) {
                if constexpr (std::is_same_v<NewUnderlying, Underlying>) {
                    return FixedPrecisionShortDecimal<NewPrecision, NewUnderlying>(value_);
                } else {
                    return FixedPrecisionShortDecimal<NewPrecision, NewUnderlying>(static_cast<NewUnderlying>(value_));
                }
            } else if constexpr (NewPrecision > Precision) {
                if constexpr (std::is_same_v<NewUnderlying, Underlying>) {
                    Underlying v = value_;
                    for (int ii=Precision; ii<NewPrecision; ++ii) {
                        v *= (Underlying) 10;
                    }
                    return FixedPrecisionShortDecimal<NewPrecision, Underlying>(v);
                } else {
                    NewUnderlying v = static_cast<NewUnderlying>(value_);
                    for (int ii=Precision; ii<NewPrecision; ++ii) {
                        v *= (NewUnderlying) 10;
                    }
                    return FixedPrecisionShortDecimal<NewPrecision, NewUnderlying>(v);
                }
            } else {
                if constexpr (std::is_same_v<NewUnderlying, Underlying>) {
                    Underlying factor = 1;
                    for (int ii=NewPrecision; ii<Precision; ++ii) {
                        factor *= (Underlying) 10;
                    }
                    if ((value_ % factor) >= factor/2) {
                        return FixedPrecisionShortDecimal<NewPrecision, Underlying>(value_/factor+1);
                    } else {
                        return FixedPrecisionShortDecimal<NewPrecision, Underlying>(value_/factor);
                    }
                } else {
                    NewUnderlying factor = 1;
                    for (int ii=NewPrecision; ii<Precision; ++ii) {
                        factor *= (NewUnderlying) 10;
                    }
                    NewUnderlying v = static_cast<NewUnderlying>(value_);
                    if ((v % factor) >= factor/2) {
                        return FixedPrecisionShortDecimal<NewPrecision, NewUnderlying>(v/factor+1);
                    } else {
                        return FixedPrecisionShortDecimal<NewPrecision, NewUnderlying>(v/factor);
                    }
                }
            }
        }

        template <std::size_t AnotherPrecision, typename AnotherUnderlying, typename=std::enable_if_t<
            AnotherPrecision != Precision
            ||
            !std::is_same_v<AnotherUnderlying, Underlying>
        >>
        explicit operator FixedPrecisionShortDecimal<AnotherPrecision, AnotherUnderlying>() const {
            return changePrecision<AnotherPrecision, AnotherUnderlying>();
        }

        template <class I, typename=std::enable_if_t<std::is_integral_v<I>>>
        static FixedPrecisionShortDecimal fromPrecisionAndValue(std::size_t inputPrecision, I inputValue) {
            if (Precision == inputPrecision) {
                return FixedPrecisionShortDecimal {static_cast<Underlying>(inputValue)};
            } else if (Precision > inputPrecision) {
                Underlying v = static_cast<Underlying>(inputValue);
                for (int ii=inputPrecision; ii<Precision; ++ii) {
                    v *= (Underlying) 10;
                }
                return FixedPrecisionShortDecimal(v);
            } else {
                Underlying factor = 1;
                for (int ii=Precision; ii<inputPrecision; ++ii) {
                    factor *= (Underlying) 10;
                }
                Underlying v = static_cast<Underlying>(inputValue);
                if ((v % factor) >= factor/2) {
                    return FixedPrecisionShortDecimal(v/factor+1);
                } else {
                    return FixedPrecisionShortDecimal(v/factor);
                }
            }
        }

        static constexpr FixedPrecisionShortDecimal zero() {
            return FixedPrecisionShortDecimal();
        }
        static constexpr FixedPrecisionShortDecimal max() {
            return FixedPrecisionShortDecimal(std::numeric_limits<Underlying>::max());
        }
        static constexpr FixedPrecisionShortDecimal min() {
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

        FixedPrecisionShortDecimal abs() const {
            if constexpr (std::is_signed_v<Underlying>) {
                return FixedPrecisionShortDecimal {(value_>=0)?value_:(-value_)};
            } else {
                return *this;
            }
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
        explicit operator std::string() const {
            return asString();
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

    template <std::size_t Precision, typename Underlying>
    class IsFixedPrecisionShortDecimal<FixedPrecisionShortDecimal<Precision, Underlying>> {
    public:
        static constexpr bool value = true;
        using TheType = FixedPrecisionShortDecimal<Precision, Underlying>;
        static constexpr std::size_t precision = Precision;
        using UnderlyingType = Underlying;
    };

    template <std::size_t Precision, typename Underlying>
    class EncodableThroughProxy<FixedPrecisionShortDecimal<Precision, Underlying>> {
    public:
        static constexpr bool value = true;
        using EncodeProxyType = double;
        using DecodeProxyType = double;
        static EncodeProxyType toProxy(FixedPrecisionShortDecimal<Precision, Underlying> const &v) {
            return (double) v;
        }
        static FixedPrecisionShortDecimal<Precision, Underlying> fromProxy(DecodeProxyType const &v) {
            return FixedPrecisionShortDecimal<Precision, Underlying> {v};
        }
    };

    template <std::size_t Precision, typename Underlying>
    class EncodableThroughMultipleProxies<FixedPrecisionShortDecimal<Precision, Underlying>> {
    public:
        static constexpr bool value = true;
        using ProxyTypes = std::variant<
            std::tuple<std::size_t, Underlying>
            , double
            , std::string
        >;
        static std::tuple<std::size_t, Underlying> toProxy(FixedPrecisionShortDecimal<Precision, Underlying> const &v) {
            return {Precision, v.value()};
        }
        static FixedPrecisionShortDecimal<Precision, Underlying> fromProxy(ProxyTypes const &v) {
            return std::visit([](auto const &x) -> FixedPrecisionShortDecimal<Precision, Underlying> {
                using V = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<V, std::tuple<std::size_t, Underlying>>) {
                    return FixedPrecisionShortDecimal<Precision, Underlying>::fromPrecisionAndValue(
                        std::get<0>(x), std::get<1>(x)
                    );
                } else if constexpr (std::is_same_v<V, double>) {
                    return FixedPrecisionShortDecimal<Precision, Underlying> {x};
                } else if constexpr (std::is_same_v<V, std::string>) {
                    return FixedPrecisionShortDecimal<Precision, Underlying> {x};
                } else {
                    return FixedPrecisionShortDecimal<Precision, Underlying>::zero();
                }
            }, v);
        }
    };
}}}}

#define TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS \
    ((std::size_t, Precision)) \
    ((typename, Underlying))

TM_BASIC_TEMPLATE_PRINT_HELPER_THROUGH_STRING(TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS, dev::cd606::tm::basic::FixedPrecisionShortDecimal);
TM_BASIC_CBOR_TEMPLATE_ENCDEC_THROUGH_PROXY(TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS, dev::cd606::tm::basic::FixedPrecisionShortDecimal);

#undef TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS

#endif
