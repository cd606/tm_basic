#ifndef TM_KIT_BASIC_CONST_VALUE_TYPE_HPP_
#define TM_KIT_BASIC_CONST_VALUE_TYPE_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <typename T, T val>
    class ConstValueType {
    public:
        using TheType = T;
        static constexpr T TheValue = val;
        constexpr operator T() const {
            return val;
        }
    };

    template <typename T, T val>
    inline bool operator==(ConstValueType<T,val> const &, ConstValueType<T,val> const &) {
        return true;
    }
    
    template <typename T, T val>
    inline bool operator<(ConstValueType<T,val> const &, ConstValueType<T,val> const &) {
        return false;
    }
} } } }

#endif