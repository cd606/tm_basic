#ifndef TM_KIT_BASIC_CONST_TYPE_HPP_
#define TM_KIT_BASIC_CONST_TYPE_HPP_

#include <iostream>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <int32_t N>
    struct ConstType {
    };

    template <int32_t N>
    inline std::ostream &operator<<(std::ostream &os, ConstType<N> const &) {
        os << "ConstType<" << N << ">{}";
        return os;
    }

    template <int32_t N>
    inline bool operator==(ConstType<N> const &, ConstType<N> const &) {
        return true;
    }
    
    template <int32_t N>
    inline bool operator<(ConstType<N> const &, ConstType<N> const &) {
        return false;
    }
} } } }

namespace std {
    template <int32_t N>
    class hash<dev::cd606::tm::basic::ConstType<N>> {
    public:
        size_t operator()(dev::cd606::tm::basic::ConstType<N> const &) const {
            return 0;
        }
    };
    template <int32_t N>
    class equal_to<dev::cd606::tm::basic::ConstType<N>> {
    public:
        bool operator()(dev::cd606::tm::basic::ConstType<N> const &, dev::cd606::tm::basic::ConstType<N> const &) const {
            return true;
        }
    };
    template <int32_t N>
    class less<dev::cd606::tm::basic::ConstType<N>> {
    public:
        bool operator()(dev::cd606::tm::basic::ConstType<N> const &, dev::cd606::tm::basic::ConstType<N> const &) const {
            return false;
        }
    };
}

#endif