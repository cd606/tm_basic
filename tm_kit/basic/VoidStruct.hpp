#ifndef TM_KIT_BASIC_VOID_STRUCT_HPP_
#define TM_KIT_BASIC_VOID_STRUCT_HPP_

#include <iostream>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    struct VoidStruct {};

    inline std::ostream &operator<<(std::ostream &os, VoidStruct const &) {
        os << "VoidStruct{}";
        return os;
    }
    inline bool operator==(VoidStruct const &, VoidStruct const &) {
        return true;
    }
} } } }

namespace std {
    template <>
    class hash<dev::cd606::tm::basic::VoidStruct> {
    public:
        size_t operator()(dev::cd606::tm::basic::VoidStruct const &) const {
            return 0;
        }
    };
    template <>
    class equal_to<dev::cd606::tm::basic::VoidStruct> {
    public:
        bool operator()(dev::cd606::tm::basic::VoidStruct const &, dev::cd606::tm::basic::VoidStruct const &) const {
            return true;
        }
    };
    template <>
    class less<dev::cd606::tm::basic::VoidStruct> {
    public:
        bool operator()(dev::cd606::tm::basic::VoidStruct const &, dev::cd606::tm::basic::VoidStruct const &) const {
            return false;
        }
    };
}

#endif