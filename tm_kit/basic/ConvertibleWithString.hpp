#ifndef TM_KIT_BASIC_CONVERTIBLE_WITH_STRING_HPP_
#define TM_KIT_BASIC_CONVERTIBLE_WITH_STRING_HPP_

#include <string>
#include <string_view>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    //This class is supposed to be specialized to provide the to/from 
    //implementation
    template <class T>
    class ConvertibleWithString {
    public:
        static constexpr bool value = false;
        static std::string toString(T const &);
        static T fromString(std::string_view const &);
    };
} } } }

#endif