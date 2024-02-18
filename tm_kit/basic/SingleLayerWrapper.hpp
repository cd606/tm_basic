#ifndef TM_KIT_BASIC_SINGLE_LAYER_WRAPPER_HPP_
#define TM_KIT_BASIC_SINGLE_LAYER_WRAPPER_HPP_

#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/basic/EqualityCheckHelper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T>
    struct SingleLayerWrapper {
        using value_type = T;
        T value;
    };
    template <class T>
    inline bool operator==(SingleLayerWrapper<T> const &a, SingleLayerWrapper<T> const &b) {
        return EqualityCheckHelper<T>::check(a.value, b.value);
    }
    template <class T>
    inline bool operator<(SingleLayerWrapper<T> const &a, SingleLayerWrapper<T> const &b) {
        return ComparisonHelper<T>::check(a.value, b.value);
    }
    template <int32_t N, class T>
    struct SingleLayerWrapperWithID {
        using value_type = T;
        T value;
        SingleLayerWrapperWithID() = default;
        SingleLayerWrapperWithID(SingleLayerWrapperWithID const &) = default;
        SingleLayerWrapperWithID &operator=(SingleLayerWrapperWithID const &) = default;
        SingleLayerWrapperWithID(SingleLayerWrapperWithID &&) = default;
        SingleLayerWrapperWithID &operator=(SingleLayerWrapperWithID &&) = default;
        ~SingleLayerWrapperWithID() = default;

        SingleLayerWrapperWithID(T const &t) : value(t) {}
        SingleLayerWrapperWithID(T &&t) : value(std::move(t)) {}
        SingleLayerWrapperWithID &operator=(T const &t) {
            value = t;
            return *this;
        }
        SingleLayerWrapperWithID &operator=(T &&t) {
            value = std::move(t);
            return *this;
        }
        operator T() const {
            return value;
        }
        T &operator*() {
            return value;
        }
        T const &operator*() const {
            return value;
        }
        T *operator->() {
            return &value;
        }
        T const *operator->() const {
            return &value;
        }
    };
    template <int32_t N, class T>
    inline bool operator==(SingleLayerWrapperWithID<N, T> const &a, SingleLayerWrapperWithID<N, T> const &b) {
        return EqualityCheckHelper<T>::check(a.value, b.value);
    }
    template <int32_t N, class T>
    inline bool operator<(SingleLayerWrapperWithID<N, T> const &a, SingleLayerWrapperWithID<N, T> const &b) {
        return ComparisonHelper<T>::check(a.value, b.value);
    }
    template <class Mark, class T>
    struct SingleLayerWrapperWithTypeMark {
        using value_type = T;
        T value;
        SingleLayerWrapperWithTypeMark() = default;
        SingleLayerWrapperWithTypeMark(SingleLayerWrapperWithTypeMark const &) = default;
        SingleLayerWrapperWithTypeMark &operator=(SingleLayerWrapperWithTypeMark const &) = default;
        SingleLayerWrapperWithTypeMark(SingleLayerWrapperWithTypeMark &&) = default;
        SingleLayerWrapperWithTypeMark &operator=(SingleLayerWrapperWithTypeMark &&) = default;
        ~SingleLayerWrapperWithTypeMark() = default;

        SingleLayerWrapperWithTypeMark(T const &t) : value(t) {}
        SingleLayerWrapperWithTypeMark(T &&t) : value(std::move(t)) {}
        SingleLayerWrapperWithTypeMark &operator=(T const &t) {
            value = t;
            return *this;
        }
        SingleLayerWrapperWithTypeMark &operator=(T &&t) {
            value = std::move(t);
            return *this;
        }
        operator T() const {
            return value;
        }
        T &operator*() {
            return value;
        }
        T const &operator*() const {
            return value;
        }
        T *operator->() {
            return &value;
        }
        T const *operator->() const {
            return &value;
        }
    };
    template <class Mark, class T>
    inline bool operator==(SingleLayerWrapperWithTypeMark<Mark, T> const &a, SingleLayerWrapperWithTypeMark<Mark, T> const &b) {
        return EqualityCheckHelper<T>::check(a.value, b.value);
    }
    template <class Mark, class T>
    inline bool operator<(SingleLayerWrapperWithTypeMark<Mark, T> const &a, SingleLayerWrapperWithTypeMark<Mark, T> const &b) {
        return ComparisonHelper<T>::check(a.value, b.value);
    }

    template <class T>
    class IsSingleLayerWrapperWithID {
    public:
        static constexpr bool Value = false;
        static constexpr int32_t ID = 0;
        using UnderlyingType = void;
    };
    template <int32_t TheID, class T>
    class IsSingleLayerWrapperWithID<SingleLayerWrapperWithID<TheID,T>> {
    public:
        static constexpr bool Value = true;
        static constexpr int32_t ID = TheID;
        using UnderlyingType = T;
    };
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
