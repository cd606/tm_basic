#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_ANY_ITER_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_ANY_ITER_HPP_

#include <type_traits>
#include <tuple>
#include <string_view>
#include <any>
#include <typeinfo>
#if __cplusplus < 202002L
#include <boost/hana/type.hpp>
#endif
#include <tm_kit/basic/StructFieldInfoHelper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {
    namespace internal {
#if __cplusplus >= 202002L
        template <class F>
        concept CallAny = requires (F &f, std::any const &x) {
            f(x);
        };
        template <class F>
        concept CallAnyWithName = requires (F &f, std::string_view const &nm, std::any const &x) {
            f(nm, x);
        };
        template <class F>
        concept CallAnyWithNameAndIndex = requires (F &f, std::size_t idx, std::string_view const &nm, std::any const &x) {
            f(idx, nm, x);
        };
        template <class F, class X>
            concept CallX = requires (F &f, X const &x) {
                f(x);
            };
            template <class F, class X>
            concept CallXWithName = requires (F &f, std::string_view const &nm, X const &x) {
                f(nm, x);
            };
            template <class F, class X>
            concept CallXWithNameAndIndex = requires (F &f, std::size_t idx, std::string_view const &nm, X const &x) {
                f(idx, nm, x);
            };
        template <class F, class X>
        concept UpdateX = requires (F &f, X &x) {
            f(x);
        };
        template <class F, class X>
        concept UpdateXWithName = requires (F &f, std::string_view const &nm, X &x) {
            f(nm, x);
        };
        template <class F, class X>
        concept UpdateXWithNameAndIndex = requires (F &f, std::size_t idx, std::string_view const &nm, X &x) {
            f(idx, nm, x);
        };
#endif
        class AnyIter {
        private:
            template <class T, class OutIter, std::size_t K, std::size_t N>
            static void copyNamesAndValuesToImpl(T const &t, OutIter &outIter) {
                if constexpr (K < N) {
                    *outIter++ = std::tuple<std::string_view, std::any> {
                        StructFieldInfo<T>::FIELD_NAMES[K]
                        , std::any {StructFieldTypeInfo<T,K>::constAccess(t)}
                    };
                    copyNamesAndValuesToImpl<T,OutIter,K+1,N>(t, outIter);
                }
            }
            template <class T, class OutIter, std::size_t K, std::size_t N>
            static void copyValuesToImpl(T const &t, OutIter &outIter) {
                if constexpr (K < N) {
                    *outIter++ = std::any {StructFieldTypeInfo<T,K>::constAccess(t)};
                    copyValuesToImpl<T,OutIter,K+1,N>(t, outIter);
                }
            }
#if __cplusplus >= 202002L
            template <class T, class F, std::size_t K, std::size_t N>
            static void forAllImpl(T const &t, F &&f) {
                if constexpr (K < N) {
                    if constexpr (CallAny<F>) {
                        f(std::any {StructFieldTypeInfo<T,K>::constAccess(t)});
                    } else if constexpr (CallAnyWithName<F>) {
                        f(StructFieldInfo<T>::FIELD_NAMES[K], std::any {StructFieldTypeInfo<T,K>::constAccess(t)});
                    } else if constexpr (CallAnyWithNameAndIndex<F>) {
                        f(K, StructFieldInfo<T>::FIELD_NAMES[K], std::any {StructFieldTypeInfo<T,K>::constAccess(t)});
                    }
                    forAllImpl<T,F,K+1,N>(t, std::forward<F>(f));
                }
            }
            template <class T, class X, class F, std::size_t K, std::size_t N>
            static void forAllByTypeImpl(T const &t, F &&f) {
                if constexpr (K < N) {
                    if constexpr (std::is_same_v<X, typename StructFieldTypeInfo<T,K>::TheType>) {
                        if constexpr (CallX<F, X>) {
                            f(StructFieldTypeInfo<T,K>::constAccess(t));
                        } else if constexpr (CallXWithName<F, X>) {
                            f(StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::constAccess(t));
                        } else if constexpr (CallXWithNameAndIndex<F, X>) {
                            f(K, StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::constAccess(t));
                        }
                    }
                    forAllByTypeImpl<T,X,F,K+1,N>(t, std::forward<F>(f));
                }
            }
            
            template <class T, class X, class F, std::size_t K, std::size_t N>
            static void updateAllByTypeImpl(T &t, F &&f) {
                if constexpr (K < N) {
                    if constexpr (std::is_same_v<X, typename StructFieldTypeInfo<T,K>::TheType>) {
                        if constexpr (UpdateX<F, X>) {
                            f(StructFieldTypeInfo<T,K>::access(t));
                        } else if constexpr (UpdateXWithName<F, X>) {
                            f(StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::access(t));
                        } else if constexpr (UpdateXWithNameAndIndex<F, X>) {
                            f(K, StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::access(t));
                        }
                    }
                    updateAllByTypeImpl<T,X,F,K+1,N>(t, std::forward<F>(f));
                }
            }
#else
            template <class T, class F, std::size_t K, std::size_t N>
            static void forAllImpl(T const &t, F &&f) {
                static const auto checker1 = boost::hana::is_valid(
                    [](auto *f, auto const *data) -> decltype((void) ((*f)(*data))) {}
                );
                static const auto checker2 = boost::hana::is_valid(
                    [](auto *f, std::string_view const *nm, auto const *data) -> decltype((void) ((*f)(*nm, *data))) {}
                );
                static const auto checker3 = boost::hana::is_valid(
                    [](auto *f, std::size_t const *idx, std::string_view const *nm, auto const *data) -> decltype((void) ((*f)(*idx, *nm, *data))) {}
                );
                if constexpr (K < N) {
                    if constexpr (checker1((F *) nullptr, (std::any const *) nullptr)) {
                        f(std::any {StructFieldTypeInfo<T,K>::constAccess(t)});
                    } else if constexpr (checker2((F *) nullptr, (std::string_view const *) nullptr, (std::any const *) nullptr)) {
                        f(StructFieldInfo<T>::FIELD_NAMES[K], std::any {StructFieldTypeInfo<T,K>::constAccess(t)});
                    } else if constexpr (checker3((F *) nullptr, (std::size_t const *) nullptr, (std::string_view const *) nullptr, (std::any const *) nullptr)) {
                        f(K, StructFieldInfo<T>::FIELD_NAMES[K], std::any {StructFieldTypeInfo<T,K>::constAccess(t)});
                    }
                    forAllImpl<T,F,K+1,N>(t, std::forward<F>(f));
                }
            }
            template <class T, class X, class F, std::size_t K, std::size_t N>
            static void forAllByTypeImpl(T const &t, F &&f) {
                static const auto checker1 = boost::hana::is_valid(
                    [](auto *f, X const *data) -> decltype((void) ((*f)(*data))) {}
                );
                static const auto checker2 = boost::hana::is_valid(
                    [](auto *f, std::string_view const *nm, X const *data) -> decltype((void) ((*f)(*nm, *data))) {}
                );
                static const auto checker3 = boost::hana::is_valid(
                    [](auto *f, std::size_t const *idx, std::string_view const *nm, X const *data) -> decltype((void) ((*f)(*idx, *nm, *data))) {}
                );
                if constexpr (K < N) {
                    if constexpr (std::is_same_v<X, typename StructFieldTypeInfo<T,K>::TheType>) {
                        if constexpr (checker1((F *) nullptr, (X const *) nullptr)) {
                            f(StructFieldTypeInfo<T,K>::constAccess(t));
                        } else if constexpr (checker2((F *) nullptr, (std::string_view const *) nullptr, (X const *) nullptr)) {
                            f(StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::constAccess(t));
                        } else if constexpr (checker3((F *) nullptr, (std::size_t const *) nullptr, (std::string_view const *) nullptr, (X const *) nullptr)) {
                            f(K, StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::constAccess(t));
                        }
                    }
                    forAllByTypeImpl<T,X,F,K+1,N>(t, std::forward<F>(f));
                }
            }
            template <class T, class X, class F, std::size_t K, std::size_t N>
            static void updateAllByTypeImpl(T &t, F &&f) {
                static const auto checker1 = boost::hana::is_valid(
                    [](auto *f, X *data) -> decltype((void) ((*f)(*data))) {}
                );
                static const auto checker2 = boost::hana::is_valid(
                    [](auto *f, std::string_view const *nm, X *data) -> decltype((void) ((*f)(*nm, *data))) {}
                );
                static const auto checker3 = boost::hana::is_valid(
                    [](auto *f, std::size_t const *idx, std::string_view const *nm, X *data) -> decltype((void) ((*f)(*idx, *nm, *data))) {}
                );
                if constexpr (K < N) {
                    if constexpr (std::is_same_v<X, typename StructFieldTypeInfo<T,K>::TheType>) {
                        if constexpr (checker1((F *) nullptr, (X *) nullptr)) {
                            f(StructFieldTypeInfo<T,K>::access(t));
                        } else if constexpr (checker2((F *) nullptr, (std::string_view const *) nullptr, (X *) nullptr)) {
                            f(StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::access(t));
                        } else if constexpr (checker3((F *) nullptr, (std::size_t const *) nullptr, (std::string_view const *) nullptr, (X *) nullptr)) {
                            f(K, StructFieldInfo<T>::FIELD_NAMES[K], StructFieldTypeInfo<T,K>::access(t));
                        }
                    }
                    updateAllByTypeImpl<T,X,F,K+1,N>(t, std::forward<F>(f));
                }
            }
#endif
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
            template <class T, class F>
            static void forAll(T const &t, F &&f) {
                forAllImpl<T,F,0,StructFieldInfo<T>::FIELD_NAMES.size()>(t, std::forward<F>(f));
            }
            template <class T, class X, class F>
            static void forAllByType(T const &t, F &&f) {
                forAllByTypeImpl<T,X,F,0,StructFieldInfo<T>::FIELD_NAMES.size()>(t, std::forward<F>(f));
            }
            template <class T, class X, class F>
            static void updateAllByType(T &t, F &&f) {
                updateAllByTypeImpl<T,X,F,0,StructFieldInfo<T>::FIELD_NAMES.size()>(t, std::forward<F>(f));
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
                            return std::any {StructFieldTypeInfo<T,K>::constAccess(t)};
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
                            StructFieldTypeInfo<T,K>::access(t) = std::any_cast<
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
        template <class F>
        void forAll(F &&f) const {
            internal::AnyIter::forAll<T,F>(*t_, std::forward<F>(f));
        }
        template <class X, class F>
        void forAllByType(F &&f) const {
            internal::AnyIter::forAllByType<T,X,F>(*t_, std::forward<F>(f));
        }
        template <class X, class F>
        void updateAllByType(F &&f) {
            internal::AnyIter::updateAllByType<T,X,F>(*t_, std::forward<F>(f));
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
        template <class F>
        void forAll(F &&f) const {
            internal::AnyIter::forAll<T,F>(*t_, std::forward<F>(f));
        }
        template <class X, class F>
        void forAllByType(F &&f) const {
            internal::AnyIter::forAllByType<T,X,F>(*t_, std::forward<F>(f));
        }
    };

} } } } }

#endif