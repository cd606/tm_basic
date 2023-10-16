#ifndef SERIALIZATION_HELPER_MACROS_HPP_
#define SERIALIZATION_HELPER_MACROS_HPP_

#include <array>
#include <string_view>

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>

#include <boost/version.hpp>

#include <tm_kit/basic/EqualityCheckHelper.hpp>
#include <tm_kit/basic/PrintHelper.hpp>
#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/EncodableThroughProxy.hpp>

#if BOOST_VERSION >= 107500
    #ifdef _MSC_VER
    #define TM_SERIALIZATION_HELPER_COMMA_START_POS 2
    #else
    #define TM_SERIALIZATION_HELPER_COMMA_START_POS 1
    #endif
#else
    #define TM_SERIALIZATION_HELPER_COMMA_START_POS 2
#endif

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils {
    //This implementation of type with parenthesis is from
    // https://stackoverflow.com/questions/13842468/comma-in-c-c-macro
    template <typename T> struct macro_type_resolver {};
    template <typename T, typename U> struct macro_type_resolver<T(U)> { using type_name = U; };
} } } } }

#ifdef _MSC_VER
    //The reason why we need "PROTECT_TYPE" is also explained on 
    // https://stackoverflow.com/questions/13842468/comma-in-c-c-macro
    #define TM_BASIC_CBOR_CAPABLE_STRUCT_PROTECT_TYPE(...) \
        typename dev::cd606::tm::basic::bytedata_utils::macro_type_resolver<void(__VA_ARGS__)>::type_name 
    #define TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(x) \
        x
#else
    #define TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(x) \
        typename dev::cd606::tm::basic::bytedata_utils::macro_type_resolver<void(x)>::type_name
#endif

#define TM_BASIC_CBOR_CAPABLE_STRUCT_ITEM_DEF(r, data, elem) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem)) BOOST_PP_TUPLE_ELEM(1,elem);

#define TM_BASIC_CBOR_CAPABLE_STRUCT_DEF(name, content) \
    struct name { \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_ITEM_DEF,_,content) \
    };
#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_DEF(name) \
    struct name {};

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(0,elem) BOOST_PP_TUPLE_ELEM(1,elem)
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams) \
    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST_ITEM,_,templateParams)
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(1,elem)
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams) \
    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST_ITEM,_,templateParams)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_DEF(templateParams, name, content) \
    template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
    struct name { \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_ITEM_DEF,_,content) \
    };
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_DEF(templateParams, name) \
    template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
    struct name {};

#define TM_BASIC_CBOR_CAPABLE_STRUCT_PRINT_ITEM(r, data, elem) \
    os << BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1,elem)) << '='; \
    dev::cd606::tm::basic::PrintHelper<TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem))>::print(os, x.BOOST_PP_TUPLE_ELEM(1,elem)); \
    os << ' ';    

#define TM_BASIC_CBOR_CAPABLE_STRUCT_EQ_ITEM(r, data, elem) \
    && dev::cd606::tm::basic::EqualityCheckHelper<TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem))>::check(x.BOOST_PP_TUPLE_ELEM(1,elem), y.BOOST_PP_TUPLE_ELEM(1,elem)) 

#define TM_BASIC_CBOR_CAPABLE_STRUCT_PRINT(name, content) \
    inline std::ostream &operator<<(std::ostream &os, name const &x) { \
        os << BOOST_PP_STRINGIZE(name) << '{'; \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_PRINT_ITEM,_,content) \
        os << '}'; \
        return os; \
    }
#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_PRINT(name) \
    inline std::ostream &operator<<(std::ostream &os, name const &x) { \
        os << BOOST_PP_STRINGIZE(name) << "{}"; \
        return os; \
    }
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_PRINT(templateParams, name, content) \
    template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
    inline std::ostream &operator<<(std::ostream &os, name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
        os << BOOST_PP_STRINGIZE(name) << '{'; \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_PRINT_ITEM,_,content) \
        os << '}'; \
        return os; \
    }
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_PRINT(templateParams, name) \
    template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
    inline std::ostream &operator<<(std::ostream &os, name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
        os << BOOST_PP_STRINGIZE(name) << "{}"; \
        return os; \
    }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_EQ(name, content) \
    inline bool operator==(name const &x, name const &y) { \
        return (true \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EQ_ITEM,_,content) \
            ); \
    }
#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_EQ(name) \
    inline bool operator==(name const &x, name const &y) { \
        return true; \
    }
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_EQ(templateParams, name, content) \
    template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
    inline bool operator==(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x, name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &y) { \
        return (true \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EQ_ITEM,_,content) \
            ); \
    }
