#ifndef TM_KIT_BASIC_CONST_STRING_TYPE_HPP_
#define TM_KIT_BASIC_CONST_STRING_TYPE_HPP_

#include <string_view>
#if (__cplusplus >= 202002L)
#include <algorithm>
#endif
#if (__cplusplus < 202002L) && defined(_MSC_VER)
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#endif

namespace dev { namespace cd606 { namespace tm { namespace basic {
#if __cplusplus >= 202002L
    
    template<size_t N>
    struct ConstStringType_StringLiteral {
        constexpr ConstStringType_StringLiteral(const char (&str)[N]) {
            std::copy_n(str, N, value);
        }
        
        char value[N];
    };

    template <ConstStringType_StringLiteral lit>
    struct ConstStringType_LiteralWrapper {};

    template <class T>
    struct ConstStringType;
    
    template <ConstStringType_StringLiteral lit>
    struct ConstStringType<ConstStringType_LiteralWrapper<lit>> {
        static constexpr std::string_view VALUE = lit.value;

        bool operator==(ConstStringType const &) const {
            return true;
        }
        bool operator<(ConstStringType const &) const {
            return false;
        }
    };

    #define TM_BASIC_CONST_STRING_TYPE(s) dev::cd606::tm::basic::ConstStringType<dev::cd606::tm::basic::ConstStringType_LiteralWrapper<s>>

#else

    #ifdef _MSC_VER

        #define TM_BASIC_CONSTSTRINGTYPE_PRED(r, state) \
        BOOST_PP_NOT_EQUAL( \
            BOOST_PP_TUPLE_ELEM(3, 0, state), \
            BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(3, 1, state)) \
        ) \
        /**/

        #define TM_BASIC_CONSTSTRINGTYPE_OP(r, state) \
        ( \
            BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(3, 0, state)), \
            BOOST_PP_TUPLE_ELEM(3, 1, state), \
            BOOST_PP_TUPLE_ELEM(3, 2, state) \
        ) \
        /**/

        #define TM_BASIC_CONSTSTRINGTYPE_MACRO(r, state) TM_BASIC_CONSTSTRINGTYPE_getChr(BOOST_PP_TUPLE_ELEM(3, 2, state),BOOST_PP_TUPLE_ELEM(3, 0, state)),

        #define TM_BASIC_CONSTSTRINGTYPE_MAX_CONST_CHAR 100

        #define TM_BASIC_CONSTSTRINGTYPE_MIN(a,b) (a)<(b)?(a):(b)

        #define TM_BASIC_CONSTSTRINGTYPE__T(s)\
            BOOST_PP_FOR((0,TM_BASIC_CONSTSTRINGTYPE_MAX_CONST_CHAR,s),TM_BASIC_CONSTSTRINGTYPE_PRED,TM_BASIC_CONSTSTRINGTYPE_OP,TM_BASIC_CONSTSTRINGTYPE_MACRO)\
            0

        #define TM_BASIC_CONSTSTRINGTYPE_getChr(name, ii) ((TM_BASIC_CONSTSTRINGTYPE_MIN(ii,TM_BASIC_CONSTSTRINGTYPE_MAX_CONST_CHAR))<sizeof(name)/sizeof(*name)?name[ii]:0)

        template <char... elements>
        struct ConstStringType_Literal {};
        template <typename>
        struct ConstStringType;
        template <char... elements>
        struct ConstStringType<ConstStringType_Literal<elements...>> {
            static constexpr char TheString[sizeof...(elements)+1] { elements..., '\0' };
            static constexpr std::string_view VALUE = TheString;
            bool operator==(ConstStringType const &) const {
                return true;
            }
            bool operator<(ConstStringType const &) const {
                return false;
            }
        };

        #define TM_BASIC_CONST_STRING_TYPE(s) dev::cd606::tm::basic::ConstStringType<dev::cd606::tm::basic::ConstStringType_Literal<TM_BASIC_CONSTSTRINGTYPE__T(s)>>

    #else

        template <char... chars>
        using ConstStringType_tstring = std::integer_sequence<char, chars...>;
} } } }

        template <typename T, T... chars>
        constexpr dev::cd606::tm::basic::ConstStringType_tstring<chars...> operator""_tmbasic_tstr() { return { }; }

namespace dev { namespace cd606 { namespace tm { namespace basic {        
        template <typename>
        struct ConstStringType;
        template <char... elements>
        struct ConstStringType<ConstStringType_tstring<elements...>> {
            static constexpr char TheString[sizeof...(elements)+1] { elements..., '\0' };
            static constexpr std::string_view VALUE = TheString;
            bool operator==(ConstStringType const &) const {
                return true;
            }
            bool operator<(ConstStringType const &) const {
                return false;
            }
        };

        #define TM_BASIC_CONST_STRING_TYPE(s) dev::cd606::tm::basic::ConstStringType<decltype(s ## _tmbasic_tstr)>

    #endif

#endif

    template <class T>
    inline std::ostream &operator<<(std::ostream &os, ConstStringType<T> const &) {
        os << "ConstStringType<" << ConstStringType<T>::VALUE << ">{}";
        return os;
    }
} } } }

#endif