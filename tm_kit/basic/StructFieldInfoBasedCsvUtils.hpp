#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {
    
    namespace internal {
        template <class F>
        class StructFieldIsGoodForCsv {
        public:
            static constexpr bool Value = 
                std::is_arithmetic_v<F>
                || std::is_same_v<F, std::tm>
                || std::is_same_v<F, std::chrono::system_clock::time_point>
                || std::is_same_v<F, std::string>
            ;
            static constexpr bool IsArray = false;
            static constexpr std::size_t ArrayLength = 0;
        };
        template <class X, std::size_t N>
        class StructFieldIsGoodForCsv<std::array<X,N>> {
        public:
            static constexpr bool Value = StructFieldIsGoodForCsv<X>::Value;
            static constexpr bool IsArray = true;
            static constexpr std::size_t ArrayLength = N;
        };
        template <class T, bool IsStructWithFieldInfo=StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        class StructFieldInfoCsvSupportChecker {
        public:
            static constexpr bool IsGoodForCsv = false;
        };
        template <class T>
        class StructFieldInfoCsvSupportChecker<T, true> {
        public:
            template <int FieldCount, int FieldIdx>
            static constexpr checkGood() {
                if constexpr (FieldIdx>=0 && FieldIdx<FieldCount) {
                    if (!StructFieldIsGoodForCsv<typename StructFieldTypeInfo<T,FieldIdx>::TheType>::Value) {
                        return false;
                    } else {
                        return checkGood<FieldCount,FieldIdx+1>();
                    }
                } else {
                    return true;
                }
            }
        public:
            static constexpr bool IsGoodForCsv = checkGood<StructFieldInfo<T>::FIELD_NAMES.size(),0>();
        };
    }

    template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv>>
    class StructFieldInfoBasedSimpleCsvOutput {
    private:
        template <int FieldCount, int FieldIndex>
        static void writeHeader_internal(std::ostream &os) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                if constexpr (FieldIndex != 0) {
                    os << ',';
                }
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (internal::StructFieldIsGoodForCsv<ColType>::IsArray) {
                    for (std::size_t ii=0; ii<internal::StructFieldIsGoodForCsv<ColType>::ArrayLength; ++ii) {
                        if (ii>0) {
                            os << ',';
                        }
                        os << StructFieldInfo<T>::FIELD_NAMES[FieldIndex] << '[' << ii << ']';
                    }
                } else {
                    os << StructFieldInfo<T>::FIELD_NAMES[FieldIndex];
                }
                writeHeader_internal<FieldCount,FieldIndex+1>(os);
            }
        }
        template <int FieldCount, int FieldIndex>
        static void writeData_internal(std::ostream &os, T const &t) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                if constexpr (FieldIndex != 0) {
                    os << ',';
                }
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (std::is_same_v<ColType,std::string>) {
                    os << std::quoted(t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()));
                } else if constexpr (std::is_same_v<ColType,std::tm>) {
                    os << std::put_time(&(t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer())), "%Y-%m-%dT%H:%M:%S");
                } else if constexpr (std::is_same_v<ColType,std::chrono::system_clock::time_point>) {
                    os << infra::withtime_utils::localTimeString(t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()));
                } else {
                    os << t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer());
                }
                writeData_internal<FieldCount,FieldIndex+1>(os, t);
            }
        }
    public:
        static void writeHeader(std::ostream &os) {
            writeHeader_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0>(os);
            os << '\n';
        }
        static void writeData(std::ostream &os, T const &t) {
            writeData_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0>(os, t);
            os << '\n';
        }
    };

} } } } }

#endif