#ifndef TM_KIT_BASIC_ENCODABLE_THROUGH_PROXY_HPP_
#define TM_KIT_BASIC_ENCODABLE_THROUGH_PROXY_HPP_

#include <type_traits>
#include <variant>

#include <tm_kit/basic/ConstValueType.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    //This class is supposed to be specialized to provide the to/from 
    //implementation
    template <class T>
    class EncodableThroughProxy {
    public:
        static constexpr bool value = false;
        using EncodeProxyType = std::monostate;
        using DecodeProxyType = std::monostate;
        static EncodeProxyType const &toProxy(T const &); // or static EncodeProxyType toProxy(T const &); either is ok, the return value will be feeded to a EncodeProxyType const &
        static T fromProxy(DecodeProxyType &&); //or static T fromProxy(DecodeProxyType const &); either is ok, the received type is DecodeProxyType &&
    };

    template <typename T, T val>
    class EncodableThroughProxy<ConstValueType<T,val>> {
    public:
        static constexpr bool value = true;
        using EncodeProxyType = T;
        using DecodeProxyType = T;
        static EncodeProxyType toProxy(ConstValueType<T,val> const &) {
            return val;
        }
        static ConstValueType<T,val> fromProxy(DecodeProxyType const &) {
            return {};
        }
    };

    template <class T>
    class EncodableThroughMultipleProxies {
    public:
        static constexpr bool value = false;
        using CBOREncodeProxyType = std::monostate;
        using JSONEncodeProxyType = std::monostate;
        using ProtoEncodeProxyType = std::monostate;
        using DecodeProxyTypes = std::variant<std::monostate>;
        static CBOREncodeProxyType const &toCBOREncodeProxy(T const &); // or static CBOREncodeProxyType toCBOREncodeProxy(T const &); either is ok, the return value will be feeded to a CBOREncodeProxyType const &
        static JSONEncodeProxyType const &toJSONEncodeProxy(T const &); // or static JSONEncodeProxyType toJSONEncodeProxy(T const &); either is ok, the return value will be feeded to a JSONEncodeProxyType const &
        static ProtoEncodeProxyType const &toProtoEncodeProxy(T const &); // or static ProtoEncodeProxyType toProtoEncodeProxy(T const &); either is ok, the return value will be feeded to a ProtoEncodeProxyType const &
        static T fromProxy(DecodeProxyTypes &&); //or static T fromProxy(DecodeProxyTypes const &); either is ok, the received type is DecodeProxyTypes &&
    };

} } } }

#endif