#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_EQ(templateParams, name) \
    template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
    inline bool operator==(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x, name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &y) { \
        return true; \
    }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem)) const *
#define TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem)) *
#define TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem))
#define TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    &(x.BOOST_PP_TUPLE_ELEM(1,elem))
#define TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1,elem))
#define TM_BASIC_CBOR_CAPABLE_STRUCT_MOVE_VALUE(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    std::move(std::get<BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)>(std::get<0>(*t)))
#define TM_BASIC_CBOR_CAPABLE_STRUCT_MOVE_VALUE_PLAIN(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) \
    std::move(std::get<BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)>(*t))

#define TM_BASIC_CBOR_CAPABLE_STRUCT_ENCODE(name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static std::string apply(name const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name const &x, char *output) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::apply(y, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }, output); \
            } \
            static std::size_t calculateSize(name const &x) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::calculateSize(y, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }); \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_ENCODE_NO_FIELD_NAMES(name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static std::string apply(name const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name const &x, char *output) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    > \
                >::apply(y, output); \
            } \
            static std::size_t calculateSize(name const &x) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    > \
                >::calculateSize(y); \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_DECODE(name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::apply(s, start, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    name { \
                        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_MOVE_VALUE,_,content) \
                    }, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &x, std::string_view const &s, size_t start) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORDeserializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::applyInPlace(y, s, start, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_DECODE_NO_FIELD_NAMES(name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE,_,content) \
                    > \
                >::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    name { \
                        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_MOVE_VALUE,_,content) \
                    }, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &x, std::string_view const &s, size_t start) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORDeserializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                    > \
                >::applyInPlace(y, s, start); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_ENCODE(name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static std::string apply(name const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name const &x, char *output) { \
                return RunCBORSerializerWithNameList<std::tuple<>, 0>::apply(std::tuple<> {}, {}, output); \
            } \
            static std::size_t calculateSize(name const &x) { \
                return RunCBORSerializerWithNameList<std::tuple<>, 0>::calculateSize(std::tuple<> {}, {}); \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_ENCODE_NO_FIELD_NAMES(name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static std::string apply(name const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name const &x, char *output) { \
                return RunCBORSerializer<std::tuple<>>::apply(std::tuple<> {}, output); \
            } \
            static std::size_t calculateSize(name const &x) { \
                return RunCBORSerializer<std::tuple<>>::calculateSize(std::tuple<> {}); \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_DECODE(name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializerWithNameList<std::tuple<>, 0>::apply(s, start, {}); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    name {}, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                std::tuple<> x; \
                return RunCBORDeserializerWithNameList<std::tuple<>, 0>::applyInPlace(x, s, start, {}); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_DECODE_NO_FIELD_NAMES(name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::tuple<>>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    name {}, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                std::tuple<> x; \
                return RunCBORDeserializer<std::tuple<>>::applyInPlace(x, s, start); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ENCODE(templateParams, name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x, char *output) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::apply(y, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }, output); \
            } \
            static std::size_t calculateSize(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::calculateSize(y, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                return RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ENCODE_NO_FIELD_NAMES(templateParams, name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x, char *output) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    > \
                >::apply(y, output); \
            } \
            static std::size_t calculateSize(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORSerializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_CONST_PTR,_,content) \
                    > \
                >::calculateSize(y); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                return RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_DECODE(templateParams, name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::apply(s, start, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t> { \
                    name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> { \
                        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_MOVE_VALUE,_,content) \
                    }, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &x, std::string_view const &s, size_t start) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORDeserializerWithNameList<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                    >, BOOST_PP_SEQ_SIZE(content) \
                >::applyInPlace(y, s, start, { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_NAMES,_,content) \
                }); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_DECODE_NO_FIELD_NAMES(templateParams, name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE,_,content) \
                    > \
                >::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t> { \
                    name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> { \
                        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_MOVE_VALUE,_,content) \
                    }, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &x, std::string_view const &s, size_t start) { \
                std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                > y { \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_GET_FIELD_PTRS,_,content) \
                }; \
                return RunCBORDeserializer<std::tuple< \
                    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_EXTRACT_TYPE_WITH_PTR,_,content) \
                    > \
                >::applyInPlace(y, s, start); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_ENCODE(templateParams, name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x, char *output) { \
                return RunCBORSerializerWithNameList<std::tuple<>, 0>::apply(std::tuple<> {}, {}, output); \
            } \
            static std::size_t calculateSize(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                return RunCBORSerializerWithNameList<std::tuple<>, 0>::calculateSize(std::tuple<> {}, {}); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                return RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_ENCODE_NO_FIELD_NAMES(templateParams, name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                std::string s; \
                s.resize(calculateSize(x)); \
                apply(x, const_cast<char *>(s.data())); \
                return s; \
            } \
            static std::size_t apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x, char *output) { \
                return RunCBORSerializer<std::tuple<>>::apply(std::tuple<> {}, output); \
            } \
            static std::size_t calculateSize(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                return RunCBORSerializer<std::tuple<>>::calculateSize(std::tuple<> {}); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::string apply(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> const &x) { \
                return RunCBORSerializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_DECODE(templateParams, name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializerWithNameList<std::tuple<>, 0>::apply(s, start, {}); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t> { \
                    name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> {}, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string_view const &s, size_t start) { \
                std::tuple<> x; \
                return RunCBORDeserializerWithNameList<std::tuple<>, 0>::applyInPlace(x, s, start, {}); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_DECODE_NO_FIELD_NAMES(templateParams, name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::tuple<>>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::tuple<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, size_t> { \
                    name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> {}, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string_view const &s, size_t start) { \
                std::tuple<> x; \
                return RunCBORDeserializer<std::tuple<>>::applyInPlace(x, s, start); \
            } \
        }; \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        struct RunDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void> { \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)> &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_FIELD_NAME(r, data, elem) \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1,elem)) BOOST_PP_COMMA()

#define TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_FIELD_OFFSET(r, data, elem) \
    offsetof(data, BOOST_PP_TUPLE_ELEM(1,elem)) BOOST_PP_COMMA()

#define TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_GET_FIELD_INDEX(r, data, elem) \
    if (fieldName == BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1,elem))) { \
        return BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS); \
    }

#define TM_BASIC_CBOR_CAPABLE_STRUCT_FIELD_TYPE_INFO(r, data, elem) \
    template <> \
    class StructFieldTypeInfo<data,BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)> { \
    public: \
        using TheType = TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem)); \
        static TheType &access(data &d) { \
            return d.BOOST_PP_TUPLE_ELEM(1,elem); \
        } \
        static TheType const &constAccess(data const &d) { \
            return d.BOOST_PP_TUPLE_ELEM(1,elem); \
        } \
        static TheType &&moveAccess(data &&d) { \
            return std::move(d.BOOST_PP_TUPLE_ELEM(1,elem)); \
        } \
    };

