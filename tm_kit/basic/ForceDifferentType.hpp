#ifndef TM_KIT_BASIC_FORCE_DIFFERENT_TYPE_HPP_
#define TM_KIT_BASIC_FORCE_DIFFERENT_TYPE_HPP_

#include <iostream>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T>
    struct ForceDifferentType {};
} } } }

namespace std {
    template <class T>
    class hash<dev::cd606::tm::basic::ForceDifferentType<T>> {
    public:
        size_t operator()(dev::cd606::tm::basic::ForceDifferentType<T> const &) const {
            return 0;
        }
    };
    template <class T>
    class equal_to<dev::cd606::tm::basic::ForceDifferentType<T>> {
    public:
        bool operator()(dev::cd606::tm::basic::ForceDifferentType<T> const &, dev::cd606::tm::basic::ForceDifferentType<T> const &) const {
            return true;
        }
    };
    template <class T>
    class less<dev::cd606::tm::basic::ForceDifferentType<T>> {
    public:
        bool operator()(dev::cd606::tm::basic::ForceDifferentType<T> const &, dev::cd606::tm::basic::ForceDifferentType<T> const &) const {
            return false;
        }
    };
}

#endif