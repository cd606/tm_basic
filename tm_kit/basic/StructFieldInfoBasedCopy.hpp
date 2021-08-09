#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <chrono>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {

    namespace internal {
        
        template <class T, class U, class ComplexCopy>
        class CopySimpleImpl {
        public:
            static void copy(T &dest, U const &src) {
                dest = T(src);
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<std::tm,std::chrono::system_clock::time_point,ComplexCopy> {
        public:
            static void copy(std::tm &dest, std::chrono::system_clock::time_point const &src) {
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
        };
        template <class T, std::size_t N, class U, std::size_t M, class ComplexCopy>
        class CopySimpleImpl<std::array<T,N>,std::array<U,M>,ComplexCopy> {
        public:
            static void copy(std::array<T,N> &dest, std::array<U,M> const &src) {
                for (std::size_t ii=0; ii<std::min(N,M); ++ii) {
                    ComplexCopy::template copy<T,U>(dest[ii], src[ii]);
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
        };
        template <class T, class U, class ComplexCopy>
        class CopySimpleImpl<T,std::optional<U>,ComplexCopy> {
        public:
            static void copy(T &dest, std::optional<U> const &src) {
                if (src) {
                    ComplexCopy::template copy<T,U>(dest, *src);
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
        public:
            static void copy(T &dest, U const &src) {
                copy_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0,StructFieldInfo<U>::FIELD_NAMES.size(),0>(dest, src);
            }
        };
        
        class StructuralCopyImpl {
        public:
            template <class T, class U>
            static void copy(T &dest, U const &src) {
                if constexpr (
                StructFieldInfo<T>::HasGeneratedStructFieldInfo
                &&
                StructFieldInfo<U>::HasGeneratedStructFieldInfo
                ) {
                    CopyStructure<T,U,StructuralCopyImpl>::copy(dest, src);
                } else {
                    CopySimpleImpl<T,U,StructuralCopyImpl>::copy(dest, src);
                }
            }
        };

        class CopyOneLevelFlatStructure {
        private:
            template <class T, class U, class ComplexCopy, int TFieldCount, int TFieldIndex, int TFieldSubIndex, int UFieldCount, int UFieldIndex, int UFieldSubIndex>
            static void copy_internal(T &dest, U const &src) {
                if constexpr (TFieldIndex >= 0 && TFieldIndex < TFieldCount && UFieldIndex >= 0 && UFieldIndex < UFieldCount) {
                    using F1 = typename StructFieldTypeInfo<T,TFieldIndex>::TheType;
                    auto T::*p1 = StructFieldTypeInfo<T,TFieldIndex>::fieldPointer();
                    using F2 = typename StructFieldTypeInfo<U,UFieldIndex>::TheType;
                    auto U::*p2 = StructFieldTypeInfo<U,UFieldIndex>::fieldPointer();
                    if constexpr (StructFieldInfo<F1>::HasGeneratedStructFieldInfo) {
                        if constexpr (StructFieldInfo<F2>::HasGeneratedStructFieldInfo) {
                            constexpr int x = std::min(StructFieldInfo<F1>::FIELD_NAMES.size()-TFieldSubIndex,StructFieldInfo<F2>::FIELD_NAMES.size()-UFieldSubIndex);
                            copy_internal<F1,F2,ComplexCopy,x,TFieldSubIndex,0,x,UFieldSubIndex,0>(dest.*p1, src.*p2);
                            if constexpr (TFieldSubIndex+x >= StructFieldInfo<F1>::FIELD_NAMES.size()) {
                                if constexpr (UFieldSubIndex+x >= StructFieldInfo<F2>::FIELD_NAMES.size()) {
                                    copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex+1,0,UFieldCount,UFieldIndex+1,0>(dest,src);
                                } else {
                                    copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex+1,0,UFieldCount,UFieldIndex,UFieldSubIndex+x>(dest,src);
                                }
                            } else {
                                if constexpr (UFieldSubIndex+x >= StructFieldInfo<F2>::FIELD_NAMES.size()) {
                                    copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex,TFieldSubIndex+x,UFieldCount,UFieldIndex+1,0>(dest,src);
                                } else {
                                    copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex,UFieldSubIndex+x,UFieldCount,UFieldIndex,UFieldSubIndex+x>(dest,src);
                                }
                            }
                        } else {
                            using SubF = typename StructFieldTypeInfo<F1,TFieldSubIndex>::TheType;
                            auto F1::*subP = StructFieldTypeInfo<F1,TFieldSubIndex>::fieldPointer();
                            ComplexCopy::template copy<SubF,F2>(dest.*p1.*subP, src.*p2);
                            if constexpr (TFieldSubIndex+1 >= StructFieldInfo<F1>::FIELD_NAMES.size()) {
                                copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex+1,0,UFieldCount,UFieldIndex+1,0>(dest,src);
                            } else {
                                copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex,TFieldSubIndex+1,UFieldCount,UFieldIndex+1,0>(dest,src);
                            }
                        }
                    } else {
                        if constexpr (StructFieldInfo<F2>::HasGeneratedStructFieldInfo) {
                            using SubF = typename StructFieldTypeInfo<F2,UFieldSubIndex>::TheType;
                            auto F2::*subP = StructFieldTypeInfo<F2,UFieldSubIndex>::fieldPointer();
                            ComplexCopy::template copy<F1,SubF>(dest.*p1, src.*p2.*subP);
                            if constexpr (UFieldSubIndex+1 >= StructFieldInfo<F2>::FIELD_NAMES.size()) {
                                copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex+1,0,UFieldCount,UFieldIndex+1,0>(dest,src);
                            } else {
                                copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex+1,0,UFieldCount,UFieldIndex,UFieldSubIndex+1>(dest,src);
                            }
                        } else {
                            ComplexCopy::template copy<F1,F2>(dest.*p1, src.*p2);
                            copy_internal<T,U,ComplexCopy,TFieldCount,TFieldIndex+1,0,UFieldCount,UFieldIndex+1,0>(dest,src);
                        }
                    }
                }
            }
        public:
            template <class T, class U, class ComplexCopy>
            static void copy(T &dest, U const &src) {
                copy_internal<T,U,ComplexCopy,StructFieldInfo<T>::FIELD_NAMES.size(),0,0,StructFieldInfo<U>::FIELD_NAMES.size(),0,0>(dest, src);
            }
        };

        class OneLevelFlatCopyImpl {
        public:
            template <class T, class U>
            static void copy(T &dest, U const &src) {
                if constexpr (
                StructFieldInfo<T>::HasGeneratedStructFieldInfo
                &&
                StructFieldInfo<U>::HasGeneratedStructFieldInfo
                ) {
                    CopyOneLevelFlatStructure::copy<T,U,OneLevelFlatCopyImpl>(dest, src);
                } else {
                    CopySimpleImpl<T,U,OneLevelFlatCopyImpl>::copy(dest, src);
                }
            }
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
    };

    class OneLevelFlatCopy {
    public:
        template <class T, class U, typename=std::enable_if_t<
            StructFieldInfo<T>::HasGeneratedStructFieldInfo
            &&
            StructFieldInfo<U>::HasGeneratedStructFieldInfo
        >>
        static void copy(T &dest, U const &src) {
            internal::OneLevelFlatCopyImpl::template copy<T,U>(dest, src);
        }
    };

} } } } }
 
#endif