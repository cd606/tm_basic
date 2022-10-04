#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_NAMED_FIELD_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_NAMED_FIELD_HPP_

#if __cplusplus >= 202002L

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/PrintHelper.hpp>
#include <string_view>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    namespace struct_field_info_helper_internal {
        template<size_t N>
        struct StringLiteral {
            constexpr StringLiteral(const char (&str)[N]) {
                std::copy_n(str, N, value);
            }
            
            char value[N];
        };
    }

    template <struct_field_info_helper_internal::StringLiteral lit, typename T>
    struct NamedItem {
        static constexpr std::string_view NAME = lit.value;
        using value_type = T;
        using ValueType = T;
        T value;
    };

    template <struct_field_info_helper_internal::StringLiteral lit, typename T>
    class StructFieldInfo<NamedItem<lit,T>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr std::array<std::string_view, 1> FIELD_NAMES = { lit.value };
        static constexpr int getFieldIndex(std::string_view const &fieldName) {
            if (fieldName == lit.value) {
                return 0;
            } else {
                return -1;
            }
        }
    };

    
    template <struct_field_info_helper_internal::StringLiteral lit, typename T>
    class StructFieldTypeInfo<NamedItem<lit,T>, 0> {
    public:
        using TheType = T;
        static TheType &access(NamedItem<lit,T> &d) {
            return d.value;
        }
        static TheType const &constAccess(NamedItem<lit,T> const &d) {
            return d.value;
        }
        static TheType &&moveAccess(NamedItem<lit,T> &&d) {
            return std::move(d.value);
        }
    };

    template <struct_field_info_helper_internal::StringLiteral lit, typename T>
    class PrintHelper<NamedItem<lit,T>> {
    public:
        static void print(std::ostream &os, NamedItem<lit,T> const &t) {
            os << lit.value << "=";
            PrintHelper<T>::print(os, t.value);
        }
    };

    namespace bytedata_utils {
        template <struct_field_info_helper_internal::StringLiteral lit, typename T>
        struct RunCBORSerializer<NamedItem<lit,T>> {
            static std::string apply(NamedItem<lit,T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
            static std::size_t apply(NamedItem<lit,T> const &data, char *output) {
                return RunCBORSerializer<T>::apply(data.value, output);
            }
            static std::size_t calculateSize(NamedItem<lit,T> const &data) {
                return RunCBORSerializer<T>::calculateSize(data.value);
            }
        };

        template <struct_field_info_helper_internal::StringLiteral lit, typename T>
        struct RunCBORDeserializer<NamedItem<lit,T>> {
            static std::optional<std::tuple<NamedItem<lit,T>,size_t>> apply(std::string_view const &data, size_t start) {
                auto ret = RunCBORDeserializer<T>::apply(data, start);
                if (!ret) {
                    return std::nullopt;
                }
                return std::tuple<NamedItem<lit,T>,size_t> {
                    NamedItem<lit,T> {std::get<0>(*ret)}
                    , std::get<1>(*ret)
                };
            }
            static std::optional<size_t> applyInPlace(NamedItem<lit,T> &output, std::string_view const &data, size_t start) {
                return RunCBORDeserializer<T>::applyInPlace(output.value, data, start);
            }
        };
    }

}}}}
#endif

#endif