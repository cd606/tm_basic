#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/StructFieldFlattenedInfo.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <chrono>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {

    namespace internal {
        
        template <class T, class U, class ComplexCopy>
        class CopySimpleImpl {
        public:
            static void copy(T &dest, U const &src) {
                GetRef<T>::ref(dest) = T(GetRef<U>::constRef(src));
            }
            static void move(T &dest, U &&src) {
                GetRef<T>::ref(dest) = T(std::move(GetRef<U>::moveRef(src)));
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<std::tm,std::chrono::system_clock::time_point,ComplexCopy> {
        public:
            static void copy(std::tm &dest, std::chrono::system_clock::time_point const &src) {
                auto t = std::chrono::system_clock::to_time_t(src);
                dest = *std::localtime(&t);
            }
            static void move(std::tm &dest, std::chrono::system_clock::time_point &&src) {
                auto t = std::chrono::system_clock::to_time_t(src);
                dest = *std::localtime(&t);
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<std::chrono::system_clock::time_point,std::tm,ComplexCopy> {
        public:
            static void copy(std::chrono::system_clock::time_point &dest, std::tm const &src) {
                std::tm t = src;
                dest = std::chrono::system_clock::from_time_t(std::mktime(&t));
            }
            static void move(std::chrono::system_clock::time_point &dest, std::tm &&src) {
                dest = std::chrono::system_clock::from_time_t(std::mktime(&src));
            }
        };
        template <class T, std::size_t N, class U, std::size_t M, class ComplexCopy>
        class CopySimpleImpl<std::array<T,N>,std::array<U,M>,ComplexCopy> {
        public:
            static void copy(std::array<T,N> &dest, std::array<U,M> const &src) {
                for (std::size_t ii=0; ii<std::min(N,M); ++ii) {
                    ComplexCopy::template copy<T,U>(dest[ii], src[ii]);
                }
            }
            static void move(std::array<T,N> &dest, std::array<U,M> &&src) {
                for (std::size_t ii=0; ii<std::min(N,M); ++ii) {
                    ComplexCopy::template move<T,U>(dest[ii], std::move(src[ii]));
                }
            }
        };
        template <class T, class U, class ComplexCopy>
        class CopySimpleImpl<std::optional<T>,U,ComplexCopy> {
        public:
            static void copy(std::optional<T> &dest, U const &src) {
                dest = T {};
                ComplexCopy::template copy<T,U>(*dest, src);
            }
            static void move(std::optional<T> &dest, U &&src) {
                dest = T {};
                ComplexCopy::template move<T,U>(*dest, std::move(src));
            }
        };
        template <class T, class U, class ComplexCopy>
        class CopySimpleImpl<T,std::optional<U>,ComplexCopy> {
        public:
            static void copy(T &dest, std::optional<U> const &src) {
                if (src) {
                    ComplexCopy::template copy<T,U>(dest, *src);
                }
            }
            static void move(T &dest, std::optional<U> &&src) {
                if (src) {
                    ComplexCopy::template move<T,U>(dest, std::move(*src));
                }
            }
        };
        template <class T, class U, class ComplexCopy>
        class CopySimpleImpl<std::optional<T>,std::optional<U>,ComplexCopy> {
        public:
            static void copy(std::optional<T> &dest, std::optional<U> const &src) {
                if (src) {
                    dest = T{};
                    ComplexCopy::template copy<T,U>(*dest, *src);
                } else {
                    dest = std::nullopt;
                }
            }
            static void move(std::optional<T> &dest, std::optional<U> &&src) {
                if (src) {
                    dest = T{};
                    ComplexCopy::template move<T,U>(*dest, std::move(*src));
                } else {
                    dest = std::nullopt;
                }
            }
        };
        template <class T, class U, class ComplexCopy>
        class CopyStructure {
        private:
            template <int TFieldCount, int TFieldIndex, int UFieldCount, int UFieldIndex>
            static void copy_internal(T &dest, U const &src) {
                if constexpr (TFieldIndex >= 0 && TFieldIndex < TFieldCount) {
                    if constexpr (UFieldIndex >= 0 && UFieldIndex < UFieldCount) {
                        if constexpr (StructFieldInfo<T>::FIELD_NAMES[TFieldIndex] == StructFieldInfo<U>::FIELD_NAMES[UFieldIndex]) {
                            using F1 = typename StructFieldTypeInfo<T,TFieldIndex>::TheType;
                            auto T::*p1 = StructFieldTypeInfo<T,TFieldIndex>::fieldPointer();
                            using F2 = typename StructFieldTypeInfo<U,UFieldIndex>::TheType;
                            auto U::*p2 = StructFieldTypeInfo<U,UFieldIndex>::fieldPointer();
                            ComplexCopy::template copy<F1,F2>(dest.*p1, src.*p2);
                        }
                        copy_internal<TFieldCount,TFieldIndex,UFieldCount,UFieldIndex+1>(dest, src);
                    } else {
                        copy_internal<TFieldCount,TFieldIndex+1,UFieldCount,0>(dest, src);
                    }
                }
            }
            template <int TFieldCount, int TFieldIndex, int UFieldCount, int UFieldIndex>
            static void move_internal(T &dest, U &&src) {
                if constexpr (TFieldIndex >= 0 && TFieldIndex < TFieldCount) {
                    if constexpr (UFieldIndex >= 0 && UFieldIndex < UFieldCount) {
                        if constexpr (StructFieldInfo<T>::FIELD_NAMES[TFieldIndex] == StructFieldInfo<U>::FIELD_NAMES[UFieldIndex]) {
                            using F1 = typename StructFieldTypeInfo<T,TFieldIndex>::TheType;
                            auto T::*p1 = StructFieldTypeInfo<T,TFieldIndex>::fieldPointer();
                            using F2 = typename StructFieldTypeInfo<U,UFieldIndex>::TheType;
                            auto U::*p2 = StructFieldTypeInfo<U,UFieldIndex>::fieldPointer();
                            ComplexCopy::template move<F1,F2>(dest.*p1, std::move(src.*p2));
                        }
                        move_internal<TFieldCount,TFieldIndex,UFieldCount,UFieldIndex+1>(dest, std::move(src));
                    } else {
                        move_internal<TFieldCount,TFieldIndex+1,UFieldCount,0>(dest, std::move(src));
                    }
                }
            }
        public:
            static void copy(T &dest, U const &src) {
                copy_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0,StructFieldInfo<U>::FIELD_NAMES.size(),0>(dest, src);
            }
            static void move(T &dest, U &&src) {
                move_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0,StructFieldInfo<U>::FIELD_NAMES.size(),0>(dest, std::move(src));
            }
        };
        
        class StructuralCopyImpl {
        public:
            template <class T, class U>
            static void copy(T &dest, U const &src) {
                if constexpr (
                StructFieldInfo<typename GetRef<T>::TheType>::HasGeneratedStructFieldInfo
                &&
                StructFieldInfo<typename GetRef<U>::TheType>::HasGeneratedStructFieldInfo
                ) {
                    CopyStructure<typename GetRef<T>::TheType,typename GetRef<U>::TheType,StructuralCopyImpl>::copy(GetRef<T>::ref(dest), GetRef<U>::constRef(src));
                } else {
                    CopySimpleImpl<typename GetRef<T>::TheType,typename GetRef<U>::TheType,StructuralCopyImpl>::copy(GetRef<T>::ref(dest), GetRef<U>::constRef(src));
                }
            }
            template <class T, class U>
            static void move(T &dest, U &&src) {
                if constexpr (
                StructFieldInfo<typename GetRef<T>::TheType>::HasGeneratedStructFieldInfo
                &&
                StructFieldInfo<typename GetRef<U>::TheType>::HasGeneratedStructFieldInfo
                ) {
                    CopyStructure<typename GetRef<T>::TheType,typename GetRef<U>::TheType,StructuralCopyImpl>::move(GetRef<T>::ref(dest), std::move(GetRef<U>::moveRef(std::move(src))));
                } else {
                    CopySimpleImpl<typename GetRef<T>::TheType,typename GetRef<U>::TheType,StructuralCopyImpl>::move(GetRef<T>::ref(dest), std::move(GetRef<U>::moveRef(std::move(src))));
                }
            }
        };

        template <class A>
        class CountTupleParameter {
        public:
            static constexpr bool IsValid = false;
            static constexpr std::size_t Count = std::numeric_limits<std::size_t>::max();
        };
        template <class... As>
        class CountTupleParameter<std::tuple<As...>> {
        public:
            static constexpr bool IsValid = true;
            static constexpr std::size_t Count = sizeof...(As);
        };

        template <class A>
        class TupleCarCdr {};
        template <>
        class TupleCarCdr<std::tuple<>> {
        public:
            using Car = void;
            using Cdr = void;
        };
        template <class F, class... Rs>
        class TupleCarCdr<std::tuple<F,Rs...>> {
        public:
            using Car = F;
            using Cdr = std::tuple<Rs...>;
        };

        template <class T, class U>
        class FlatCopyImpl {
        private:
            using TL = typename StructFieldFlattenedInfo<T>::TheType;
            using UL = typename StructFieldFlattenedInfo<U>::TheType;
            template <std::size_t Count, std::size_t Index, class RemainingTL, class RemainingUL>
            static void copy_internal(T &dest, U const &src) {
                if constexpr (Count == 0) {
                    return;
                } else if constexpr (Index >= Count) {
                    return;
                } else {
                    GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                        T, typename TupleCarCdr<RemainingTL>::Car
                    >::TheType>::ref(*(StructFieldFlattenedInfoCursorBasedAccess<
                        T, typename TupleCarCdr<RemainingTL>::Car
                    >::valuePointer(dest)))
                    =
                    GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                        U, typename TupleCarCdr<RemainingUL>::Car
                    >::TheType>::constRef(*(StructFieldFlattenedInfoCursorBasedAccess<
                        U, typename TupleCarCdr<RemainingUL>::Car
                    >::constValuePointer(src)));
                    copy_internal<Count,Index+1,typename TupleCarCdr<RemainingTL>::Cdr,typename TupleCarCdr<RemainingUL>::Cdr>(dest, src);
                }
            }
            template <std::size_t Count, std::size_t Index, class RemainingTL, class RemainingUL>
            static void move_internal(T &dest, U &&src) {
                if constexpr (Count == 0) {
                    return;
                } else if constexpr (Index >= Count) {
                    return;
                } else {
                    GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                        T, typename TupleCarCdr<RemainingTL>::Car
                    >::TheType>::ref(*(StructFieldFlattenedInfoCursorBasedAccess<
                        T, typename TupleCarCdr<RemainingTL>::Car
                    >::valuePointer(dest)))
                    =
                    std::move(GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                        U, typename TupleCarCdr<RemainingUL>::Car
                    >::TheType>::moveRef(*(StructFieldFlattenedInfoCursorBasedAccess<
                        U, typename TupleCarCdr<RemainingUL>::Car
                    >::valuePointer(src))));
                    move_internal<Count,Index+1,typename TupleCarCdr<RemainingTL>::Cdr,typename TupleCarCdr<RemainingUL>::Cdr>(dest, std::move(src));
                }
            }
        public:
            static void copy(T &dest, U const &src) {
                static_assert(
                    (CountTupleParameter<TL>::IsValid
                    && CountTupleParameter<UL>::IsValid
                    && CountTupleParameter<TL>::Count==CountTupleParameter<UL>::Count)
                    , "For two structures to be flat-copyable, their flattened field list must have the same size"
                );
                copy_internal<CountTupleParameter<TL>::Count,0,TL,UL>(dest, src);
            };
            static void move(T &dest, U &&src) {
                static_assert(
                    (CountTupleParameter<TL>::IsValid
                    && CountTupleParameter<UL>::IsValid
                    && CountTupleParameter<TL>::Count==CountTupleParameter<UL>::Count)
                    , "For two structures to be flat-copyable, their flattened field list must have the same size"
                );
                move_internal<CountTupleParameter<TL>::Count,0,TL,UL>(dest, std::move(src));
            };
        };
    }

    class StructuralCopy {
    public:
        template <class T, class U, typename=std::enable_if_t<
            StructFieldInfo<T>::HasGeneratedStructFieldInfo
            &&
            StructFieldInfo<U>::HasGeneratedStructFieldInfo
        >>
        static void copy(T &dest, U const &src) {
            internal::StructuralCopyImpl::template copy<T,U>(dest, src);
        }
        template <class T, class U, typename=std::enable_if_t<
            StructFieldInfo<T>::HasGeneratedStructFieldInfo
            &&
            StructFieldInfo<U>::HasGeneratedStructFieldInfo
        >>
        static void move(T &dest, U &&src) {
            internal::StructuralCopyImpl::template move<T,U>(dest, std::move(src));
        }
    };

    class FlatCopy {
    public:
        template <class T, class U, typename=std::enable_if_t<
            StructFieldInfo<T>::HasGeneratedStructFieldInfo
            &&
            StructFieldInfo<U>::HasGeneratedStructFieldInfo
        >>
        static void copy(T &dest, U const &src) {
            internal::FlatCopyImpl<T,U>::copy(dest, src);
        }
        template <class T, class U, typename=std::enable_if_t<
            StructFieldInfo<T>::HasGeneratedStructFieldInfo
            &&
            StructFieldInfo<U>::HasGeneratedStructFieldInfo
        >>
        static void move(T &dest, U &&src) {
            internal::FlatCopyImpl<T,U>::move(dest, std::move(src));
        }
    };

} } } } }
 
#endif