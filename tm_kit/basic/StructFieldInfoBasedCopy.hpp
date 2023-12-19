#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_COPY_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/StructFieldFlattenedInfo.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/DateHolder.hpp>
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
                GetRef<T>::ref(dest) = T(std::move(GetRef<U>::moveRef(std::move(src))));
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
        template <class ComplexCopy>
        class CopySimpleImpl<DateHolder,std::chrono::system_clock::time_point,ComplexCopy> {
        public:
            static void copy(DateHolder &dest, std::chrono::system_clock::time_point const &src) {
                auto t = std::chrono::system_clock::to_time_t(src);
                auto tmVal = std::localtime(&t);
                dest.year = tmVal->tm_year+1900;
                dest.month = tmVal->tm_mon+1;
                dest.day = tmVal->tm_mday;
            }
            static void move(DateHolder &dest, std::chrono::system_clock::time_point &&src) {
                auto t = std::chrono::system_clock::to_time_t(src);
                auto tmVal = std::localtime(&t);
                dest.year = tmVal->tm_year+1900;
                dest.month = tmVal->tm_mon+1;
                dest.day = tmVal->tm_mday;
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<std::chrono::system_clock::time_point,DateHolder,ComplexCopy> {
        public:
            static void copy(std::chrono::system_clock::time_point &dest, DateHolder const &src) {
                std::tm t;
                t.tm_year = (src.year==0?0:(src.year-1900));
                t.tm_mon = (src.month==0?0:(src.month-1));
                t.tm_mday = (src.day==0?1:src.day);
                t.tm_hour = 0;
                t.tm_min = 0;
                t.tm_sec = 0;
                t.tm_isdst = -1;
                dest = std::chrono::system_clock::from_time_t(std::mktime(&t));
            }
            static void move(std::chrono::system_clock::time_point &dest, DateHolder &&src) {
                std::tm t;
                t.tm_year = (src.year==0?0:(src.year-1900));
                t.tm_mon = (src.month==0?0:(src.month-1));
                t.tm_mday = (src.day==0?1:src.day);
                t.tm_hour = 0;
                t.tm_min = 0;
                t.tm_sec = 0;
                t.tm_isdst = -1;
                dest = std::chrono::system_clock::from_time_t(std::mktime(&t));
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<std::tm,DateHolder,ComplexCopy> {
        public:
            static void copy(std::tm &dest, DateHolder const &src) {
                dest.tm_year = (src.year==0?0:(src.year-1900));
                dest.tm_mon = (src.month==0?0:(src.month-1));
                dest.tm_mday = (src.day==0?1:src.day);
                dest.tm_hour = 0;
                dest.tm_min = 0;
                dest.tm_sec = 0;
                dest.tm_isdst = -1;
            }
            static void move(std::tm &dest, DateHolder &&src) {
                dest.tm_year = (src.year==0?0:(src.year-1900));
                dest.tm_mon = (src.month==0?0:(src.month-1));
                dest.tm_mday = (src.day==0?1:src.day);
                dest.tm_hour = 0;
                dest.tm_min = 0;
                dest.tm_sec = 0;
                dest.tm_isdst = -1;
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<DateHolder,std::tm,ComplexCopy> {
        public:
            static void copy(DateHolder &dest, std::tm const &src) {
                dest.year = src.tm_year+1900;
                dest.month = src.tm_mon+1;
                dest.day = src.tm_mday;
            }
            static void move(DateHolder &dest, std::tm &&src) {
                dest.year = src.tm_year+1900;
                dest.month = src.tm_mon+1;
                dest.day = src.tm_mday;
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<uint32_t,DateHolder,ComplexCopy> {
        public:
            static void copy(uint32_t &dest, DateHolder const &src) {
                dest = ((uint32_t) src.year)*1900+((uint32_t) src.month)*100+((uint32_t) src.day);
            }
            static void move(uint32_t &dest, DateHolder &&src) {
                dest = ((uint32_t) src.year)*1900+((uint32_t) src.month)*100+((uint32_t) src.day);
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<DateHolder,uint32_t,ComplexCopy> {
        public:
            static void copy(DateHolder &dest, uint32_t src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
            static void move(DateHolder &dest, uint32_t &&src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<int32_t,DateHolder,ComplexCopy> {
        public:
            static void copy(int32_t &dest, DateHolder const &src) {
                dest = ((int32_t) src.year)*1900+((int32_t) src.month)*100+((int32_t) src.day);
            }
            static void move(int32_t &dest, DateHolder &&src) {
                dest = ((int32_t) src.year)*1900+((int32_t) src.month)*100+((int32_t) src.day);
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<DateHolder,int32_t,ComplexCopy> {
        public:
            static void copy(DateHolder &dest, int32_t src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
            static void move(DateHolder &dest, int32_t &&src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<uint64_t,DateHolder,ComplexCopy> {
        public:
            static void copy(uint64_t &dest, DateHolder const &src) {
                dest = ((uint64_t) src.year)*1900+((uint64_t) src.month)*100+((uint64_t) src.day);
            }
            static void move(uint64_t &dest, DateHolder &&src) {
                dest = ((uint64_t) src.year)*1900+((uint64_t) src.month)*100+((uint64_t) src.day);
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<DateHolder,uint64_t,ComplexCopy> {
        public:
            static void copy(DateHolder &dest, uint64_t src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
            static void move(DateHolder &dest, uint64_t &&src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<int64_t,DateHolder,ComplexCopy> {
        public:
            static void copy(int64_t &dest, DateHolder const &src) {
                dest = ((int64_t) src.year)*1900+((int64_t) src.month)*100+((int64_t) src.day);
            }
            static void move(int64_t &dest, DateHolder &&src) {
                dest = ((int64_t) src.year)*1900+((int64_t) src.month)*100+((int64_t) src.day);
            }
        };
        template <class ComplexCopy>
        class CopySimpleImpl<DateHolder,int64_t,ComplexCopy> {
        public:
            static void copy(DateHolder &dest, int64_t src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
            }
            static void move(DateHolder &dest, int64_t &&src) {
                dest.year = src/10000;
                dest.month = (src%10000)/100;
                dest.day = src%100;
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
        template <std::size_t N, class ComplexCopy>
        class CopySimpleImpl<std::array<char,N>, std::string, ComplexCopy> {
        public:
            static void copy(std::array<char,N> &dest, std::string const &src) {
                std::memset(dest.data(), 0, N);
                std::memcpy(dest.data(), src.data(), std::min(N,src.length()));
            }
            static void move(std::array<char,N> &dest, std::string const &src) {
                std::memset(dest.data(), 0, N);
                std::memcpy(dest.data(), src.data(), std::min(N,src.length()));
            }
        };
        template <std::size_t N, class ComplexCopy>
        class CopySimpleImpl<std::string, std::array<char,N>, ComplexCopy> {
        public:
            static void copy(std::string &dest, std::array<char,N> const &src) {
                std::size_t ii = 0;
                for (; ii<N; ++ii) {
                    if (src[ii] == '\0') {
                        break;
                    }
                }
                dest = std::string(src.data(), ii);
            }
            static void move(std::string &dest, std::array<char,N> const &src) {
                std::size_t ii = 0;
                for (; ii<N; ++ii) {
                    if (src[ii] == '\0') {
                        break;
                    }
                }
                dest = std::string(src.data(), ii);
            }
        };
        template <std::size_t N, class ComplexCopy>
        class CopySimpleImpl<std::array<char,N>, ByteData, ComplexCopy> {
        public:
            static void copy(std::array<char,N> &dest, ByteData const &src) {
                std::memset(dest.data(), 0, N);
                std::memcpy(dest.data(), src.content.data(), std::min(N,src.content.length()));
            }
            static void move(std::array<char,N> &dest, ByteData const &src) {
                std::memset(dest.data(), 0, N);
                std::memcpy(dest.data(), src.content.data(), std::min(N,src.content.length()));
            }
        };
        template <std::size_t N, class ComplexCopy>
        class CopySimpleImpl<ByteData, std::array<char,N>, ComplexCopy> {
        public:
            static void copy(ByteData &dest, std::array<char,N> const &src) {
                dest.content = std::string(src.data(), N);
            }
            static void move(ByteData &dest, std::array<char,N> const &src) {
                dest.content = std::string(src.data(), N);
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
        template <class U, class ComplexCopy>
        class CopySimpleImpl<std::string, U, ComplexCopy> {
        public:
            static void copy(std::string &dest, U const &src) {
                if constexpr (bytedata_utils::IsEnumWithStringRepresentation<U>::value) {
                    std::ostringstream oss;
                    oss << GetRef<U>::constRef(src);
                    dest = oss.str();
                } else {
                    dest = std::string(GetRef<U>::constRef(src));
                }
            }
            static void move(std::string &dest, U &&src) {
                if constexpr (bytedata_utils::IsEnumWithStringRepresentation<U>::value) {
                    std::ostringstream oss;
                    oss << GetRef<U>::constRef(src);
                    dest = oss.str();
                } else {
                    dest = std::string(std::move(GetRef<U>::moveRef(std::move(src))));
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
                            using F2 = typename StructFieldTypeInfo<U,UFieldIndex>::TheType;
                            ComplexCopy::template copy<F1,F2>(
                                StructFieldTypeInfo<T,TFieldIndex>::access(dest)
                                , StructFieldTypeInfo<U,UFieldIndex>::constAccess(src)
                            );
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
                            using F2 = typename StructFieldTypeInfo<U,UFieldIndex>::TheType;
                            ComplexCopy::template move<F1,F2>(
                                StructFieldTypeInfo<T,TFieldIndex>::access(dest)
                                , StructFieldTypeInfo<U,UFieldIndex>::moveAccess(std::move(src))
                            );
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

        class RecursiveSimpleCopy {
        public:
            template <class T, class U>
            static void copy(T &dest, U const &src) {
                CopySimpleImpl<T,U,RecursiveSimpleCopy>::copy(dest,src);
            }
            template <class T, class U>
            static void move(T &dest, U &&src) {
                CopySimpleImpl<T,U,RecursiveSimpleCopy>::move(dest,std::move(src));
            }
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
                    CopySimpleImpl<
                        typename StructFieldFlattenedInfoCursorBasedAccess<
                            T, typename TupleCarCdr<RemainingTL>::Car
                        >::TheType
                        , typename StructFieldFlattenedInfoCursorBasedAccess<
                            U, typename TupleCarCdr<RemainingUL>::Car
                        >::TheType
                        , RecursiveSimpleCopy
                    >::copy(
                        GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                            T, typename TupleCarCdr<RemainingTL>::Car
                        >::TheType>::ref(*(StructFieldFlattenedInfoCursorBasedAccess<
                            T, typename TupleCarCdr<RemainingTL>::Car
                        >::valuePointer(dest)))
                        ,
                        GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                            U, typename TupleCarCdr<RemainingUL>::Car
                        >::TheType>::constRef(*(StructFieldFlattenedInfoCursorBasedAccess<
                            U, typename TupleCarCdr<RemainingUL>::Car
                        >::constValuePointer(src)))
                    );
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
                    CopySimpleImpl<
                        typename StructFieldFlattenedInfoCursorBasedAccess<
                            T, typename TupleCarCdr<RemainingTL>::Car
                        >::TheType
                        , typename StructFieldFlattenedInfoCursorBasedAccess<
                            U, typename TupleCarCdr<RemainingUL>::Car
                        >::TheType
                        , RecursiveSimpleCopy
                    >::move(
                        GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                            T, typename TupleCarCdr<RemainingTL>::Car
                        >::TheType>::ref(*(StructFieldFlattenedInfoCursorBasedAccess<
                            T, typename TupleCarCdr<RemainingTL>::Car
                        >::valuePointer(dest)))
                        ,
                        std::move(GetRef<typename StructFieldFlattenedInfoCursorBasedAccess<
                            U, typename TupleCarCdr<RemainingUL>::Car
                        >::TheType>::moveRef(*(StructFieldFlattenedInfoCursorBasedAccess<
                            U, typename TupleCarCdr<RemainingUL>::Car
                        >::valuePointer(src))))
                    );
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