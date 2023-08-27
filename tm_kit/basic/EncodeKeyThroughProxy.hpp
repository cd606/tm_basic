#ifndef TM_KIT_BASIC_ENCODE_KEY_THROUGH_PROXY_HPP_
#define TM_KIT_BASIC_ENCODE_KEY_THROUGH_PROXY_HPP_

#include <tm_kit/infra/WithTimeData.hpp>

#include <tm_kit/basic/EncodableThroughProxy.hpp>
#include <tm_kit/basic/SerializationHelperMacros.hpp>

#define INFRA_KEY_ENCODE_DECODE_PROXY_TEMPLATE_PARAMS \
    ((typename, K)) \
    ((typename, Env))

#define INFRA_KEY_ENCODE_DECODE_PROXY_FIELDS \
    ((typename Env::IDType, id)) \
    ((K, key))

namespace dev { namespace cd606 { namespace tm { namespace basic { 

    TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT(INFRA_KEY_ENCODE_DECODE_PROXY_TEMPLATE_PARAMS, InfraKeyEncodeDecodeProxy, INFRA_KEY_ENCODE_DECODE_PROXY_FIELDS);
    
    template <class K, class Env>
    class EncodableThroughProxy<typename infra::template Key<K,Env>> {
    public:
        static constexpr bool value = true;
        using EncodeProxyType = InfraKeyEncodeDecodeProxy<K,Env>;
        using DecodeProxyType = InfraKeyEncodeDecodeProxy<K,Env>;
        static EncodeProxyType toProxy(typename infra::template Key<K,Env> const &x) {
            return {x.id(), x.key()};
        }
        static typename infra::template Key<K,Env> fromProxy(DecodeProxyType &&p) {
            return {std::move(p.id), std::move(p.key)};
        }
    };
} } } }

TM_BASIC_CBOR_CAPABLE_TEMPLATE_STRUCT_SERIALIZE(INFRA_KEY_ENCODE_DECODE_PROXY_TEMPLATE_PARAMS, dev::cd606::tm::basic::InfraKeyEncodeDecodeProxy, INFRA_KEY_ENCODE_DECODE_PROXY_FIELDS);

#undef INFRA_KEY_ENCODE_DECODE_PROXY_FIELDS 
#undef INFRA_KEY_ENCODE_DECODE_PROXY_TEMPLATE_PARAMS

#endif