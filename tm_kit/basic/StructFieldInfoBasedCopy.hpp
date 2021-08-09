#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <chrono>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {

    namespace internal {
        class CopyImpl {
        public:
            template <class T, class U>
            static void copy(T &dest, U const &src);
        };

        template <class T, class U>
        class CopySimpleImpl {
        public:
            static void copy(T &dest, U const &src) {
                dest = T(src);
            }
        };
        template <>
        class CopySimpleImpl<std::tm,std::chrono::system_clock::time_point> {
        public:
            static void copy(std::tm &dest, std::chrono::system_clock::time_point const &src) {
                auto t = std::chrono::system_clock::to_time_t(src);
                dest = *std::localtime(&t);
            }
        };
        template <>
        class CopySimpleImpl<std::chrono::system_clock::time_point,std::tm> {
        public:
            static void copy(std::chrono::system_clock::time_point &dest, std::tm const &src) {
                std::tm t = src;
                dest = std::chrono::system_clock::from_time_t(std::mktime(&t));
            }
        };
        template <class T, std::size_t N, class U, std::size_t M>
        class CopySimpleImpl<std::array<T,N>,std::array<U,M>> {
        public:
            static void copy(std::array<T,N> &dest, std::array<U,M> const &src) {
                for (std::size_t ii=0; ii<std::min(N,M); ++ii) {
                    CopyImpl::template copy<T,U>(dest[ii], src[ii]);
                }
            }
        };
        template <class T, class U>
        class CopySimpleImpl<std::optional<T>,U> {
        public:
            static void copy(std::optional<T> &dest, U const &src) {
                dest = T {};
                CopyImpl::template copy<T,U>(*dest, src);
            }
        };
        template <class T, class U>
        class CopySimpleImpl<T,std::optional<U>> {
        public:
            static void copy(T &dest, std::optional<U> const &src) {
                if (src) {
                    CopyImpl::template copy<T,U>(dest, *src);
                }
            }
        };
        template <class T, class U>
        class CopySimpleImpl<std::optional<T>,std::optional<U>> {
        public:
            static void copy(std::optional<T> &dest, std::optional<U> const &src) {
                if (src) {
                    dest = T{};
                    CopyImpl::template copy<T,U>(*dest, *src);
                } else {
                    dest = std::nullopt;
                }
            }
        };
        template <class T, class U>
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
                            CopyImpl::template copy<F1,F2>(dest.*p1, src.*p2);
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
        
        template <class T, class U>
        void CopyImpl::copy(T &dest, U const &src) {
            if constexpr (
                StructFieldInfo<T>::HasGeneratedStructFieldInfo
                &&
                StructFieldInfo<U>::HasGeneratedStructFieldInfo
            ) {
                CopyStructure<T,U>::copy(dest, src);
            } else {
                CopySimpleImpl<T,U>::copy(dest, src);
            }
        }
    }

    template <class T, class U, typename=std::enable_if_t<
        StructFieldInfo<T>::HasGeneratedStructFieldInfo
        &&
        StructFieldInfo<U>::HasGeneratedStructFieldInfo
    >>
    class Copy {
    public:
        static void copy(T &dest, U const &src) {
            StructFieldInfoBasedInitializer<T>::initialize(dest);
            internal::CopyImpl::template copy<T,U>(dest, src);
        }
    };

} } } } }
 
#endif