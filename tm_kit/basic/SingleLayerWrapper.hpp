#ifndef TM_KIT_BASIC_SINGLE_LAYER_WRAPPER_HPP_
#define TM_KIT_BASIC_SINGLE_LAYER_WRAPPER_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T>
    struct SingleLayerWrapper {
        using value_type = T;
        T value;
    };
    template <class T>
    inline bool operator==(SingleLayerWrapper<T> const &a, SingleLayerWrapper<T> const &b) {
        return (a.value == b.value);
    }
} } } }

#endif