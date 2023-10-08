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

        static Underlying parseMantissa(std::string_view const &s, short e) {
            auto pos = s.find_first_of('.');
            auto e1 = (e>0?e:0);
            Underlying f = s_divisionFactor;
            if (e1 > 0) {
                auto e2 = e1;
                Underlying d = 10;
                while (e2 > 0) {
                    if ((e2 & 1) == 1) {
                        f *= d;
                    }
                    d *= d;
                    e2 >>= 1;
                }
            }
            if (pos == std::string::npos) {
                try {
                    return boost::lexical_cast<Underlying>(s)*f;
                } catch (boost::bad_lexical_cast const &) {
                    return 0;
                }
            } else {
                try {
                    Underlying integerPart = boost::lexical_cast<Underlying>(s.substr(0, pos));
                    if (pos+1 < s.length()) {
                        Underlying fractionPart = 0;
                        for (std::size_t ii=0; ii<Precision+e1; ++ii) {
                            fractionPart *= 10;
                            if (pos+ii+1 < s.length()) {
                                if (s[pos+ii+1] < '0' || s[pos+ii+1] > '9') {
                                    return 0;
                                }
                                fractionPart += (Underlying) (s[pos+ii+1]-'0');
                            }
                        }
                        if (pos+Precision+e1+1 < s.length()) {
                            if (s[pos+Precision+e1+1] >= '5' && s[pos+Precision+e1+1] <= '9') {
                                fractionPart += 1;
                            } else if (s[pos+Precision+e1+1] >= '0' && s[pos+Precision+e1+1] <= '4') {
                                //do nothing
                            } else {
                                return 0;
                            }
                        }
                        if constexpr (std::is_signed_v<Underlying>) {
                            if (integerPart >= 0) {
                                return (integerPart*f)+fractionPart;
                            } else {
                                return -((-integerPart)*f+fractionPart);
                            }
                        } else {
                            return (integerPart*f)+fractionPart;
                        }
                    } else {
                        return integerPart*f;
                    }
                } catch (boost::bad_lexical_cast const &) {
                    return 0;
                }
            }
        }

        static Underlying combineFP(Underlying mantissa, short exp) {
            if (mantissa == 0) {
                return 0;
            }
            if (exp >= 0) {
                return mantissa;
            }
            if constexpr (std::is_signed_v<Underlying>) {
                if (mantissa < 0) {
                    return -combineFP(-mantissa, exp);
                }
            }
            short e1 = -exp;
            Underlying d = 10;
            Underlying factor = 1;
            while (e1 > 0) {
                if ((e1 & 1) == 1) {
                    factor *= d;
                }
                e1 >>= 1;
                d *= d;
            }
            if (mantissa%factor >= factor/2) {
                return (mantissa/factor+1);
            } else {
                return (mantissa/factor);
            }
        }

        static Underlying parseString(std::string_view const &s) {
            try {
                auto p = s.find_first_of('e');
                if (p != std::string::npos) {
                    auto e = boost::lexical_cast<short>(s.substr(p+1));
                    return combineFP(parseMantissa(s.substr(0, p), e), e);
                } 
                p = s.find_first_of('E');
                if (p != std::string::npos) {
                    auto e = boost::lexical_cast<short>(s.substr(p+1));
                    return combineFP(parseMantissa(s.substr(0, p), e), e);
                } 
                return combineFP(parseMantissa(s, 0), 0);
            } catch (boost::bad_lexical_cast const &) {
                return 0;
            }
        }
    public:
        using UnderlyingType = Underlying;
        static constexpr std::size_t PrecisionValue = Precision;
        FixedPrecisionShortDecimal() : value_(0) {}
        explicit FixedPrecisionShortDecimal(Underlying u) : value_(u) {}
        explicit FixedPrecisionShortDecimal(double d) : value_(static_cast<Underlying>(std::llrint(d/s_conversionFactor))) {}
        explicit FixedPrecisionShortDecimal(std::string_view const &s) : value_(parseString(s)) {
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
    class EncodableThroughMultipleProxies<FixedPrecisionShortDecimal<Precision, Underlying>> {
    public:
        static constexpr bool value = true;
        using CBOREncodeProxyType = std::tuple<std::size_t, Underlying>;
        using JSONEncodeProxyType = std::string;
        using ProtoEncodeProxyType = double;
        using DecodeProxyTypes = std::variant<
            std::tuple<std::size_t, Underlying>
            , std::string
            , double
        >;
        static std::tuple<std::size_t, Underlying> toCBOREncodeProxy(FixedPrecisionShortDecimal<Precision, Underlying> const &v) {
            return {Precision, v.value()};
        }
        static std::string toJSONEncodeProxy(FixedPrecisionShortDecimal<Precision, Underlying> const &v) {
            return v.asString();
        }
        static double toProtoEncodeProxy(FixedPrecisionShortDecimal<Precision, Underlying> const &v) {
            return (double) v;
        } 
        static FixedPrecisionShortDecimal<Precision, Underlying> fromProxy(DecodeProxyTypes const &v) {
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
TM_BASIC_CBOR_TEMPLATE_ENCDEC_THROUGH_MULTIPLE_PROXIES(TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS, dev::cd606::tm::basic::FixedPrecisionShortDecimal);

#undef TM_BASIC_FIXED_PRECISION_SHORT_DECIMAL_TEMPLATE_ARGS

#endif
