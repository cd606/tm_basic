#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_NAMED_FIELD_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_NAMED_FIELD_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/PrintHelper.hpp>
#include <string_view>
#include <tuple>

namespace dev { namespace cd606 { namespace tm { namespace basic {
#if __cplusplus >= 202002L
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

        template <typename T>
        concept HasStructFieldInfo = StructFieldInfo<T>::HasGeneratedStructFieldInfo;

        template <class T> requires HasStructFieldInfo<T>
        class GetByName {
        public:
            template <struct_field_info_helper_internal::StringLiteral lit> 
                requires (StructFieldInfo<T>::getFieldIndex(lit.value) >= 0)
            static auto &get(T &t) {
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(lit.value)>::access(t);
            }
            template <struct_field_info_helper_internal::StringLiteral lit> 
                requires (StructFieldInfo<T>::getFieldIndex(lit.value) >= 0)
            static auto const &get(T const &t) {
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(lit.value)>::constAccess(t);
            }
            template <struct_field_info_helper_internal::StringLiteral lit> 
                requires (StructFieldInfo<T>::getFieldIndex(lit.value) >= 0)
            static auto &&get(T &&t) {
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(lit.value)>::moveAccess(std::move(t));
            }
        };

        #define TM_BASIC_GET_FIELD(x, f) \
            dev::cd606::tm::basic::bytedata_utils::GetByName<std::decay_t<decltype(x)>>::get<#f>(x)
    }

    #define _TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(x) #x
    #define TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(x) _TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(x)
    #define TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_DEF(r, data, elem)    \
        dev::cd606::tm::basic::NamedItem<TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(BOOST_PP_TUPLE_ELEM(1,elem)), BOOST_PP_TUPLE_ELEM(0,elem)> BOOST_PP_COMMA_IF(BOOST_PP_SUB(data, r))

    #define TM_BASIC_NAMED_TUPLE(name, content)                                                                 \
        using name = std::tuple<                                                                                \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_DEF,BOOST_PP_SEQ_SIZE(content),content) \
        >;

#else
    namespace struct_field_info_helper_internal {
        struct CompileString {};

        template <std::size_t N>
        inline constexpr auto toStringView(const char (&s)[N]) {
            return std::string_view{s, N - (s[N - 1] == '\0' ? 1 : 0)};
        }

        template <typename T>
        inline constexpr bool is_compile_string_v = std::is_base_of_v<CompileString, T>;
    }

    #define TM_BASIC_COMPILE_STRING(x) []() {                                                           \
        struct Str : public dev::cd606::tm::basic::struct_field_info_helper_internal::CompileString {   \
            constexpr operator std::string_view() const noexcept {                                      \
                return dev::cd606::tm::basic::struct_field_info_helper_internal::toStringView(x);       \
            }                                                                                           \
        };                                                                                              \
        return Str{};                                                                                   \
    }()

    template <typename S, typename T, std::enable_if_t<struct_field_info_helper_internal::is_compile_string_v<S>, int> = 0>
    struct NamedItem {
        static constexpr std::string_view NAME = S{};
        using value_type = T;
        using ValueType = T;
        T value;
    };

