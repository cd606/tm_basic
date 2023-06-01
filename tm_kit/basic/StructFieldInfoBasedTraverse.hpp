#ifndef TM_KIT_BASICSTRUCT_FIELD_INFO_BASED_TRAVERSE_HPP_
#define TM_KIT_BASICSTRUCT_FIELD_INFO_BASED_TRAVERSE_HPP_

#if __cplusplus >= 202002L

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>

#include <concepts>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {
    template <class T>
    class StructFieldInfoBasedTraverse {    
    private:
        template <class X, std::size_t Idx, std::size_t Count, typename F>
        static void traverse_helper(X &t, F &f) {
            if constexpr (Idx >= 0 && Idx < Count) {
                StructFieldInfoBasedTraverse<typename StructFieldTypeInfo<X,Idx>::TheType>::traverse(StructFieldTypeInfo<X,Idx>::access(t), f);
                traverse_helper<X,Idx+1,Count,F>(t, f);
            }
        }
        template <class X, std::size_t Idx, std::size_t Count, typename F>
        static void fold_helper(X const &t, F &f) {
            if constexpr (Idx >= 0 && Idx < Count) {
                StructFieldInfoBasedTraverse<typename StructFieldTypeInfo<X,Idx>::TheType>::fold(StructFieldTypeInfo<X,Idx>::constAccess(t), f);
                fold_helper<X,Idx+1,Count,F>(t, f);
            }
        }
        template <class X, std::size_t Idx, std::size_t Count, typename F>
        static void apply_helper(X &t, F const &f) {
            if constexpr (Idx >= 0 && Idx < Count) {
                StructFieldInfoBasedTraverse<typename StructFieldTypeInfo<X,Idx>::TheType>::apply(StructFieldTypeInfo<X,Idx>::access(t), f);
                apply_helper<X,Idx+1,Count,F>(t, f);
            }
        }
    public:
        template <class F>
        static void traverse(T &t, F &f) {
            if constexpr (std::invocable<F &,T &>) {
                f(t);
            } else if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                traverse_helper<T, 0, StructFieldInfo<T>::FIELD_NAMES.size(), F>(t, f);
            }
        }
        template <class F>
        static void fold(T const &t, F &f) {
            if constexpr (std::invocable<F &,T const &>) {
                f(t);
            } else if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                fold_helper<T, 0, StructFieldInfo<T>::FIELD_NAMES.size(), F>(t, f);
            }
        }
        template <class F>
        static void apply(T &t, F const &f) {
            if constexpr (std::invocable<F const &,T &>) {
                f(t);
            } else if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                apply_helper<T, 0, StructFieldInfo<T>::FIELD_NAMES.size(), F>(t, f);
            }
        }
    };
    template <class T, std::size_t N>
    class StructFieldInfoBasedTraverse<std::array<T,N>> {    
    public:
        template <class F>
        static void traverse(std::array<T,N> &t, F &f) {
            if constexpr (std::invocable<F &,std::array<T,N> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x, f);
                }
            }
        }
        template <class F>
        static void fold(std::array<T,N> const &t, F &f) {
            if constexpr (std::invocable<F &,std::array<T,N> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x, f);
                }
            }
        }
        template <class F>
        static void apply(std::array<T,N> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::array<T,N> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x, f);
                }
            }
        }
    };
    template <class T>
    class StructFieldInfoBasedTraverse<std::vector<T>> {    
    public:
        template <class F>
        static void traverse(std::vector<T> &t, F &f) {
            if constexpr (std::invocable<F &,std::vector<T> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x, f);
                }
            }
        }
        template <class F>
        static void fold(std::vector<T> const &t, F &f) {
            if constexpr (std::invocable<F &,std::vector<T> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x, f);
                }
            }
        }
        template <class F>
        static void apply(std::vector<T> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::vector<T> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x, f);
                }
            }
        }
    };
    template <class T>
    class StructFieldInfoBasedTraverse<std::list<T>> {    
    public:
        template <class F>
        static void traverse(std::list<T> &t, F &f) {
            if constexpr (std::invocable<F &,std::list<T> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x, f);
                }
            }
        }
        template <class F>
        static void fold(std::list<T> const &t, F &f) {
            if constexpr (std::invocable<F &,std::list<T> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x, f);
                }
            }
        }
        template <class F>
        static void apply(std::list<T> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::list<T> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x, f);
                }
            }
        }
    };
    template <class T>
    class StructFieldInfoBasedTraverse<std::valarray<T>> {    
    public:
        template <class F>
        static void traverse(std::valarray<T> &t, F &f) {
            if constexpr (std::invocable<F &,std::valarray<T> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x, f);
                }
            }
        }
        template <class F>
        static void fold(std::valarray<T> const &t, F &f) {
            if constexpr (std::invocable<F &,std::valarray<T> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x, f);
                }
            }
        }
        template <class F>
        static void apply(std::valarray<T> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::valarray<T> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x, f);
                }
            }
        }
    };
    template <class T, class Cmp>
    class StructFieldInfoBasedTraverse<std::set<T,Cmp>> {    
    public:
        template <class F>
        static void traverse(std::set<T,Cmp> &t, F &f) {
            if constexpr (std::invocable<F &,std::set<T,Cmp> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x, f);
                }
            }
        }
        template <class F>
        static void fold(std::set<T,Cmp> const &t, F &f) {
            if constexpr (std::invocable<F &,std::set<T,Cmp> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x, f);
                }
            }
        }
        template <class F>
        static void apply(std::set<T,Cmp> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::set<T,Cmp> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x, f);
                }
            }
        }
    };
    template <class T, class Hash>
    class StructFieldInfoBasedTraverse<std::unordered_set<T,Hash>> {    
    public:
        template <class F>
        static void traverse(std::unordered_set<T,Hash> &t, F &f) {
            if constexpr (std::invocable<F &,std::unordered_set<T,Hash> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x, f);
                }
            }
        }
        template <class F>
        static void fold(std::unordered_set<T,Hash> const &t, F &f) {
            if constexpr (std::invocable<F &,std::unordered_set<T,Hash> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x, f);
                }
            }
        }
        template <class F>
        static void apply(std::unordered_set<T,Hash> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::unordered_set<T,Hash> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x, f);
                }
            }
        }
    };
    template <class T, class Key, class Cmp>
    class StructFieldInfoBasedTraverse<std::map<Key,T,Cmp>> {    
    public:
        template <class F>
        static void traverse(std::map<Key,T,Cmp> &t, F &f) {
            if constexpr (std::invocable<F &,std::map<Key,T,Cmp> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x.second, f);
                }
            }
        }
        template <class F>
        static void fold(std::map<Key,T,Cmp> const &t, F &f) {
            if constexpr (std::invocable<F &,std::map<Key,T,Cmp> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x.second, f);
                }
            }
        }
        template <class F>
        static void apply(std::map<Key,T,Cmp> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::map<Key,T,Cmp> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x.second, f);
                }
            }
        }
    };
    template <class T, class Key, class Hash>
    class StructFieldInfoBasedTraverse<std::unordered_map<Key,T,Hash>> {    
    public:
        template <class F>
        static void traverse(std::unordered_map<Key,T,Hash> &t, F &f) {
            if constexpr (std::invocable<F &,std::unordered_map<Key,T,Hash> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::traverse(x.second, f);
                }
            }
        }
        template <class F>
        static void fold(std::unordered_map<Key,T,Hash> const &t, F &f) {
            if constexpr (std::invocable<F &,std::unordered_map<Key,T,Hash> const &>) {
                f(t);
            } else {
                for (auto const &x : t) {
                    StructFieldInfoBasedTraverse<T>::fold(x.second, f);
                }
            }
        }
        template <class F>
        static void apply(std::unordered_map<Key,T,Hash> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::unordered_map<Key,T,Hash> &>) {
                f(t);
            } else {
                for (auto &x : t) {
                    StructFieldInfoBasedTraverse<T>::apply(x.second, f);
                }
            }
        }
    };
    template <typename... Ts>
    class StructFieldInfoBasedTraverse<std::tuple<Ts...>> {
    private:
        template <std::size_t Idx, typename F>
        static void traverse_helper(std::tuple<Ts...> &t, F &f) {
            if constexpr (Idx >= 0 && Idx < sizeof...(Ts)) {
                StructFieldInfoBasedTraverse<std::tuple_element_t<Idx, std::tuple<Ts...>>>::traverse(std::get<Idx>(t), f);
                traverse_helper<Idx+1,F>(t, f);
            }
        }
        template <std::size_t Idx, typename F>
        static void fold_helper(std::tuple<Ts...> const &t, F &f) {
            if constexpr (Idx >= 0 && Idx < sizeof...(Ts)) {
                StructFieldInfoBasedTraverse<std::tuple_element_t<Idx, std::tuple<Ts...>>>::fold(std::get<Idx>(t), f);
                fold_helper<Idx+1,F>(t, f);
            }
        }
        template <std::size_t Idx, typename F>
        static void apply_helper(std::tuple<Ts...> &t, F const &f) {
            if constexpr (Idx >= 0 && Idx < sizeof...(Ts)) {
                StructFieldInfoBasedTraverse<std::tuple_element_t<Idx, std::tuple<Ts...>>>::apply(std::get<Idx>(t), f);
                apply_helper<Idx+1,F>(t, f);
            }
        }
    public:
        template <class F>
        static void traverse(std::tuple<Ts...> &t, F &f) {
            if constexpr (std::invocable<F &,std::tuple<Ts...> &>) {
                f(t);
            } else {
                traverse_helper<0, F>(t, f);
            }
        }
        template <class F>
        static void fold(std::tuple<Ts...> const &t, F &f) {
            if constexpr (std::invocable<F &,std::tuple<Ts...> const &>) {
                f(t);
            } else {
                fold_helper<0, F>(t, f);
            }
        }
        template <class F>
        static void apply(std::tuple<Ts...> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::tuple<Ts...> &>) {
                f(t);
            } else {
                apply_helper<0, F>(t, f);
            }
        }
    };
    template <typename... Ts>
    class StructFieldInfoBasedTraverse<std::variant<Ts...>> {
    private:
        template <std::size_t Idx, typename F>
        static void traverse_helper(std::variant<Ts...> &t, F &f) {
            if constexpr (Idx >= 0 && Idx < sizeof...(Ts)) {
                if (t.index() == Idx) {
                    StructFieldInfoBasedTraverse<std::variant_alternative_t<Idx, std::variant<Ts...>>>::traverse(std::get<Idx>(t), f);
                }
                traverse_helper<Idx+1,F>(t, f);
            }
        }
        template <std::size_t Idx, typename F>
        static void fold_helper(std::variant<Ts...> const &t, F &f) {
            if constexpr (Idx >= 0 && Idx < sizeof...(Ts)) {
                if (t.index() == Idx) {
                    StructFieldInfoBasedTraverse<std::variant_alternative_t<Idx, std::variant<Ts...>>>::fold(std::get<Idx>(t), f);
                }
                fold_helper<Idx+1,F>(t, f);
            }
        }
        template <std::size_t Idx, typename F>
        static void apply_helper(std::variant<Ts...> &t, F const &f) {
            if constexpr (Idx >= 0 && Idx < sizeof...(Ts)) {
                if (t.index() == Idx) {
                    StructFieldInfoBasedTraverse<std::variant_alternative_t<Idx, std::variant<Ts...>>>::apply(std::get<Idx>(t), f);
                }
                apply_helper<Idx+1,F>(t, f);
            }
        }
    public:
        template <class F>
        static void traverse(std::variant<Ts...> &t, F &f) {
            if constexpr (std::invocable<F &,std::variant<Ts...> &>) {
                f(t);
            } else {
                traverse_helper<0, F>(t, f);
            }
        }
        template <class F>
        static void fold(std::variant<Ts...> const &t, F &f) {
            if constexpr (std::invocable<F &,std::variant<Ts...> const &>) {
                f(t);
            } else {
                fold_helper<0, F>(t, f);
            }
        }
        template <class F>
        static void apply(std::variant<Ts...> &t, F const &f) {
            if constexpr (std::invocable<F const &,std::variant<Ts...> &>) {
                f(t);
            } else {
                apply_helper<0, F>(t, f);
            }
        }
    };
} } } } }

#endif

#endif