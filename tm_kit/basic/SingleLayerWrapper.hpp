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
    template <int32_t N, class T>
    struct SingleLayerWrapperWithID {
        using value_type = T;
        T value;
    };
    template <int32_t N, class T>
    inline bool operator==(SingleLayerWrapperWithID<N, T> const &a, SingleLayerWrapperWithID<N, T> const &b) {
        return (a.value == b.value);
    }
    template <class Mark, class T>
    struct SingleLayerWrapperWithTypeMark {
        using value_type = T;
        T value;
    };
    template <class Mark, class T>
    inline bool operator==(SingleLayerWrapperWithTypeMark<Mark, T> const &a, SingleLayerWrapperWithTypeMark<Mark, T> const &b) {
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
    template <int32_t N, class T>
    struct IsKey<basic::SingleLayerWrapperWithID<N, T>> {
	    enum {value=IsKey<T>::value};
    };
    template <int32_t N, class T>
    struct IsKeyedData<basic::SingleLayerWrapperWithID<N, T>> {
	    enum {value=IsKeyedData<T>::value};
    };
    template <class Mark, class T>
    struct IsKey<basic::SingleLayerWrapperWithTypeMark<Mark, T>> {
	    enum {value=IsKey<T>::value};
    };
    template <class Mark, class T>
    struct IsKeyedData<basic::SingleLayerWrapperWithTypeMark<Mark, T>> {
	    enum {value=IsKeyedData<T>::value};
    };
    namespace withtime_utils {
        template <class T>
        struct ValueCopier<basic::SingleLayerWrapper<T>> {
            inline static basic::SingleLayerWrapper<T> copy(basic::SingleLayerWrapper<T> const &x) {
                return basic::SingleLayerWrapper<T> {ValueCopier<T>::copy(x.value)};
            }
        };
        template <int32_t N, class T>
        struct ValueCopier<basic::SingleLayerWrapperWithID<N, T>> {
            inline static basic::SingleLayerWrapperWithID<N, T> copy(basic::SingleLayerWrapperWithID<N, T> const &x) {
                return basic::SingleLayerWrapperWithID<N, T> {ValueCopier<T>::copy(x.value)};
            }
        };
        template <class Mark, class T>
        struct ValueCopier<basic::SingleLayerWrapperWithTypeMark<Mark, T>> {
            inline static basic::SingleLayerWrapperWithTypeMark<Mark, T> copy(basic::SingleLayerWrapperWithTypeMark<Mark, T> const &x) {
                return basic::SingleLayerWrapperWithTypeMark<Mark, T> {ValueCopier<T>::copy(x.value)};
            }
        };
    }
} } } }

#endif