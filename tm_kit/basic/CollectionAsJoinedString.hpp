#ifndef TM_KIT_BASIC_COLLECTION_AS_JOINED_STRING_HPP_
#define TM_KIT_BASIC_COLLECTION_AS_JOINED_STRING_HPP_

#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/EncodableThroughProxy.hpp>
#include <tm_kit/basic/SerializationHelperMacros_Proxy.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <sstream>
#include <string>

#include <stdint.h>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class Collection, char Sep=','>
    struct CollectionAsJoinedString {
        Collection value;

        bool operator==(CollectionAsJoinedString const &c) const {
            return value == c.value;
        }
        bool operator!=(CollectionAsJoinedString const &c) const {
            return value != c.value;
        }
    };

    template <class Collection, char Sep>
    class ConvertibleWithString<CollectionAsJoinedString<Collection,Sep>> {
    public:
        static constexpr bool value = true;
        static std::string toString(CollectionAsJoinedString<Collection,Sep> const &d) {
            std::ostringstream oss;
            bool start = true;
            for (auto const &x : d.value) {
                if (!start) {
                    oss << Sep;
                }
                oss << x;
                start = false;
            }
            return oss.str();
        }
        static CollectionAsJoinedString<Collection,Sep> fromString(std::string_view const &s) {
            static char sepStr[2] {Sep, '\0'};
            std::vector<std::string> parts;
            std::string s1 {s};
            boost::split(parts, s1, boost::is_any_of(sepStr));
            CollectionAsJoinedString<Collection,Sep> c;
            for (auto const &p : parts) {
                c.value.insert(c.value.end(), boost::lexical_cast<typename Collection::value_type>(p));
            }
            return c;
        }
    };
    template <class Collection, char Sep>
    class EncodableThroughProxy<CollectionAsJoinedString<Collection,Sep>> {
    public:
        static constexpr bool value = true;
        using EncodeProxyType = Collection;
        using DecodeProxyType = Collection;
        static EncodeProxyType const &toProxy(CollectionAsJoinedString<Collection,Sep> const &t) {
            return t.value;
        }
        static CollectionAsJoinedString<Collection,Sep> fromProxy(DecodeProxyType &&c) {
            return {std::move(c)};
        }
    };
}}}}

#define TM_BASIC_COLLECTION_AS_JOINED_STRING_TEMPLATE_ARGS \
    ((typename, Collection)) \
    ((char, Sep))

TM_BASIC_TEMPLATE_PRINT_HELPER_THROUGH_STRING(TM_BASIC_COLLECTION_AS_JOINED_STRING_TEMPLATE_ARGS, dev::cd606::tm::basic::CollectionAsJoinedString);
TM_BASIC_CBOR_TEMPLATE_ENCDEC_THROUGH_PROXY(TM_BASIC_COLLECTION_AS_JOINED_STRING_TEMPLATE_ARGS, dev::cd606::tm::basic::CollectionAsJoinedString);

#undef TM_BASIC_TIME_COLLECTION_AS_JOINED_STRING_TEMPLATE_ARGS

#endif