#ifndef TM_KIT_INFRA_INT_ID_COMPONENT_HPP_
#define TM_KIT_INFRA_INT_ID_COMPONENT_HPP_

#include <tm_kit/basic/ByteData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

template <class IntType = uint64_t>
class IntIDComponent {
private:
    static IntType id;
public:
    using IDType = IntType;
    using IDHash = std::hash<IntType>;
    static IDType new_id() {
        return (id++);
    }
    static std::string id_to_string(IDType const &id) {
        return std::to_string(id);
    }
    static IDType id_from_string(std::string const &s) {
        try {
            return std::stoi(s);
        } catch (...) {
            return IDType {};
        }
    }
    static basic::ByteData id_to_bytes(IDType const &id) {
        char buf[sizeof(IntType)];
        std::memcpy(buf, &id, sizeof(IntType));
        return basic::ByteData {std::string(buf, buf+sizeof(IntType))};
    }
    static IDType id_from_bytes(basic::ByteDataView const &bytes) {
        if (bytes.content.length() != sizeof(IntType)) {
            return IDType {};
        }
        IDType id;
        std::memcpy(&id, bytes.content.data(), sizeof(IntType));
        return id;
    }
    static bool less_comparison_id(IDType const &a, IDType const &b) {
        return std::less<IntType>()(a,b);
    }
};

template <class IntType>
IntType IntIDComponent<IntType>::id = 0;

} } } }

#endif