#define TM_BASIC_CBOR_CAPABLE_STRUCT_FIELD_INFO(name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class StructFieldInfo<name> { \
        public: \
            static constexpr bool HasGeneratedStructFieldInfo = true; \
            static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(content)> FIELD_NAMES = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_FIELD_NAME,_,content) \
            }; \
            static constexpr int getFieldIndex(std::string_view const &fieldName) { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_GET_FIELD_INDEX,_,content) \
                return -1; \
            } \
        }; \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_FIELD_TYPE_INFO,name,content); \
        template <> \
        class StructFieldOffsetInfo<name> { \
        public: \
            static constexpr bool HasGeneratedStructFieldOffsetInfo = true; \
            static constexpr std::array<std::size_t, BOOST_PP_SEQ_SIZE(content)> FIELD_OFFSETS = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_FIELD_OFFSET,name,content) \
            }; \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_FIELD_INFO(name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class StructFieldInfo<name> { \
        public: \
            static constexpr bool HasGeneratedStructFieldInfo = true; \
            static constexpr std::array<std::string_view, 0> FIELD_NAMES = {}; \
            static constexpr int getFieldIndex(std::string_view const &fieldName) { \
                return -1; \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ONE_FIELD_NAME(r, data, elem) \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1,elem)) BOOST_PP_COMMA()

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ONE_GET_FIELD_INDEX(r, data, elem) \
    if (fieldName == BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1,elem))) { \
        return BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS); \
    }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_FIELD_TYPE_INFO(r, data, elem) \
    template <BOOST_PP_TUPLE_ENUM(BOOST_PP_TUPLE_ELEM(1,data))> \
    class StructFieldTypeInfo<BOOST_PP_TUPLE_ELEM(0,data)<BOOST_PP_TUPLE_ENUM(BOOST_PP_TUPLE_ELEM(2,data))>,BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)> { \
    public: \
        using TheType = TM_BASIC_CBOR_CAPABLE_STRUCT_TYPE_NAME(BOOST_PP_TUPLE_ELEM(0,elem)); \
        static TheType &access(BOOST_PP_TUPLE_ELEM(0,data)<BOOST_PP_TUPLE_ENUM(BOOST_PP_TUPLE_ELEM(2,data))> &d) { \
            return d.BOOST_PP_TUPLE_ELEM(1,elem); \
        } \
        static TheType const &constAccess(BOOST_PP_TUPLE_ELEM(0,data)<BOOST_PP_TUPLE_ENUM(BOOST_PP_TUPLE_ELEM(2,data))> const &d) { \
            return d.BOOST_PP_TUPLE_ELEM(1,elem); \
        } \
        static TheType &&moveAccess(BOOST_PP_TUPLE_ELEM(0,data)<BOOST_PP_TUPLE_ENUM(BOOST_PP_TUPLE_ELEM(2,data))> &&d) { \
            return std::move(d.BOOST_PP_TUPLE_ELEM(1,elem)); \
        } \
    };

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_FIELD_INFO(templateParams, name, content) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        class StructFieldInfo<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> { \
        public: \
            static constexpr bool HasGeneratedStructFieldInfo = true; \
            static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(content)> FIELD_NAMES = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ONE_FIELD_NAME,_,content) \
            }; \
            static constexpr int getFieldIndex(std::string_view const &fieldName) { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ONE_GET_FIELD_INDEX,_,content) \
                return -1; \
            } \
        }; \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_FIELD_TYPE_INFO,(name, (TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)), (TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams))),content); \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        class StructFieldOffsetInfo<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> { \
        public: \
            static constexpr bool HasGeneratedStructFieldOffsetInfo = true; \
            using TheStruct = name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>; \
            static constexpr std::array<std::size_t, BOOST_PP_SEQ_SIZE(content)> FIELD_OFFSETS = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_STRUCT_ONE_FIELD_OFFSET,TheStruct,content) \
            }; \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_FIELD_INFO(templateParams, name) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_DEF_LIST(templateParams)> \
        class StructFieldInfo<name<TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_TEMPLATE_USE_LIST(templateParams)>> { \
        public: \
            static constexpr bool HasGeneratedStructFieldInfo = true; \
            static constexpr std::array<std::string_view, 0> FIELD_NAMES = {}; \
            static constexpr int getFieldIndex(std::string_view const &fieldName) { \
                return -1; \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_STRUCT(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_DEF(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_EQ(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_PRINT(name, content)

#define TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_ENCODE(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_DECODE(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_FIELD_INFO(name, content)

#define TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE_NO_FIELD_NAMES(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_ENCODE_NO_FIELD_NAMES(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_DECODE_NO_FIELD_NAMES(name, content) \
    TM_BASIC_CBOR_CAPABLE_STRUCT_FIELD_INFO(name, content)

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_DEF(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_EQ(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_PRINT(name)

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_SERIALIZE(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_ENCODE(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_DECODE(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_FIELD_INFO(name)

#define TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_SERIALIZE_NO_FIELD_NAMES(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_ENCODE_NO_FIELD_NAMES(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_DECODE_NO_FIELD_NAMES(name) \
    TM_BASIC_CBOR_CAPABLE_EMPTY_STRUCT_FIELD_INFO(name)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_DEF(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_EQ(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_PRINT(templateParams, name, content)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ENCODE(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_DECODE(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_FIELD_INFO(templateParams, name, content)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE_NO_FIELD_NAMES(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_ENCODE_NO_FIELD_NAMES(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_DECODE_NO_FIELD_NAMES(templateParams, name, content) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_FIELD_INFO(templateParams, name, content)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_DEF(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_EQ(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_PRINT(templateParams, name)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_SERIALIZE(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_ENCODE(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_DECODE(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_FIELD_INFO(templateParams, name)

#define TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_SERIALIZE_NO_FIELD_NAMES(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_ENCODE_NO_FIELD_NAMES(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_DECODE_NO_FIELD_NAMES(templateParams, name) \
    TM_BASIC_CBOR_CAPABLE_TEMPLATE_EMPTY_STRUCT_FIELD_INFO(templateParams, name)

//The followings are for defining enum classes that should be serialized as strings
//If you want enum classes that are serialized as underlying integer types, then
//you don't need these macros

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ITEM_DEF(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) elem

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_DEF(name, items) \
    enum class name { \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ITEM_DEF,_,items) \
    };

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_STRINGIZE(elem)
#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_PAIR_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) std::pair<std::string_view, data> {BOOST_PP_STRINGIZE(elem), data::elem}

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_PRINT(name, items) \
    inline std::ostream &operator<<(std::ostream &os, name const &x) { \
        static const std::string NAMES[] = { \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ARRAY_ITEM,_,items) \
        }; \
        os << NAMES[static_cast<int>(x)]; \
        return os; \
    }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ENCODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct IsEnumWithStringRepresentation<name> { \
            static constexpr bool value = true; \
            static constexpr std::size_t item_count = BOOST_PP_SEQ_SIZE(items); \
            static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(items)> names = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ARRAY_ITEM,_,items) \
            }; \
            static constexpr std::array<std::pair<std::string_view, name>, BOOST_PP_SEQ_SIZE(items)> namesAndValues = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_PAIR_ARRAY_ITEM,name,items) \
            }; \
            static constexpr name lookup(std::string_view const &s) { \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (names[ii] == s) { \
                        return static_cast<name>(ii); \
                    } \
                } \
                return static_cast<name>(item_count); \
            } \
            static constexpr std::size_t findNameIndexForValue(name e) { \
                return static_cast<std::size_t>(e); \
            } \
            static constexpr std::underlying_type_t<name> oneBeyondLargest() { \
                return static_cast<std::underlying_type_t<name>>(item_count); \
            } \
        }; \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static inline const std::array<std::string, BOOST_PP_SEQ_SIZE(items)> S_NAMES = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ARRAY_ITEM,_,items) \
            }; \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<std::string>::apply(S_NAMES[static_cast<int>(x)]); \
            } \
            static std::size_t apply(name const &x, char *output) { \
                return RunCBORSerializer<std::string>::apply(S_NAMES[static_cast<int>(x)], output); \
            } \
            static std::size_t calculateSize(name const &x) { \
                return RunCBORSerializer<std::string>::calculateSize(S_NAMES[static_cast<int>(x)]); \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_MAP_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) { BOOST_PP_STRINGIZE(elem), data::elem }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_DECODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    return name {}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        return static_cast<name>(y); \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            return name {}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_DECODE_STRICT(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    throw std::runtime_error {std::string("Can't parse '")+std::string(s)+"' into "+ #name}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        if (y < 0 || y >= bytedata_utils::RunCBORSerializer<name>::S_NAMES.size()) { \
                            throw std::runtime_error {std::string("Can't parse value ")+std::to_string(y)+" into "+ #name}; \
                        } else { \
                            return static_cast<name>(y); \
                        } \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            throw std::runtime_error {std::string("Can't parse '")+std::string(y)+"' into "+ #name}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ITEM_DEF(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(0,elem)

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_DEF(name, items) \
    enum class name { \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ITEM_DEF,_,items) \
    };

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(1,elem)
#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_PAIR_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) std::pair<std::string_view, data> {BOOST_PP_TUPLE_ELEM(1,elem), data::BOOST_PP_TUPLE_ELEM(0,elem)}

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_PRINT(name, items) \
    inline std::ostream &operator<<(std::ostream &os, name const &x) { \
        static const std::string NAMES[] = { \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ARRAY_ITEM,_,items) \
        }; \
        os << NAMES[static_cast<int>(x)]; \
        return os; \
    }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ENCODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct IsEnumWithStringRepresentation<name> { \
            static constexpr bool value = true; \
            static constexpr std::size_t item_count = BOOST_PP_SEQ_SIZE(items); \
            static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(items)> names = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ARRAY_ITEM,_,items) \
            }; \
            static constexpr std::array<std::pair<std::string_view,name>, BOOST_PP_SEQ_SIZE(items)> namesAndValues = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_PAIR_ARRAY_ITEM,name,items) \
            }; \
            static constexpr name lookup(std::string_view const &s) { \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (names[ii] == s) { \
                        return static_cast<name>(ii); \
                    } \
                } \
                return static_cast<name>(item_count); \
            } \
            static constexpr std::size_t findNameIndexForValue(name e) { \
                return static_cast<std::size_t>(e); \
            } \
            static constexpr std::underlying_type_t<name> oneBeyondLargest() { \
                return static_cast<std::underlying_type_t<name>>(item_count); \
            } \
        }; \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static inline const std::array<std::string, BOOST_PP_SEQ_SIZE(items)> S_NAMES = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ARRAY_ITEM,_,items) \
            }; \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<std::string>::apply(S_NAMES[static_cast<int>(x)]); \
            } \
            static std::size_t apply(name const &x, char *output) { \
                return RunCBORSerializer<std::string>::apply(S_NAMES[static_cast<int>(x)], output); \
            } \
            static std::size_t calculateSize(name const &x) { \
                return RunCBORSerializer<std::string>::calculateSize(S_NAMES[static_cast<int>(x)]); \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_MAP_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) { BOOST_PP_TUPLE_ELEM(1,elem), data::BOOST_PP_TUPLE_ELEM(0,elem) }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_DECODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    return name {}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        return static_cast<name>(y); \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            return name {}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_DECODE_STRICT(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    throw std::runtime_error {std::string("Can't parse '")+std::string(s)+"' into "+ #name}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                return bytedata_utils::RunCBORSerializer<name>::S_NAMES[static_cast<int>(x)]; \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        if (y < 0 || y >= bytedata_utils::RunCBORSerializer<name>::S_NAMES.size()) { \
                            throw std::runtime_error {std::string("Can't parse value ")+std::to_string(y)+" into "+ #name}; \
                        } else { \
                            return static_cast<name>(y); \
                        } \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            throw std::runtime_error {std::string("Can't parse '")+std::string(y)+"' into "+ #name}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }


#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ITEM_DEF(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(0,elem) = BOOST_PP_TUPLE_ELEM(1,elem)

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_DEF(name, items) \
    enum class name { \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ITEM_DEF,_,items) \
    };

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0,elem))
#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_PAIR_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) std::pair<std::string_view, data> {BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0,elem)), data::BOOST_PP_TUPLE_ELEM(0,elem)}
#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_MAP_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) {data::BOOST_PP_TUPLE_ELEM(0,elem), BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0,elem))}


#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_PRINT(name, items) \
    inline std::ostream &operator<<(std::ostream &os, name const &x) { \
        static const std::unordered_map<name, std::string_view> NAMES { \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_MAP_ITEM,name,items) \
        }; \
        auto iter = NAMES.find(x); \
        if (iter == NAMES.end()) { \
            os << "(unknown)"; \
        } else { \
            os << iter->second; \
        } \
        return os; \
    }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ENCODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct IsEnumWithStringRepresentation<name> { \
            static constexpr bool value = true; \
            static constexpr std::size_t item_count = BOOST_PP_SEQ_SIZE(items); \
            static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(items)> names = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ARRAY_ITEM,name,items) \
            }; \
            static constexpr std::array<std::pair<std::string_view, name>, BOOST_PP_SEQ_SIZE(items)> namesAndValues = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_PAIR_ARRAY_ITEM,name,items) \
            }; \
            static constexpr name lookup(std::string_view const &s) { \
                std::underlying_type_t<name> val {0}; \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (namesAndValues[ii].first == s) { \
                        return namesAndValues[ii].second; \
                    } \
                    if (val < static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second)) { \
                        val = static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second); \
                    } \
                } \
                val = val+1; \
                return static_cast<name>(val); \
            } \
            static constexpr std::size_t findNameIndexForValue(name e) { \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (namesAndValues[ii].second == e) { \
                        return ii; \
                    } \
                } \
                return item_count; \
            } \
            static constexpr std::underlying_type_t<name> oneBeyondLargest() { \
                std::underlying_type_t<name> val {0}; \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (val < static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second)) { \
                        val = static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second); \
                    } \
                } \
                return (val+1); \
            } \
        }; \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static inline const std::unordered_map<name, std::string> S_NAMES { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_MAP_ITEM,name,items) \
            }; \
            static std::string apply(name const &x) { \
                auto iter = S_NAMES.find(x); \
                if (iter == S_NAMES.end()) { \
                    return RunCBORSerializer<std::string>::apply("(unknown)"); \
                } else { \
                    return RunCBORSerializer<std::string>::apply(iter->second); \
                } \
            } \
            static std::size_t apply(name const &x, char *output) { \
                auto iter = S_NAMES.find(x); \
                if (iter == S_NAMES.end()) { \
                    return RunCBORSerializer<std::string>::apply("(unknown)", output); \
                } else { \
                    return RunCBORSerializer<std::string>::apply(iter->second, output); \
                } \
            } \
            static std::size_t calculateSize(name const &x) { \
                auto iter = S_NAMES.find(x); \
                if (iter == S_NAMES.end()) { \
                    return RunCBORSerializer<std::string>::calculateSize("(unknown)"); \
                } else { \
                    return RunCBORSerializer<std::string>::calculateSize(iter->second); \
                } \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_REVERSE_MAP_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) { BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0,elem)), data::BOOST_PP_TUPLE_ELEM(0,elem) }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_DECODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_REVERSE_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    return name {}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        return static_cast<name>(y); \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            return name {}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_DECODE_STRICT(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_REVERSE_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    throw std::runtime_error {std::string("Can't parse '")+std::string(s)+"' into "+ #name}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        if (bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(static_cast<name>(y)) == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                            throw std::runtime_error {std::string("Can't parse value ")+std::to_string(y)+" into "+ #name}; \
                        } else { \
                            return static_cast<name>(y); \
                        } \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            throw std::runtime_error {std::string("Can't parse '")+std::string(y)+"' into "+ #name}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }


#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ITEM_DEF(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(0,elem) = BOOST_PP_TUPLE_ELEM(2,elem)

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_DEF(name, items) \
    enum class name { \
        BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ITEM_DEF,_,items) \
    };

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) BOOST_PP_TUPLE_ELEM(1,elem)
#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_PAIR_ARRAY_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) std::pair<std::string_view, data> {BOOST_PP_TUPLE_ELEM(1,elem), data::BOOST_PP_TUPLE_ELEM(0,elem)}
#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_MAP_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) {data::BOOST_PP_TUPLE_ELEM(0,elem), BOOST_PP_TUPLE_ELEM(1,elem)}


