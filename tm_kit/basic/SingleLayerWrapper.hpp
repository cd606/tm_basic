#ifndef TM_KIT_BASIC_SINGLE_LAYER_WRAPPER_HPP_
#define TM_KIT_BASIC_SINGLE_LAYER_WRAPPER_HPP_

#include <tm_kit/infra/WithTimeData.hpp>

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

namespace dev { namespace cd606 { namespace tm { namespace infra {
    template <class T>
    struct IsKey<basic::SingleLayerWrapper<T>> {
	    enum {value=IsKey<T>::value};
    };
    template <class T>
    struct IsKeyedData<basic::SingleLayerWrapper<T>> {
	    enum {value=IsKeyedData<T>::value};
    };
} } } }

#endif