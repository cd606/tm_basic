#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_NAMED_FIELD_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_NAMED_FIELD_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/PrintHelper.hpp>
#include <tm_kit/basic/EqualityCheckHelper.hpp>
#include <tm_kit/basic/ConstStringType.hpp>
#include <string_view>
#include <tuple>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <typename Name, typename T>
    struct NamedItem;

    template <typename Lit, typename T>
    struct NamedItem<ConstStringType<Lit>, T> {
        static constexpr std::string_view NAME = ConstStringType<Lit>::VALUE;
        using value_type = T;
        using ValueType = T;
        T value;
        bool operator==(NamedItem<ConstStringType<Lit>, T> const &x) const {
            return EqualityCheckHelper<T>::check(value, x.value);
        }
        bool operator<(NamedItem<ConstStringType<Lit>, T> const &x) const {
            return ComparisonHelper<T>::check(value, x.value);
        }
    };

    template <typename Name, typename T>
    class StructFieldInfo<NamedItem<Name,T>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr bool EncDecWithFieldNames = false;
        static constexpr std::string_view REFERENCE_NAME = "NamedItem<>" ; \
        static constexpr std::array<std::string_view, 1> FIELD_NAMES = { NamedItem<Name,T>::NAME };
        static constexpr int getFieldIndex(std::string_view const &fieldName) {
            if (fieldName == NamedItem<Name,T>::NAME) {
                return 0;
            } else {
                return -1;
            }
        }
    };
    
    template <typename Name, typename T>
    class StructFieldTypeInfo<NamedItem<Name,T>, 0> {
    public:
        using TheType = T;
        static TheType &access(NamedItem<Name,T> &d) {
            return d.value;
        }
        static TheType const &constAccess(NamedItem<Name,T> const &d) {
            return d.value;
        }
        static TheType &&moveAccess(NamedItem<Name,T> &&d) {
            return std::move(d.value);
        }
    };

    template <typename Name, typename T>
    class PrintHelper<NamedItem<Name,T>> {
    public:
        static void print(std::ostream &os, NamedItem<Name,T> const &t) {
            os << NamedItem<Name,T>::NAME << "=";
            PrintHelper<T>::print(os, t.value);
        }
    };

    #define TM_BASIC_NAMED_ITEM_TYPE(name, t) dev::cd606::tm::basic::NamedItem<TM_BASIC_CONST_STRING_TYPE(name), t> 

    namespace bytedata_utils {
        template <typename Name, typename T>
        struct RunCBORSerializer<NamedItem<Name,T>> {
            static std::string apply(NamedItem<Name,T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
            static std::size_t apply(NamedItem<Name,T> const &data, char *output) {
                return RunCBORSerializer<T>::apply(data.value, output);
            }
            static std::size_t calculateSize(NamedItem<Name,T> const &data) {
                return RunCBORSerializer<T>::calculateSize(data.value);
            }
        };

        template <typename Name, typename T>
        struct RunCBORDeserializer<NamedItem<Name,T>> {
            static std::optional<std::tuple<NamedItem<Name,T>,size_t>> apply(std::string_view const &data, size_t start) {
                auto ret = RunCBORDeserializer<T>::apply(data, start);
                if (!ret) {
                    return std::nullopt;
                }
                return std::tuple<NamedItem<Name,T>,size_t> {
                    NamedItem<Name,T> {std::get<0>(*ret)}
                    , std::get<1>(*ret)
                };
            }
            static std::optional<size_t> applyInPlace(NamedItem<Name,T> &output, std::string_view const &data, size_t start) {
                return RunCBORDeserializer<T>::applyInPlace(output.value, data, start);
            }
        };

        template <class T, class Enable=void>
        class GetByName;

        template <class T>
        class GetByName<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
        public:
            template <class Name>
            static auto &get(T &t) {
                static_assert(StructFieldInfo<T>::getFieldIndex(Name::VALUE) >= 0);
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(Name::VALUE)>::access(t);
            }
            template <class Name>
            static auto const &get(T const &t) {
                static_assert(StructFieldInfo<T>::getFieldIndex(Name::VALUE) >= 0);
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(Name::VALUE)>::constAccess(t);
            } 
            template <class Name>
            static auto &&get(T &&t) {
                static_assert(StructFieldInfo<T>::getFieldIndex(Name::VALUE) >= 0);
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(Name::VALUE)>::moveAccess(std::move(t));
            } 
#if __cplusplus >= 202002L
            template <ConstStringType_StringLiteral lit>
            static auto &get(T &t) requires (StructFieldInfo<T>::getFieldIndex(lit.value) >= 0) {
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(lit.value)>::access(t);
            }
            template <ConstStringType_StringLiteral lit>
            static auto const &get(T const &t) requires (StructFieldInfo<T>::getFieldIndex(lit.value) >= 0) {
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(lit.value)>::constAccess(t);
            }
            template <ConstStringType_StringLiteral lit>
            static auto &&get(T &&t) requires (StructFieldInfo<T>::getFieldIndex(lit.value) >= 0) {
                return StructFieldTypeInfo<T, StructFieldInfo<T>::getFieldIndex(lit.value)>::moveAccess(std::move(t));
            }
#endif
        };

        #define TM_BASIC_GET_FIELD(x, f) \
            dev::cd606::tm::basic::bytedata_utils::GetByName<std::decay_t<decltype(x)>>::get<TM_BASIC_CONST_STRING_TYPE(BOOST_PP_STRINGIZE(f))>(x)
    }

    #define _TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(x) #x
    #define TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(x) _TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(x)
    #define TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_DEF(r, data, elem)    \
        dev::cd606::tm::basic::NamedItem<TM_BASIC_CONST_STRING_TYPE(TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_NAME(BOOST_PP_TUPLE_ELEM(1,elem))), BOOST_PP_TUPLE_ELEM(0,elem)> BOOST_PP_COMMA_IF(BOOST_PP_SUB(data, r))

    #define TM_BASIC_NAMED_TUPLE(name, content)                                                                 \
        using name = std::tuple<                                                                                \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_NAMED_TUPLE_FIELD_NAMED_ITEM_DEF,BOOST_PP_SEQ_SIZE(content),content) \
        >;
}}}}

#endif