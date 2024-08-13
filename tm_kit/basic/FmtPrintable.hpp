#ifndef TM_KIT_BASIC_FMT_PRINTABLE_HPP_
#define TM_KIT_BASIC_FMT_PRINTABLE_HPP_

#include <chrono>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <variant>

#include <fmt/format.h>
#include <tm_kit/basic/PrintHelper.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <typename T>
    struct FmtPrintable {
        const T &value;

        std::string toString() const
        {
            std::ostringstream oss;
            PrintHelper<T>::print(oss, value);
            return oss.str();
        }
    };

    template <typename T>
    FmtPrintable(T &&)->FmtPrintable<std::decay_t<T>>;
} } } }

template <typename T>
struct fmt::formatter<dev::cd606::tm::basic::FmtPrintable<T>> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(dev::cd606::tm::basic::FmtPrintable<T> const &c, FormatContext &ctx) const
    {
        return fmt::formatter<std::string_view>::format(c.toString(), ctx);
    }
};

#define TM_BASIC_FMT_PRINTABLE(x)                                                                                                                                                                               \
    dev::cd606::tm::basic::FmtPrintable<std::decay_t<decltype(x)>>                                                                                                                                     \
    {                                                                                                                                                                                                  \
        x                                                                                                                                                                                              \
    }

#endif