    template <typename S, typename T>
    class StructFieldInfo<NamedItem<S,T>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr std::array<std::string_view, 1> FIELD_NAMES = { NamedItem<S,T>::NAME };
        static constexpr int getFieldIndex(std::string_view const &fieldName) {
            if (fieldName == NamedItem<S,T>::NAME) {
                return 0;
            } else {
                return -1;
            }
        }
    };

    template <typename S, typename T>
    class StructFieldTypeInfo<NamedItem<S,T>, 0> {
    public:
        using TheType = T;
        static TheType &access(NamedItem<S,T> &d) {
            return d.value;
        }
        static TheType const &constAccess(NamedItem<S,T> const &d) {
            return d.value;
        }
        static TheType &&moveAccess(NamedItem<S,T> &&d) {
            return std::move(d.value);
        }
    };

    template <typename S, typename T>
    class PrintHelper<NamedItem<S,T>> {
    public:
        static void print(std::ostream &os, NamedItem<S,T> const &t) {
            os << NamedItem<S,T>::NAME << "=";
            PrintHelper<T>::print(os, t.value);
        }
    };

    namespace bytedata_utils {
        template <typename S, typename T>
        struct RunCBORSerializer<NamedItem<S,T>> {
            static std::string apply(NamedItem<S,T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
            static std::size_t apply(NamedItem<S,T> const &data, char *output) {
                return RunCBORSerializer<T>::apply(data.value, output);
            }
            static std::size_t calculateSize(NamedItem<S,T> const &data) {
                return RunCBORSerializer<T>::calculateSize(data.value);
            }
        };

        template <typename S, typename T>
        struct RunCBORDeserializer<NamedItem<S,T>> {
            static std::optional<std::tuple<NamedItem<S,T>,size_t>> apply(std::string_view const &data, size_t start) {
                auto ret = RunCBORDeserializer<T>::apply(data, start);
                if (!ret) {
                    return std::nullopt;
                }
                return std::tuple<NamedItem<S,T>,size_t> {
                    NamedItem<S,T> {std::get<0>(*ret)}
                    , std::get<1>(*ret)
                };
            }
            static std::optional<size_t> applyInPlace(NamedItem<S,T> &output, std::string_view const &data, size_t start) {
                return RunCBORDeserializer<T>::applyInPlace(output.value, data, start);
            }
        };

        template <typename T>
        inline constexpr bool HasStructFieldInfo = StructFieldInfo<T>::HasGeneratedStructFieldInfo;

        template <class T, std::enable_if_t<HasStructFieldInfo<T>, int> = 0>
        class GetByName {
        public:
            template <typename S>
            static auto &get(T &t, std::enable_if_t<struct_field_info_helper_internal::is_compile_string_v<S>, S> f) {
                if constexpr (StructFieldInfo<T>::getFieldIndex(f) >= 0) {
                    return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(f)>::access(t);
                } else {
                    static_assert(sizeof(S*) < 0, "field index must be positive");
                }
            }
            template <typename S>
            static auto const &get(T const &t, std::enable_if_t<struct_field_info_helper_internal::is_compile_string_v<S>, S> f) {
                if constexpr (StructFieldInfo<T>::getFieldIndex(f) >= 0) {
                    return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(f)>::constAccess(t);
                } else {
                    static_assert(sizeof(S*) < 0, "field index must be positive");
                }
            }
            template <typename S>
            static auto &&get(T &&t, std::enable_if_t<struct_field_info_helper_internal::is_compile_string_v<S>, S> f) {
                if constexpr (StructFieldInfo<T>::getFieldIndex(f) >= 0) {
                    return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(f)>::moveAccess(std::move(t));
                } else {
                    static_assert(sizeof(S*) < 0, "field index must be positive");
                }
            }
        };

        #define TM_BASIC_GET_FIELD(x, f) \
            dev::cd606::tm::basic::bytedata_utils::GetByName<std::decay_t<decltype(x)>>::get(x, TM_BASIC_COMPILE_STRING(#f))
        
        #define _TM_BASIC_NAMED_TUPLE_FIELD_NAME_ARG(n) __compile_str_##n
        #define TM_BASIC_NAMED_TUPLE_FIELD_NAME_ARG(n) _TM_BASIC_NAMED_TUPLE_FIELD_NAME_ARG(n)
        #define _TM_BASIC_NAMED_TUPLE_FIELD_NAME_COMPILE_STRING(n) TM_BASIC_COMPILE_STRING(#n)
        #define TM_BASIC_NAMED_TUPLE_FIELD_NAME_COMPILE_STRING(n) _TM_BASIC_NAMED_TUPLE_FIELD_NAME_COMPILE_STRING(n)
        #define TM_BASIC_NAMED_TUPLE_FIELD_NAME_DEF(r, data, elem)  \
            static constexpr auto TM_BASIC_NAMED_TUPLE_FIELD_NAME_ARG(BOOST_PP_TUPLE_ELEM(1,elem)) = TM_BASIC_NAMED_TUPLE_FIELD_NAME_COMPILE_STRING(BOOST_PP_TUPLE_ELEM(1,elem));

        #define TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_DEF(r, data, elem)    \
            dev::cd606::tm::basic::NamedItem<decltype(TM_BASIC_NAMED_TUPLE_FIELD_NAME_ARG(BOOST_PP_TUPLE_ELEM(1,elem))), BOOST_PP_TUPLE_ELEM(0,elem)> BOOST_PP_COMMA_IF(BOOST_PP_SUB(data, r))

        #define TM_BASIC_NAMED_TUPLE(name, content)                                         \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_NAMED_TUPLE_FIELD_NAME_DEF,_,content)            \
            using name = std::tuple<                                                        \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_DEF,BOOST_PP_SEQ_SIZE(content),content)  \
            >;
    }

#endif
}}}}

#endif