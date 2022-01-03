#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_ANY_ITER_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_ANY_ITER_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <type_traits>
#include <tuple>
#include <string_view>
#include <any>
#include <typeinfo>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {
    namespace internal {
        class AnyIter {
        private:
            template <class T, class OutIter, std::size_t K, std::size_t N>
            static void copyNamesAndValuesToImpl(T const &t, OutIter &outIter) {
                if constexpr (K < N) {
                    *outIter++ = std::tuple<std::string_view, std::any> {
                        StructFieldInfo<T>::FIELD_NAMES[K]
                        , std::any {t.*(StructFieldTypeInfo<T,K>::fieldPointer())}
                    };
                    copyNamesAndValuesToImpl<T,OutIter,K+1,N>(t, outIter);
                }
            }
            template <class T, class OutIter, std::size_t K, std::size_t N>
            static void copyValuesToImpl(T const &t, OutIter &outIter) {
                if constexpr (K < N) {
                    *outIter++ = std::any {t.*(StructFieldTypeInfo<T,K>::fieldPointer())};
                    copyValuesToImpl<T,OutIter,K+1,N>(t, outIter);
                }
            }
        public:
            template <class T, class OutIter, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
            static void copyNamesAndValuesTo(T const &t, OutIter outIter) {
                copyNamesAndValuesToImpl<
                    T, OutIter, 0, StructFieldInfo<T>::FIELD_NAMES.size() 
                >(t, outIter);
            }
            template <class T, class OutIter, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
            static void copyValuesTo(T const &t, OutIter outIter) {
                copyValuesToImpl<
                    T, OutIter, 0, StructFieldInfo<T>::FIELD_NAMES.size() 
                >(t, outIter);
            }
        };

        template <class T, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
        class FieldOperationThroughAny {
        private:
            template <std::size_t K, std::size_t N>
            static std::function<std::any(T const &)> getAnyAccessFuncImpl(std::size_t ii) {
                if constexpr (K<N) {
                    if (K==ii) {
                        return [](T const &t) {
                            return std::any {t.*(StructFieldTypeInfo<T,K>::fieldPointer())};
                        };
                    } else {
                        return getAnyAccessFuncImpl<K+1,N>(ii);
                    }
                } else {
                    return {};
                }
            }
            static std::function<std::any(T const &)> getAnyAccessFunc(std::size_t ii) {
                return getAnyAccessFuncImpl<0,StructFieldInfo<T>::FIELD_NAMES.size()>(ii);
            }
            template <std::size_t K, std::size_t N>
            static std::function<void(T &, std::any const &)> getAnyAssignFuncImpl(std::size_t ii) {
                if constexpr (K<N) {
                    if (K==ii) {
                        return [](T &t, std::any const &input) {
                            t.*(StructFieldTypeInfo<T,K>::fieldPointer()) = std::any_cast<
                                typename StructFieldTypeInfo<T,K>::TheType
                            >(input);
                        };
                    } else {
                        return getAnyAssignFuncImpl<K+1,N>(ii);
                    }
                } else {
                    return {};
                }
            }
            static std::function<void(T &, std::any const &)> getAnyAssignFunc(std::size_t ii) {
                return getAnyAssignFuncImpl<0,StructFieldInfo<T>::FIELD_NAMES.size()>(ii);
            }
            template <std::size_t K, std::size_t N>
            static std::type_index *getTypeInfoImpl(std::size_t ii) {
                if constexpr (K<N) {
                    if (K==ii) {
                        return new std::type_index(typeid(typename StructFieldTypeInfo<T,K>::TheType));
                    } else {
                        return getTypeInfoImpl<K+1,N>(ii);
                    }
                } else {
                    return nullptr;
                }
            }
            static std::type_index *getTypeInfo(std::size_t ii) {
                return getTypeInfoImpl<0,StructFieldInfo<T>::FIELD_NAMES.size()>(ii);
            }
            static auto createFuncArray() -> 
                std::array<std::tuple<
                    std::function<std::any(T const &)>
                    , std::function<void(T &, std::any const &)>
                    , std::type_index *
                >, StructFieldInfo<T>::FIELD_NAMES.size()> 
            {
                static constexpr std::size_t N = StructFieldInfo<T>::FIELD_NAMES.size();
                std::array<std::tuple<
                    std::function<std::any(T const &)>
                    , std::function<void(T &, std::any const &)>
                    , std::type_index *
                >, StructFieldInfo<T>::FIELD_NAMES.size()> ret;
                for (std::size_t ii=0; ii<N; ++ii) {
                    ret[ii] = std::tuple<
                        std::function<std::any(T const &)>
                        , std::function<void(T &, std::any const &)>
                        , std::type_index *
                    >{
                        getAnyAccessFunc(ii)
                        , getAnyAssignFunc(ii)
                        , getTypeInfo(ii)
                    };
                };
                return ret;
            }
            static std::unordered_map<std::string_view, std::size_t> createFieldMap() {
                static constexpr std::size_t N = StructFieldInfo<T>::FIELD_NAMES.size();
                std::unordered_map<std::string_view, std::size_t> ret;
                for (std::size_t ii=0; ii<N; ++ii) {
                    ret[StructFieldInfo<T>::FIELD_NAMES[ii]] = ii;
                };
                return ret;
            }

            static inline std::unordered_map<std::string_view, std::size_t> s_fieldMap = createFieldMap();
            static inline std::array<std::tuple<
                std::function<std::any(T const &)>
                , std::function<void(T &, std::any const &)>
                , std::type_index *
            >, StructFieldInfo<T>::FIELD_NAMES.size()> s_funcArray = createFuncArray();
        public:
            static std::optional<std::any> get(T const &t, std::string_view const &fieldName) {
                auto iter = s_fieldMap.find(fieldName);
                if (iter == s_fieldMap.end()) {
                    return std::nullopt;
                }
                return (std::get<0>(s_funcArray[iter->second]))(t);
            }
            static void set(T &t, std::string_view const &fieldName, std::any const &input) {
                auto iter = s_fieldMap.find(fieldName);
                if (iter == s_fieldMap.end()) {
                    return;
                }
                (std::get<1>(s_funcArray[iter->second]))(t, input);
            }
            static std::optional<std::any> get(T const &t, std::size_t fieldIdx) {
                if (fieldIdx >= s_funcArray.size()) {
                    return std::nullopt;
                }
                return (std::get<0>(s_funcArray[fieldIdx]))(t);
            }
            static void set(T &t, std::size_t fieldIdx, std::any const &input) {
                if (fieldIdx >= s_funcArray.size()) {
                    return;
                }
                (std::get<1>(s_funcArray[fieldIdx]))(t, input);
            }
            template <class X>
            static std::optional<X> getTyped(T const &t, std::string_view const &fieldName) {
                auto iter = s_fieldMap.find(fieldName);
                if (iter == s_fieldMap.end()) {
                    return std::nullopt;
                }
                auto ti = std::get<2>(s_funcArray[iter->second]);
                if (!(ti && (*ti == std::type_index(typeid(X))))) {
                    return std::nullopt;
                }
                return std::any_cast<X>((std::get<0>(s_funcArray[iter->second]))(t));
            }
            template <class X>
            static std::optional<X> getTyped(T const &t, std::size_t fieldIdx) {
                if (fieldIdx >= s_funcArray.size()) {
                    return std::nullopt;
                }
                auto ti = std::get<2>(s_funcArray[fieldIdx]);
                if (!(ti && (*ti == std::type_index(typeid(X))))) {
                    return std::nullopt;
                }
                return std::any_cast<X>((std::get<0>(s_funcArray[fieldIdx]))(t));
            }
            template <class X>
            static bool setTyped(T &t, std::string_view const &fieldName, X const &x) {
                auto iter = s_fieldMap.find(fieldName);
                if (iter == s_fieldMap.end()) {
                    return false;
                }
                auto ti = std::get<2>(s_funcArray[iter->second]);
                if (!(ti && (*ti == std::type_index(typeid(X))))) {
                    return false;
                }
                (std::get<1>(s_funcArray[iter->second]))(t, std::any {x});
                return true;
            }
            template <class X>
            static bool setTyped(T &t, std::size_t fieldIdx, X const &x) {
                if (fieldIdx >= s_funcArray.size()) {
                    return false;
                }
                auto ti = std::get<2>(s_funcArray[fieldIdx]);
                if (!(ti && (*ti == std::type_index(typeid(X))))) {
                    return false;
                }
                (std::get<1>(s_funcArray[fieldIdx]))(t, std::any {x});
                return true;
            }
        };

        template <class T>
        class AssignableFromAnyByName {
        private:
            T *t_;
            std::string_view f_;
            using O = std::optional<std::any>;
        public:
            AssignableFromAnyByName(T *t, std::string_view const &f) 
                : t_(t), f_(f) 
            {}
            AssignableFromAnyByName &operator=(AssignableFromAnyByName const &) = default;
            AssignableFromAnyByName &operator=(std::any const &x) {
                FieldOperationThroughAny<T>::set(*t_, f_, x);
                return *this;
            }
            template <class X>
            AssignableFromAnyByName &operator=(X const &x) {
                return operator=(std::any {x});
            }
            std::any operator*() const {
                return *(FieldOperationThroughAny<T>::get(*t_, f_));
            }
            operator bool() const {
                return (bool) (FieldOperationThroughAny<T>::get(*t_, f_));
            }
        };
        template <class T>
        class AssignableFromAnyByIdx {
        private:
            T *t_;
            std::size_t f_;
        public:
            AssignableFromAnyByIdx(T *t, std::size_t f) 
                : t_(t), f_(f) 
            {}
            AssignableFromAnyByIdx &operator=(AssignableFromAnyByIdx const &) = default;
            AssignableFromAnyByIdx &operator=(std::any const &x) {
                FieldOperationThroughAny<T>::set(*t_, f_, x);
                return *this;
            }
            template <class X>
            AssignableFromAnyByIdx &operator=(X const &x) {
                return operator=(std::any {x});
            }
            std::any operator*() const {
                return *(FieldOperationThroughAny<T>::get(*t_, f_));
            }
            operator bool() const {
                return (bool) (FieldOperationThroughAny<T>::get(*t_, f_));
            }
        };
    }

    template <class T, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
    class DynamicView {
    private:
        T *t_;
    public:
        DynamicView(T &t) : t_(&t) {}
        internal::AssignableFromAnyByName<T> operator[](std::string_view const &s) {
            return {t_, s};
        }
        internal::AssignableFromAnyByIdx<T> operator[](std::size_t idx) {
            return {t_, idx};
        }
        template <class OutIter>
        void copyNamesAndValuesTo(OutIter iter) const {
            internal::AnyIter::copyNamesAndValuesTo(*t_, iter);
        }
        template <class OutIter>
        void copyValuesTo(OutIter iter) const {
            internal::AnyIter::copyValuesTo(*t_, iter);
        }
        template <class Iter>
        void copyFromNamesAndValues(Iter begin, Iter end) {
            for (Iter iter=begin; iter != end; ++iter) {
                internal::FieldOperationThroughAny<T>::set(*t_, std::get<0>(*iter), std::get<1>(*iter));
            }
        }
        template <class Iter>
        void copyFromValues(Iter begin, Iter end) {
            std::size_t idx = 0;
            for (Iter iter=begin; iter != end; ++iter, ++idx) {
                internal::FieldOperationThroughAny<T>::set(*t_, idx, *iter);
            }
        }
        template <class X>
        std::optional<X> get(std::string_view const &s) const {
            return internal::FieldOperationThroughAny<T>::template getTyped<X>(*t_, s);
        }
        template <class X>
        std::optional<X> get(std::size_t idx) const {
            return internal::FieldOperationThroughAny<T>::template getTyped<X>(*t_, idx);
        }
        template <class X>
        bool set(std::string_view const &s, X const &x) {
            return internal::FieldOperationThroughAny<T>::template setTyped<X>(*t_, s, x);
        }
        template <class X>
        bool set(std::size_t idx, X const &x) {
            return internal::FieldOperationThroughAny<T>::template setTyped<X>(*t_, idx, x);
        }
    };

    template <class T, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
    class DynamicConstView {
    private:
        T const *t_;
    public:
        DynamicConstView(T const &t) : t_(&t) {}
        std::optional<std::any> operator[](std::string_view const &s) const {
            return internal::FieldOperationThroughAny<T>::get(*t_, s);
        }
        std::optional<std::any> operator[](std::size_t idx) const {
            return internal::FieldOperationThroughAny<T>::get(*t_, idx);
        }
        template <class OutIter>
        void copyNamesAndValuesTo(OutIter iter) const {
            internal::AnyIter::copyNamesAndValuesTo(*t_, iter);
        }
        template <class OutIter>
        void copyValuesTo(OutIter iter) const {
            internal::AnyIter::copyValuesTo(*t_, iter);
        }
        template <class X>
        std::optional<X> get(std::string_view const &s) const {
            return internal::FieldOperationThroughAny<T>::template getTyped<X>(*t_, s);
        }
        template <class X>
        std::optional<X> get(std::size_t idx) const {
            return internal::FieldOperationThroughAny<T>::template getTyped<X>(*t_, idx);
        }
    };

} } } } }

#endif