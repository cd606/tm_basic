#ifndef TM_KIT_BASIC_TRIVIALLY_SERIALIZABLE_HPP_
#define TM_KIT_BASIC_TRIVIALLY_SERIALIZABLE_HPP_

#include <type_traits>
#include <string>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
    struct TriviallySerializable final {
        using value_type = T;
        T value;

        void SerializeToString(std::string *s) const {
            s->resize(sizeof(T));
            std::memcpy(s->data(), &value, sizeof(T));
        }
        bool ParseFromString(std::string const &s) {
            if (s.length() != sizeof(T)) {
                return false;
            }
            std::memcpy(&value, s.data(), sizeof(T));
            return true;
        }
    };
    template <class T>
    inline bool operator==(TriviallySerializable<T> const &a, TriviallySerializable<T> const &b) {
        return (a.value == b.value);
    }
} } } }

#endif