#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_PRINT(name, items) \
    inline std::ostream &operator<<(std::ostream &os, name const &x) { \
        static const std::unordered_map<name, std::string_view> NAMES { \
            BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_MAP_ITEM,name,items) \
        }; \
        auto iter = NAMES.find(x); \
        if (iter == NAMES.end()) { \
            os << "(unknown)"; \
        } else { \
            os << iter->second; \
        } \
        return os; \
    }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ENCODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct IsEnumWithStringRepresentation<name> { \
            static constexpr bool value = true; \
            static constexpr std::size_t item_count = BOOST_PP_SEQ_SIZE(items); \
            static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(items)> names = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ARRAY_ITEM,name,items) \
            }; \
            static constexpr std::array<std::pair<std::string_view, name>, BOOST_PP_SEQ_SIZE(items)> namesAndValues = { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_PAIR_ARRAY_ITEM,name,items) \
            }; \
            static constexpr name lookup(std::string_view const &s) { \
                std::underlying_type_t<name> val {0}; \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (namesAndValues[ii].first == s) { \
                        return namesAndValues[ii].second; \
                    } \
                    if (val < static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second)) { \
                        val = static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second); \
                    } \
                } \
                val = val+1; \
                return static_cast<name>(val); \
            } \
            static constexpr std::size_t findNameIndexForValue(name e) { \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (namesAndValues[ii].second == e) { \
                        return ii; \
                    } \
                } \
                return item_count; \
            } \
            static constexpr std::underlying_type_t<name> oneBeyondLargest() { \
                std::underlying_type_t<name> val {0}; \
                for (std::size_t ii=0; ii<item_count; ++ii) { \
                    if (val < static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second)) { \
                        val = static_cast<std::underlying_type_t<name>>(namesAndValues[ii].second); \
                    } \
                } \
                return (val+1); \
            } \
        }; \
        template <> \
        struct RunCBORSerializer<name, void> { \
            static inline const std::unordered_map<name, std::string> S_NAMES { \
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_MAP_ITEM,name,items) \
            }; \
            static std::string apply(name const &x) { \
                auto iter = S_NAMES.find(x); \
                if (iter == S_NAMES.end()) { \
                    return RunCBORSerializer<std::string>::apply("(unknown)"); \
                } else { \
                    return RunCBORSerializer<std::string>::apply(iter->second); \
                } \
            } \
            static std::size_t apply(name const &x, char *output) { \
                auto iter = S_NAMES.find(x); \
                if (iter == S_NAMES.end()) { \
                    return RunCBORSerializer<std::string>::apply("(unknown)", output); \
                } else { \
                    return RunCBORSerializer<std::string>::apply(iter->second, output); \
                } \
            } \
            static std::size_t calculateSize(name const &x) { \
                auto iter = S_NAMES.find(x); \
                if (iter == S_NAMES.end()) { \
                    return RunCBORSerializer<std::string>::calculateSize("(unknown)"); \
                } else { \
                    return RunCBORSerializer<std::string>::calculateSize(iter->second); \
                } \
            } \
        }; \
        template <> \
        struct RunSerializer<name, void> { \
            static std::string apply(name const &x) { \
                return RunCBORSerializer<name, void>::apply(x); \
            } \
        }; \
    } } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_REVERSE_MAP_ITEM(r, data, elem) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(r,TM_SERIALIZATION_HELPER_COMMA_START_POS)) { BOOST_PP_TUPLE_ELEM(1,elem), data::BOOST_PP_TUPLE_ELEM(0,elem) }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_DECODE(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_REVERSE_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    return name {}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    return "(unknown)"; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        return static_cast<name>(y); \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            return name {}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_DECODE_STRICT(name, items) \
    namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils { \
        template <> \
        struct RunCBORDeserializer<name, void> { \
            static inline const std::unordered_map<std::string_view, name> S_MAP = {\
                BOOST_PP_SEQ_FOR_EACH(TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_REVERSE_MAP_ITEM,name,items) \
            }; \
            static std::optional<std::tuple<name, size_t>> apply(std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::string_view {std::get<0>(*t)}); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                return std::tuple<name, size_t> { \
                    iter->second, std::get<1>(*t) \
                }; \
            } \
            static std::optional<size_t> applyInPlace(name &output, std::string_view const &s, size_t start) { \
                auto t = RunCBORDeserializer<std::string>::apply(s, start); \
                if (!t) {\
                    return std::nullopt; \
                } \
                auto iter = S_MAP.find(std::get<0>(*t)); \
                if (iter == S_MAP.end()) { \
                    return std::nullopt; \
                } \
                output = iter->second; \
                return std::get<1>(*t); \
            } \
        }; \
        template <> \
        struct RunDeserializer<name, void> { \
            static std::optional<name> apply(std::string_view const &s) { \
                auto t = RunDeserializer<CBOR<name>, void>::apply(s); \
                if (!t) {\
                    return std::nullopt; \
                } \
                return std::move(t->value); \
            } \
            static std::optional<name> apply(std::string const &s) { \
                return apply(std::string_view {s}); \
            } \
            static bool applyInPlace(name &output, std::string_view const &s) { \
                auto t = RunCBORDeserializer<name, void>::applyInPlace(output, s, 0); \
                if (!t) { \
                    return false; \
                } \
                if (*t != s.length()) { \
                    return false; \
                } \
                return true; \
            } \
            static bool applyInPlace(name &output, std::string const &s) { \
                return applyInPlace(output, std::string_view {s}); \
            } \
        }; \
    } } } } } \
    namespace dev { namespace cd606 { namespace tm { namespace basic { \
        template <> \
        class ConvertibleWithString<name> { \
        public: \
            static constexpr bool value = true; \
            static std::string toString(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromString(std::string_view const &s) { \
                auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(s); \
                if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                    throw std::runtime_error {std::string("Can't parse '")+std::string(s)+"' into "+ #name}; \
                } else { \
                    return iter->second; \
                } \
            } \
        }; \
        template <> \
        class EncodableThroughMultipleProxies<name> { \
        public: \
            static constexpr bool value = true; \
            using CBOREncodeProxyType = std::string; \
            using JSONEncodeProxyType = std::string; \
            using ProtoEncodeProxyType = std::string; \
            using DecodeProxyTypes = std::variant<int, std::string>; \
            static CBOREncodeProxyType toCBOREncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static JSONEncodeProxyType toJSONEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static ProtoEncodeProxyType toProtoEncodeProxy(name const &x) { \
                auto iter = bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(x); \
                if (iter == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                    throw std::runtime_error {std::string("Can't format ")+ #name +" value "+std::to_string(static_cast<std::underlying_type_t<name>>(x))+" into string"}; \
                } else {\
                    return std::string {iter->second}; \
                } \
            } \
            static name fromProxy(DecodeProxyTypes const &x) { \
                return std::visit([](auto const &y) -> name { \
                    using T = std::decay_t<decltype(y)>; \
                    if constexpr (std::is_same_v<T, int>) { \
                        if (bytedata_utils::RunCBORSerializer<name>::S_NAMES.find(static_cast<name>(y)) == bytedata_utils::RunCBORSerializer<name>::S_NAMES.end()) { \
                            throw std::runtime_error {std::string("Can't parse value ")+std::to_string(y)+" into "+ #name}; \
                        } else { \
                            return static_cast<name>(y); \
                        } \
                    } else if constexpr (std::is_same_v<T, std::string>) { \
                        auto iter = bytedata_utils::RunCBORDeserializer<name>::S_MAP.find(y); \
                        if (iter == bytedata_utils::RunCBORDeserializer<name>::S_MAP.end()) { \
                            throw std::runtime_error {std::string("Can't parse '")+std::string(y)+"' into "+ #name}; \
                        } else { \
                            return iter->second; \
                        } \
                    } else { \
                        return name {}; \
                    } \
                }, x); \
            } \
        }; \
    } } } }


#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_DEF(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_PRINT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_SERIALIZE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_DECODE(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_SERIALIZE_STRICT(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_DECODE_STRICT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_DEF(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_PRINT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_SERIALIZE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_DECODE(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_SERIALIZE_STRICT(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_DECODE_STRICT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_DEF(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_PRINT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_SERIALIZE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_DECODE(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_SERIALIZE_STRICT(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_VALUES_DECODE_STRICT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_DEF(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_PRINT(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_SERIALIZE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_DECODE(name, items) 

#define TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_SERIALIZE_STRICT(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_ENCODE(name, items) \
    TM_BASIC_CBOR_CAPABLE_ENUM_AS_STRING_WITH_ALTERNATES_AND_VALUES_DECODE_STRICT(name, items) 

#endif
