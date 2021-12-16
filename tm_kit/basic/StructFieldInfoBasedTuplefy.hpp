#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_TUPLEFY_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_TUPLEFY_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tuple>
#include <utility>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    namespace struct_field_info_utils {
        template <class T, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
        class StructFieldInfoBasedTuplefy {
        private:
            static constexpr std::size_t N = StructFieldInfo<T>::FIELD_NAMES.size();
            using IndexSeq = std::make_integer_sequence<int, N>;
            template <int K>
            using OneFieldType = typename StructFieldTypeInfo<T,K>::TheType;
            template <int K>
            using OnePtrFieldType = typename StructFieldTypeInfo<T,K>::TheType *;
            template <int K>
            using OneConstPtrFieldType = typename StructFieldTypeInfo<T,K>::TheType const *;
            template <class X>
            class TupleTypeInternal {};
            template <int... Ints>
            class TupleTypeInternal<std::integer_sequence<int, Ints...>> {
            public:
                using TheType = std::tuple<OneFieldType<Ints>...>;
                using ThePtrType = std::tuple<OnePtrFieldType<Ints>...>;
                using TheConstPtrType = std::tuple<OneConstPtrFieldType<Ints>...>;
            };
            template <int K>
            static constexpr OneFieldType<K> fetchOneField(T const &t) {
                return t.*(StructFieldTypeInfo<T,K>::fieldPointer());
            }
            template <int K>
            static constexpr OnePtrFieldType<K> fetchOneFieldPointer(T const &t) {
                return &(t.*(StructFieldTypeInfo<T,K>::fieldPointer()));
            }
            template <int K>
            static constexpr OneConstPtrFieldType<K> fetchOneFieldConstPointer(T const &t) {
                return &(t.*(StructFieldTypeInfo<T,K>::fieldPointer()));
            }
            template <int K>
            static constexpr void copyFromField(T &t, OneFieldType<K> const &v) {
                t.*(StructFieldTypeInfo<T,K>::fieldPointer()) = v;
            }
            template <int K>
            static void moveFromField(T &t, OneFieldType<K> &&v) {
                t.*(StructFieldTypeInfo<T,K>::fieldPointer()) = std::move(v);
            }
        public:
            using TupleType = typename TupleTypeInternal<IndexSeq>::TheType;
            using PtrTupleType = typename TupleTypeInternal<IndexSeq>::ThePtrType;
            using ConstPtrTupleType = typename TupleTypeInternal<IndexSeq>::TheConstPtrType;
        private:
            template <int... Ints>
            static constexpr TupleType fetchTuple(std::integer_sequence<int, Ints...> const *seq, T const &t) {
                return TupleType {fetchOneField<Ints>(t)...};
            }
            template <int... Ints>
            static constexpr PtrTupleType fetchPtrTuple(std::integer_sequence<int, Ints...> const *seq, T const &t) {
                return ConstPtrTupleType {fetchOneFieldPointer<Ints>(t)...};
            }
            template <int... Ints>
            static constexpr ConstPtrTupleType fetchConstPtrTuple(std::integer_sequence<int, Ints...> const *seq, T const &t) {
                return ConstPtrTupleType {fetchOneFieldConstPointer<Ints>(t)...};
            }
            template <int K>
            static constexpr void copyFromTuple_internal(T &t, TupleType const &tu) {
                if constexpr (K < N) {
                    copyFromField<K>(t, std::get<K>(tu));
                    copyFromTuple_internal<K+1>(t, tu);
                }
            }
            template <int K>
            static void moveFromTuple_internal(T &t, TupleType &&tu) {
                if constexpr (K < N) {
                    moveFromField<K>(t, std::move(std::get<K>(tu)));
                    moveFromTuple_internal<K+1>(t, std::move(tu));
                }
            }
        public:
            static constexpr TupleType toTuple(T const &t) {
                return fetchTuple((IndexSeq const *) nullptr, t);
            }
            static constexpr PtrTupleType toPtrTuple(T &t) {
                return fetchPtrTuple((IndexSeq const *) nullptr, t);
            }
            static constexpr ConstPtrTupleType toConstPtrTuple(T const &t) {
                return fetchConstPtrTuple((IndexSeq const *) nullptr, t);
            }
            static constexpr void copyFromTuple(T &t, TupleType const &tu) {
                copyFromTuple_internal<0>(t, tu);
            }
            static void moveFromTuple(T &t, TupleType &&tu) {
                moveFromTuple_internal<0>(t, std::move(tu));
            }
            static constexpr T fromTuple(TupleType const &tu) {
                T t;
                copyFromTuple(t, tu);
                return t;
            }
            static T fromTupleByMove(TupleType &&tu) {
                T t;
                moveFromTuple(t, std::move(tu));
                return t;
            }
        };
    }
} } } }

#endif