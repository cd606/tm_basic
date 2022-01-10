#ifndef TM_KIT_BASIC_ENCODABLE_THROUGH_PROXY_HPP_
#define TM_KIT_BASIC_ENCODABLE_THROUGH_PROXY_HPP_

#include <type_traits>
#include <variant>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    //This class is supposed to be specialized to provide the to/from 
    //implementation
    template <class T>
    class EncodableThroughProxy {
    public:
        static constexpr bool value = false;
        using EncodeProxyType = std::monostate;
        using DecodeProxyType = std::monostate;
        static EncodeProxyType toProxy(T const &);
        static T fromProxy(DecodeProxyType const &);
    };
} } } }

#endif