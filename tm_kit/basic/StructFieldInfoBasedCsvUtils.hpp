#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/DateHolder.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <chrono>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {
    
    namespace internal {
        
        template <class T>
        class CsvSingleLayerWrapperHelper {
        public:
            static constexpr bool Value = false;
            using UnderlyingType = T;
            static T &ref(T &t) {
                return t;
            }
            static T const &constRef(T const &t) {
                return t;
            }
        };
        template <class X>
        class CsvSingleLayerWrapperHelper<SingleLayerWrapper<X>> {
        public:
            static constexpr bool Value = true;
            using UnderlyingType = typename CsvSingleLayerWrapperHelper<X>::UnderlyingType;
            static auto &ref(SingleLayerWrapper<X> &t) {
                return CsvSingleLayerWrapperHelper<X>::ref(t.value);
            }
            static auto &constRef(SingleLayerWrapper<X> const &t) {
                return CsvSingleLayerWrapperHelper<X>::constRef(t.value);
            }
        };
        template <int32_t N, class X>
        class CsvSingleLayerWrapperHelper<SingleLayerWrapperWithID<N,X>> {
        public:
            static constexpr bool Value = true;
            using UnderlyingType = typename CsvSingleLayerWrapperHelper<X>::UnderlyingType;
            static auto &ref(SingleLayerWrapperWithID<N,X> &t) {
                return CsvSingleLayerWrapperHelper<X>::ref(t.value);
            }
            static auto &constRef(SingleLayerWrapperWithID<N,X> const &t) {
                return CsvSingleLayerWrapperHelper<X>::constRef(t.value);
            }
        };
        template <typename M, class X>
        class CsvSingleLayerWrapperHelper<SingleLayerWrapperWithTypeMark<M,X>> {
        public:
            static constexpr bool Value = true;
            using UnderlyingType = typename CsvSingleLayerWrapperHelper<X>::UnderlyingType;
            static auto &ref(SingleLayerWrapperWithTypeMark<M,X> &t) {
                return CsvSingleLayerWrapperHelper<X>::ref(t.value);
            }
            static auto &constRef(SingleLayerWrapperWithTypeMark<M,X> const &t) {
                return CsvSingleLayerWrapperHelper<X>::constRef(t.value);
            }
        };

        template <class F>
        constexpr bool is_simple_csv_field_v = 
            std::is_arithmetic_v<F>
            || std::is_same_v<F, bool>
            || std::is_same_v<F, std::tm>
            || std::is_same_v<F, DateHolder>
            || std::is_same_v<F, std::chrono::system_clock::time_point>
            || std::is_same_v<F, std::string>
            || std::is_empty_v<F>
            || ConvertibleWithString<F>::value
            || (CsvSingleLayerWrapperHelper<F>::Value && is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>)
        ;
        template <class T>
        class ArrayAndOptionalChecker {
        public:
            static constexpr bool IsArray = false;
            static constexpr bool IsOptional = false;
            static constexpr std::size_t ArrayLength = 0;
            static constexpr bool IsCharArray = false;
            using BaseType = T;
        };
        template <class X, std::size_t N>
        class ArrayAndOptionalChecker<std::array<X,N>> {
        public:
            static constexpr bool IsArray = true;
            static constexpr bool IsOptional = false;
            static constexpr std::size_t ArrayLength = N;
            static constexpr bool IsCharArray = false;
            using BaseType = X;
        };
        template <std::size_t N>
        class ArrayAndOptionalChecker<std::array<char,N>> {
        public:
            static constexpr bool IsArray = true;
            static constexpr bool IsOptional = false;
            static constexpr std::size_t ArrayLength = N;
            static constexpr bool IsCharArray = true;
            using BaseType = char;
        };
        template <class X>
        class ArrayAndOptionalChecker<std::optional<X>> {
        public:
            static constexpr bool IsArray = false;
            static constexpr bool IsOptional = true;
            static constexpr std::size_t ArrayLength = 0;
            static constexpr bool IsCharArray = false;
            using BaseType = X;
        };

        template <class T, bool IsStructWithFieldInfo=StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::HasGeneratedStructFieldInfo>
        class StructFieldInfoCsvSupportChecker {
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForCsv = false;
            static constexpr std::size_t CsvFieldCount = 0;
        };
        template <class T>
        class StructFieldInfoCsvSupportChecker<T, false> {
        private:
            static constexpr bool fieldIsGood() {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>) {
                    return true;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsCharArray) {
                    return true;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsArray) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::IsGoodForCsv;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsOptional) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::IsGoodForCsv;
                } else if constexpr (CsvSingleLayerWrapperHelper<T>::Value) {
                    return StructFieldInfoCsvSupportChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsGoodForCsv;
                } else {
                    return false;
                }
            }
            static constexpr std::size_t fieldCount() {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsCharArray) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsArray) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::CsvFieldCount*ArrayAndOptionalChecker<T>::ArrayLength;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsOptional) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::CsvFieldCount;
                } else if constexpr (CsvSingleLayerWrapperHelper<T>::Value) {
                    return StructFieldInfoCsvSupportChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::CsvFieldCount;
                } else {
                    return 0;
                }
            }
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForCsv = fieldIsGood();
            static constexpr std::size_t CsvFieldCount = fieldCount();
        };
        template <>
        class StructFieldInfoCsvSupportChecker<std::tuple<>, false> {
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForCsv = true;
            static constexpr std::size_t CsvFieldCount = 0;
        };
        template <class T>
        class StructFieldInfoCsvSupportChecker<std::tuple<T>, false> {
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForCsv = StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv;
            static constexpr std::size_t CsvFieldCount = StructFieldInfoCsvSupportChecker<T>::CsvFieldCount;
        };
        template <class FirstT, class... RestTs>
        class StructFieldInfoCsvSupportChecker<std::tuple<FirstT,RestTs...>, false> {
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForCsv = 
                StructFieldInfoCsvSupportChecker<FirstT>::IsGoodForCsv
                && StructFieldInfoCsvSupportChecker<std::tuple<RestTs...>>::IsGoodForCsv;
            static constexpr std::size_t CsvFieldCount = 
                StructFieldInfoCsvSupportChecker<FirstT>::CsvFieldCount
                +StructFieldInfoCsvSupportChecker<std::tuple<RestTs...>>::CsvFieldCount;
        };
        template <class T>
        class StructFieldInfoCsvSupportCheckerHelper {
        public:
            template <class F>
            static constexpr bool fieldIsGood() {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    return true;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {
                    return true;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::IsGoodForCsv;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::IsGoodForCsv;
                } else {
                    return StructFieldInfoCsvSupportChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsGoodForCsv;
                }
            }
            template <class F>
            static constexpr std::size_t fieldCount() {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::CsvFieldCount*ArrayAndOptionalChecker<F>::ArrayLength;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::CsvFieldCount;
                } else {
                    return StructFieldInfoCsvSupportChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::CsvFieldCount;
                }
            }

            template <int FieldCount, int FieldIdx>
            static constexpr bool checkGood() {
                if constexpr (FieldIdx>=0 && FieldIdx<FieldCount) {
                    if constexpr (!fieldIsGood<typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIdx>::TheType>()) {
                        return false;
                    } else {
                        return checkGood<FieldCount,FieldIdx+1>();
                    }
                } else {
                    return true;
                }
            }
            template <int FieldCount, int FieldIdx, std::size_t SoFar>
            static constexpr std::size_t totalFieldCount() {
                if constexpr (FieldIdx>=0 && FieldIdx<FieldCount) {
                    return totalFieldCount<FieldCount,FieldIdx+1,SoFar+fieldCount<typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIdx>::TheType>()>();
                } else {
                    return SoFar;
                }
            }
        };
        template <class T>
        class StructFieldInfoCsvSupportChecker<T, true> {
        private:
            template <class F>
            static constexpr bool fieldIsGood() {
                return StructFieldInfoCsvSupportCheckerHelper<T>::template fieldIsGood<F>();
            }
            template <class F>
            static constexpr std::size_t fieldCount() {
                return StructFieldInfoCsvSupportCheckerHelper<T>::template fieldCount<F>();
            }
            template <int FieldCount, int FieldIdx>
            static constexpr bool checkGood() {
                return StructFieldInfoCsvSupportCheckerHelper<T>::template checkGood<FieldCount, FieldIdx>();
            }
            template <int FieldCount, int FieldIdx, std::size_t SoFar>
            static constexpr std::size_t totalFieldCount() {
                return StructFieldInfoCsvSupportCheckerHelper<T>::template totalFieldCount<FieldCount, FieldIdx, SoFar>();
            }
        public:
            static constexpr bool IsComposite = true;
            static constexpr bool IsGoodForCsv = StructFieldInfoCsvSupportCheckerHelper<T>::template checkGood<StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>();
            static constexpr std::size_t CsvFieldCount = StructFieldInfoCsvSupportCheckerHelper<T>::template totalFieldCount<StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0,0>();
        };

        template <class T>
        struct IsTuple {
            static constexpr bool Value = false;
        };
        template <class... Ts>
        struct IsTuple<std::tuple<Ts...>> {
            static constexpr bool Value = true;
        };

        class StructFieldInfoBasedSimpleCsvOutputImpl {
        private:
            template <class T>
            static void collectTupleFieldName(T *, std::string const &/*prefix*/, std::string const &/*thisField*/, std::vector<std::string> &/*output*/, int /*index*/) {
            }
            static void collectTupleFieldName(std::tuple<> *, std::string const &/*prefix*/, std::string const &/*thisField*/, std::vector<std::string> &/*output*/, int /*index*/) {
            }
            template <class FirstT, class... RestTs>
            static void collectTupleFieldName(std::tuple<FirstT,RestTs...> *, std::string const &prefix, std::string const &thisField, std::vector<std::string> &output, int index) {
                collectSingleFieldName<FirstT>(prefix, thisField+"."+std::to_string(index), output);
                collectTupleFieldName(
                    (std::tuple<RestTs...> *) nullptr 
                    , prefix
                    , thisField
                    , output
                    , index+1
                );
            }
            template <class F>
            static void collectSingleFieldName(std::string const &prefix, std::string const &thisField, std::vector<std::string> &output) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    output.push_back(prefix+thisField);
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {
                    output.push_back(prefix+thisField);
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    auto arrName = prefix+thisField;
                    if (boost::ends_with(arrName, ".")) {
                        arrName = arrName.substr(0, arrName.length()-1);
                    }
                    for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                        if constexpr (is_simple_csv_field_v<BT>) {
                            output.push_back(arrName+"["+std::to_string(ii)+"]");
                        } else {
                            collectSingleFieldName<BT>(
                                arrName+"["+std::to_string(ii)+"]."
                                , ""
                                , output
                            );
                        }
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    auto optionName = prefix+thisField;
                    if (boost::ends_with(optionName, ".")) {
                        optionName = optionName.substr(0, optionName.length()-1);
                    }
                    if constexpr (is_simple_csv_field_v<BT>) {
                        output.push_back(optionName);
                    } else {
                        collectSingleFieldName<BT>(
                            optionName+"."
                            , ""
                            , output
                        );
                    }
                } else if constexpr (IsTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::Value) {
                    collectTupleFieldName(
                        (typename CsvSingleLayerWrapperHelper<F>::UnderlyingType *) nullptr
                        , prefix, thisField, output
                        , 0
                    );
                } else {
                    auto fieldName = prefix+thisField;
                    if (boost::ends_with(fieldName, ".")) {
                        fieldName = fieldName.substr(0, fieldName.length()-1);
                    }
                    StructFieldInfoBasedSimpleCsvOutputImpl::collectFieldNames<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>(
                        fieldName+"."
                        , output
                    );
                }
            }
            template <class T, int FieldCount, int FieldIndex, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void collectFieldNames_internal(std::string const &prefix, std::vector<std::string> &output) {
                if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    collectSingleFieldName<F>(prefix, std::string(StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES[FieldIndex]), output);
                    collectFieldNames_internal<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(prefix, output);
                }
            }
            template <class ColType>
            static void writeSimpleField_internal(std::ostream &os, ColType const &x) {
                if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::string>) {
                    os << std::quoted(CsvSingleLayerWrapperHelper<ColType>::constRef(x));
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,bool>) {
                    os << (CsvSingleLayerWrapperHelper<ColType>::constRef(x)?"true":"false");
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::tm>) {
                    os << std::put_time(&CsvSingleLayerWrapperHelper<ColType>::constRef(x), "%Y-%m-%dT%H:%M:%S");
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,DateHolder>) {
                    os << CsvSingleLayerWrapperHelper<ColType>::constRef(x);
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::chrono::system_clock::time_point>) {
                    os << infra::withtime_utils::localTimeString(CsvSingleLayerWrapperHelper<ColType>::constRef(x));
                } else if constexpr (std::is_empty_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>) {
                    //do nothing
                } else if constexpr (ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::value) {
                    os << std::quoted(ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::toString(CsvSingleLayerWrapperHelper<ColType>::constRef(x)));
                } else {
                    os << CsvSingleLayerWrapperHelper<ColType>::constRef(x);
                }
            }
            template <class ColType>
            static void outputSimpleFieldToReceiver_internal(std::string const &name, ColType const &x, std::function<void(std::string const &, std::string const &)> const &receiver) {
                if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::string>) {
                    receiver(name, CsvSingleLayerWrapperHelper<ColType>::constRef(x));
                } else if constexpr (ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::value) {
                    receiver(name, ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::toString(CsvSingleLayerWrapperHelper<ColType>::constRef(x)));
                } else if constexpr (std::is_empty_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>) {
                    //do nothing
                } else {
                    std::ostringstream oss;
                    writeSimpleField_internal<ColType>(oss, x);
                    receiver(name, oss.str());
                }
            }
            template <class T, std::size_t Index>
            static void writeTupleField(std::ostream &os, T const &f) {
                if constexpr (Index<std::tuple_size_v<T>) {
                    if constexpr (Index > 0) {
                        os << ',';
                    }
                    writeSingleField(os, std::get<Index>(f));
                    writeTupleField<T,Index+1>(os, f);
                }
            }
            template <class T, std::size_t Index>
            static void outputTupleFieldToReceiver(std::string &name, T const &f, std::function<void(std::string const &, std::string const &)> const &receiver) {
                if constexpr (Index<std::tuple_size_v<T>) {
                    outputSingleFieldToReceiver(name+"."+std::to_string(Index), std::get<Index>(f), receiver);
                    outputTupleFieldToReceiver<T,Index+1>(name, f, receiver);
                }
            }
        public:
            template <class F>
            static void writeSingleField(std::ostream &os, F const &f) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    writeSimpleField_internal<F>(os, f);
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {
                    char const *p = CsvSingleLayerWrapperHelper<F>::constRef(f).data();
                    std::size_t ii = 0;
                    for (; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                        if (p[ii] == '\0') {
                            break;
                        }
                    }
                    os << std::quoted(std::string_view(
                        p
                        , ii
                    ));
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                        if (ii>0) {
                            os << ',';
                        }
                        if constexpr (is_simple_csv_field_v<BT>) {
                            writeSimpleField_internal<BT>(os, (CsvSingleLayerWrapperHelper<F>::constRef(f))[ii]);
                        } else {
                            writeSingleField<BT>(os, (CsvSingleLayerWrapperHelper<F>::constRef(f))[ii]);
                        }
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        if (f) {
                            writeSimpleField_internal<BT>(os, *(CsvSingleLayerWrapperHelper<F>::constRef(f)));
                        }
                    } else {
                        if (f) {
                            writeSingleField<BT>(
                                os, *(CsvSingleLayerWrapperHelper<F>::constRef(f))
                            ); 
                        } else {
                            for (std::size_t ii=0; ii<StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::CsvFieldCount-1; ++ii) {
                                os << ',';
                            }
                        }
                    }
                } else if constexpr (IsTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::Value) {
                    writeTupleField<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType, 0>(os, CsvSingleLayerWrapperHelper<F>::constRef(f));
                } else {
                    StructFieldInfoBasedSimpleCsvOutputImpl::writeData<F>(
                        os, f
                    );
                }
            }
            template <class F>
            static void outputSingleFieldToReceiver(std::string const &name, F const &f, std::function<void(std::string const &, std::string const &)> const &receiver) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    outputSimpleFieldToReceiver_internal<F>(name, f, receiver);
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {
                    char const *p = CsvSingleLayerWrapperHelper<F>::constRef(f).data();
                    std::size_t ii = 0;
                    for (; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                        if (p[ii] == '\0') {
                            break;
                        }
                    }
                    receiver(
                        name
                        , std::string(
                            p
                            , ii
                        )
                    );
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                        if constexpr (is_simple_csv_field_v<BT>) {
                            outputSimpleFieldToReceiver_internal<BT>(name+"["+std::to_string(ii)+"]", (CsvSingleLayerWrapperHelper<F>::constRef(f))[ii], receiver);
                        } else {
                            outputSingleFieldToReceiver<BT>(name+"["+std::to_string(ii)+"]", (CsvSingleLayerWrapperHelper<F>::constRef(f))[ii], receiver);
                        }
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        if (f) {
                            outputSimpleFieldToReceiver_internal<BT>(name, *(CsvSingleLayerWrapperHelper<F>::constRef(f)), receiver);
                        }
                    } else {
                        if (f) {
                            outputSingleFieldToReceiver<BT>(
                                name, *(CsvSingleLayerWrapperHelper<F>::constRef(f)), receiver
                            ); 
                        }
                    }
                } else if constexpr (IsTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::Value) {
                    outputTupleFieldToReceiver<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType, 0>(name, CsvSingleLayerWrapperHelper<F>::constRef(f), receiver);
                } else {
                    StructFieldInfoBasedSimpleCsvOutputImpl::outputNameValuePairs<F>(
                        name, f, receiver
                    );
                }
            }
        private:
            template <class T, int FieldCount, int FieldIndex, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void writeData_internal(std::ostream &os, T const &t) {
                if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                    if constexpr (FieldIndex != 0) {
                        os << ',';
                    }
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    writeSingleField<F>(os, StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::constAccess(CsvSingleLayerWrapperHelper<T>::constRef(t)));
                    writeData_internal<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(os, (CsvSingleLayerWrapperHelper<T>::constRef(t)));
                }
            }
            template <class T, int FieldCount, int FieldIndex, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void outputNameValuePairs_internal(std::string const &prefix, T const &t, std::function<void(std::string const &, std::string const &)> const &receiver) {
                if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto s = std::string(StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES[FieldIndex]);
                    if (prefix != "") {
                        s = prefix+"."+s;
                    }
                    outputSingleFieldToReceiver<F>(s, StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::constAccess(CsvSingleLayerWrapperHelper<T>::constRef(t)), receiver);
                    outputNameValuePairs_internal<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(prefix, CsvSingleLayerWrapperHelper<T>::constRef(t), receiver);
                }
            }
        public:
            template <class T, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void collectFieldNames(std::string const &prefix, std::vector<std::string> &output) {
                collectFieldNames_internal<T,StructFieldInfo<T>::FIELD_NAMES.size(),0>(prefix, output);
            }
            template <class T, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv>>
            static void writeHeader(std::ostream &os) {
                std::vector<std::string> fieldNames;
                collectFieldNames<T>("", fieldNames);
                bool begin = true;
                for (auto const &s : fieldNames) {
                    if (!begin) {
                        os << ',';
                    }
                    begin = false;
                    os << s;
                }
            }
            template <class T, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void writeData(std::ostream &os, T const &t) {
                writeData_internal<T,StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(os, t);
            }
            template <class T, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void outputNameValuePairs(std::string const &prefix, T const &t, std::function<void(std::string const &, std::string const &)> const &receiver) {
                outputNameValuePairs_internal<T,StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(prefix, t, receiver);
            }
        };
    }

    template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && internal::StructFieldInfoCsvSupportChecker<T>::IsComposite>>
    class StructFieldInfoBasedSimpleCsvOutput {
    public:
        static void collectFieldNames(std::vector<std::string> &output) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::collectFieldNames<T>("", output);
        }
        static void writeHeader(std::ostream &os) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeHeader<T>(os);
            os << '\n';
        }
        static void writeHeaderNoNewLine(std::ostream &os) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeHeader<T>(os);
        }
        static void writeData(std::ostream &os, T const &t) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeData<T>(os, t);
            os << '\n';
        }
        static void writeDataNoNewLine(std::ostream &os, T const &t) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeData<T>(os, t);
        }
        template <class Iter>
        static void writeDataCollection(std::ostream &os, Iter begin, Iter end, bool dontWriteHeader=false) {
            if (!dontWriteHeader) {
                writeHeader(os);
            }
            for (Iter iter = begin; iter != end; ++iter) {
                writeData(os, *iter);
            }
        }
        static void outputNameValuePairs(T const &t, std::function<void(std::string const &, std::string const &)> const &receiver) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::outputNameValuePairs<T>("", t, receiver);
        }
    };
    //This is a helper class where we may want to output in csv format
    //a single value (especially if that value is array or tuple)
    class StructFieldInfoBasedSimpleCsvOutput_SingleValue {
    public:
        template <class T>
        static void writeData(std::ostream &os, T const &t) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeSingleField<T>(os, t);
            os << '\n';
        }
        template <class T>
        static void writeDataNoNewLine(std::ostream &os, T const &t) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeSingleField<T>(os, t);
        }
        template <class T>
        static void outputNameValuePairs(T const &t, std::function<void(std::string const &, std::string const &)> const &receiver) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::outputNameValuePairs<T>("", t, receiver);
        }
    };
    
    enum class StructFieldInfoBasedCsvInputOption {
        IgnoreHeader 
        , HasNoHeader
        , UseHeaderAsDict
    };

    namespace internal {
        class StructFieldInfoBasedSimpleCsvInputImpl {
        private:
            template <class ColType>
            static bool parseSimpleField_internal(std::string_view const &s, ColType &x) {
                if (s == "") {
                    if constexpr (std::is_empty_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>) {
                        return true;
                    } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::string>) {
                        return true;
                    } else if constexpr (ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::value) {
                        CsvSingleLayerWrapperHelper<ColType>::ref(x) = ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::fromString("");
                        return true;
                    } else {
                        return false;
                    }
                }
                if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::string>) {
                    if (s.length() > 0) {
                        if (s[0] == '"') {
#ifdef _MSC_VER
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                                >> std::quoted(CsvSingleLayerWrapperHelper<ColType>::ref(x));
                        } else {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = std::string(s);
                        }
                    } else {
                        CsvSingleLayerWrapperHelper<ColType>::ref(x) = "";
                    }
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,bool>) {
                    if (s[0] == '"') {
                        std::string unquoted;
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                            >> std::quoted(unquoted);
                        boost::to_lower(unquoted);
                        if (unquoted == "true" || unquoted == "yes") {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = true;
                        } else {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = false;
                        }
                    } else {
                        std::string ss = std::string(s);
                        boost::to_lower(ss);
                        if (ss == "true" || ss == "yes") {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = true;
                        } else {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = false;
                        }
                    }
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::tm>) {
                    if (s[0] == '"') {
                        std::string unquoted;
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                            >> std::quoted(unquoted);
                        boost::erase_all(unquoted, ",");
                        std::istringstream(unquoted)
                            >> std::get_time(&CsvSingleLayerWrapperHelper<ColType>::ref(x)
                                , ((s.length()==8)?"%Y%m%d":((s.length()==10)?"%Y-%m-%d":"%Y-%m-%dT%H:%M:%S")));
                    } else {
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                            >> std::get_time(&CsvSingleLayerWrapperHelper<ColType>::ref(x)
                                , ((s.length()==8)?"%Y%m%d":((s.length()==10)?"%Y-%m-%d":"%Y-%m-%dT%H:%M:%S")));
                    }
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,DateHolder>) {
                    if (s[0] == '"') {
                        std::string unquoted;
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                            >> std::quoted(unquoted);
                        boost::erase_all(unquoted, ",");
                        auto &x1 = CsvSingleLayerWrapperHelper<ColType>::ref(x);
                        if (unquoted.length() == 8) {
                            x1.year = (uint16_t) std::stoi(unquoted.substr(0,4));
                            x1.month = (uint8_t) std::stoi(unquoted.substr(4,2));
                            x1.day = (uint8_t) std::stoi(unquoted.substr(6,2));
                        } else if (unquoted.length() >= 10) {
                            x1.year = (uint16_t) std::stoi(unquoted.substr(0,4));
                            x1.month = (uint8_t) std::stoi(unquoted.substr(5,2));
                            x1.day = (uint8_t) std::stoi(unquoted.substr(8,2));
                        } else {
                            x1.year = 0;
                            x1.month = 0;
                            x1.day = 0;
                        }
                    } else {
                        std::string unquoted = std::string(s);
                        auto &x1 = CsvSingleLayerWrapperHelper<ColType>::ref(x);
                        if (unquoted.length() == 8) {
                            x1.year = (uint16_t) std::stoi(unquoted.substr(0,4));
                            x1.month = (uint8_t) std::stoi(unquoted.substr(4,2));
                            x1.day = (uint8_t) std::stoi(unquoted.substr(6,2));
                        } else if (unquoted.length() >= 10) {
                            x1.year = (uint16_t) std::stoi(unquoted.substr(0,4));
                            x1.month = (uint8_t) std::stoi(unquoted.substr(5,2));
                            x1.day = (uint8_t) std::stoi(unquoted.substr(8,2));
                        } else {
                            x1.year = 0;
                            x1.month = 0;
                            x1.day = 0;
                        }
                    }
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::chrono::system_clock::time_point>) {
                    if (s.length() > 0) {
                        if (s[0] == '"') {
                            std::string unquoted;
    #ifdef _MSC_VER
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
    #else
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
    #endif
                                >> std::quoted(unquoted);
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = infra::withtime_utils::parseLocalTime(unquoted);
                        } else {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = infra::withtime_utils::parseLocalTime(s);
                        }
                    } else {
                        CsvSingleLayerWrapperHelper<ColType>::ref(x) = std::chrono::system_clock::time_point {};
                    }
                } else if constexpr (std::is_empty_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>) {
                    //do nothing
                } else if constexpr (ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::value) {
                    if (s.length() > 0) {
                        if (s[0] == '"') {
                            std::string unquoted;
#ifdef _MSC_VER
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                                >> std::quoted(unquoted);
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::fromString(unquoted);
                        } else {
                            CsvSingleLayerWrapperHelper<ColType>::ref(x) = ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::fromString(s);
                        }
                    } else {
                        CsvSingleLayerWrapperHelper<ColType>::ref(x) = ConvertibleWithString<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType>::fromString("");
                    }
                } else {
                    if (s[0] == '"') {
                        std::string unquoted;
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                            >> std::quoted(unquoted);
                        boost::erase_all(unquoted, ",");
                        std::istringstream iss(unquoted);
                        iss >> CsvSingleLayerWrapperHelper<ColType>::ref(x);
                    } else {
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                            >> CsvSingleLayerWrapperHelper<ColType>::ref(x);
                    }
                }
                return true;
            }
            template <class T, std::size_t Index>
            static std::tuple<bool,std::size_t> parseOneTuple(std::vector<std::string_view> const &parts, T &output, std::size_t currentIdx, bool cumBool, std::size_t cumIdxCount) {
                if constexpr (Index < std::tuple_size_v<T>) {
                    auto res = parseOne(parts, std::get<Index>(output), currentIdx);
                    return parseOneTuple<T,Index+1>(
                        parts, output, currentIdx+std::get<1>(res), (cumBool || std::get<0>(res)), cumIdxCount+std::get<1>(res)
                    );
                } else {
                    return {cumBool, cumIdxCount};
                }
            }
            template <class F>
            static std::tuple<bool,std::size_t> parseOne(std::vector<std::string_view> const &parts, F &output, std::size_t currentIdx) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    if (currentIdx >= parts.size()) {
                        return {false, 1};
                    }
                    return {parseSimpleField_internal<F>(parts[currentIdx], output), 1};
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {                    
                    if (currentIdx >= parts.size()) {
                        return {false, 1};
                    }
                    std::string unquoted;
                    if (parts[currentIdx].length() > 0) {
                        if (parts[currentIdx][0] == '"') {
#ifdef _MSC_VER
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[currentIdx].data(), parts[currentIdx].length())
#else
                            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[currentIdx].begin(), parts[currentIdx].size())
#endif
                            >> std::quoted(unquoted);
                        } else {
                            unquoted = std::string(parts[currentIdx]);
                        }
                    }
                    auto &r = (CsvSingleLayerWrapperHelper<F>::ref(output));
                    std::memset(r.data(), 0, ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength);
                    std::memcpy(r.data(), unquoted.data(), std::min(ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength, unquoted.length()));
                    return {true, 1};
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    bool someGood = false;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            if (currentIdx+ii >= parts.size()) {
                                break;
                            }
                            if (parseSimpleField_internal<BT>(parts[currentIdx+ii], (CsvSingleLayerWrapperHelper<F>::ref(output))[ii])) {
                                someGood = true;
                            }
                        }
                        return {someGood, ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength};
                    } else {
                        std::size_t count = 0;
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            auto res = parseOne<BT>(parts, (CsvSingleLayerWrapperHelper<F>::ref(output))[ii], currentIdx+count);
                            if (std::get<0>(res)) {
                                someGood = true;
                            }
                            count += std::get<1>(res);
                        }
                        return {someGood, count};
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    BT v;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        if (currentIdx >= parts.size()) {
                            return {false, 1};
                        }
                        if (parseSimpleField_internal<BT>(parts[currentIdx], v)) {
                            (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::move(v);
                            return {true, 1};
                        } else {
                            (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::nullopt;
                            return {false, 1};
                        }
                    } else {
                        auto res = parseOne<BT>(parts, v, currentIdx);
                        if (std::get<0>(res)) {
                            (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::move(v);
                        } else {
                            (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::nullopt;
                        }
                        return res;
                    }
                } else if constexpr (IsTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::Value) {
                    return parseOneTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType, 0>(parts, CsvSingleLayerWrapperHelper<F>::ref(output), currentIdx, false, 0);
                } else {
                    auto res = StructFieldInfoBasedSimpleCsvInputImpl::template parse<F>(parts, output, currentIdx);
                    return {std::get<0>(res), std::get<1>(res)-currentIdx};
                }        
            }
            template <class T, std::size_t Index>
            static std::tuple<bool,std::size_t> parseOneTupleWithIdxDict(std::vector<std::string_view> const &parts, T &output, std::size_t currentIdx, std::vector<std::size_t> const &idxDict, bool cumBool, std::size_t cumIdxCount) {
                if constexpr (Index < std::tuple_size_v<T>) {
                    auto res = parseOneWithIdxDict(parts, std::get<Index>(output), currentIdx, idxDict);
                    return parseOneTupleWithIdxDict<T,Index+1>(
                        parts, output, currentIdx+std::get<1>(res), idxDict, (cumBool || std::get<0>(res)), cumIdxCount+std::get<1>(res)
                    );
                } else {
                    return {cumBool, cumIdxCount};
                }
            }
            template <class F>
            static std::tuple<bool,std::size_t> parseOneWithIdxDict(std::vector<std::string_view> const &parts, F &output, std::size_t currentIdx, std::vector<std::size_t> const &idxDict) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    auto realIdx = idxDict[currentIdx];
                    if (realIdx != std::numeric_limits<std::size_t>::max() && realIdx < parts.size()) {
                        return {parseSimpleField_internal<F>(parts[realIdx], output), 1};
                    } else {
                        return {false, 1};
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {
                    auto realIdx = idxDict[currentIdx];
                    if (realIdx != std::numeric_limits<std::size_t>::max() && realIdx < parts.size()) {
                        std::string unquoted;
                        if (parts[realIdx].length() > 0) {
                            if (parts[realIdx][0] == '"') {
#ifdef _MSC_VER
                                boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[realIdx].data(), parts[realIdx].length())
#else
                                boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[realIdx].begin(), parts[realIdx].size())
#endif
                                    >> std::quoted(unquoted);
                            } else {
                                unquoted = std::string(parts[realIdx]);
                            }
                        }
                        auto &r = (CsvSingleLayerWrapperHelper<F>::ref(output));
                        std::memset(r.data(), 0, ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength);
                        std::memcpy(r.data(), unquoted.data(), std::min(ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength, unquoted.length()));
                        return {true, 1};
                    } else {
                        return {false, 1};
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    bool someGood = false;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            auto realIdx = idxDict[currentIdx+ii];
                            if (realIdx != std::numeric_limits<std::size_t>::max() && realIdx < parts.size()) {
                                if (parseSimpleField_internal<BT>(parts[realIdx], (CsvSingleLayerWrapperHelper<F>::ref(output))[ii])) {
                                    someGood = true;
                                }
                            }
                        }
                        return {someGood, ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength};
                    } else {
                        std::size_t count = 0;
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            auto res = parseOneWithIdxDict<BT>(parts, (CsvSingleLayerWrapperHelper<F>::ref(output))[ii], currentIdx+count, idxDict);
                            if (std::get<0>(res)) {
                                someGood = true;
                            }
                            count += std::get<1>(res);
                        }
                        return {someGood, count};
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    BT v;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        auto realIdx = idxDict[currentIdx];
                        if (realIdx != std::numeric_limits<std::size_t>::max() && realIdx < parts.size()) {
                            if (parseSimpleField_internal<BT>(parts[realIdx], v)) {
                                (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::move(v);
                                return {true, 1};
                            } else {
                                (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::nullopt;
                                return {false, 1};
                            }
                        } else {
                            return {false, 1};
                        }
                    } else {
                        auto res = parseOneWithIdxDict<BT>(parts, v, currentIdx, idxDict);
                        if (std::get<0>(res)) {
                            (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::move(v);
                        } else {
                            (CsvSingleLayerWrapperHelper<F>::ref(output)) = std::nullopt;
                        }
                        return res;
                    }
                } else if constexpr (IsTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::Value) {
                    return parseOneTupleWithIdxDict<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType, 0>(parts, CsvSingleLayerWrapperHelper<F>::ref(output), currentIdx, idxDict, false, 0);
                } else {
                    auto res = StructFieldInfoBasedSimpleCsvInputImpl::template parse_with_idx_dict<F>(parts, output, currentIdx, idxDict);
                    return {std::get<0>(res), std::get<1>(res)-currentIdx};
                }        
            }
            template <class T, class F, std::size_t Index>
            static void createOneTupleParser(std::string const &name, F &(*f)(T &), std::unordered_map<std::string, std::function<void(T &, std::string_view const &)>> &ret) {
                using BT = typename CsvSingleLayerWrapperHelper<F>::UnderlyingType;
                if constexpr (Index < std::tuple_size_v<BT>) {
                    auto insideMap = createParserMap<std::tuple_element_t<Index,BT>>(name+"."+std::to_string(Index));
                    for (auto const &item : insideMap) {
                        ret[item.first] = [f,g=item.second](T &t, std::string_view const &v) {
                            g(std::get<Index>(CsvSingleLayerWrapperHelper<F>::ref(f(t))), v);
                        };
                    }
                    createOneTupleParser<T,F,Index+1>(name, f, ret);
                }
            }
            template <class T, class F>
            static void createOneFieldParser_internal(std::string const &name, F &(*f)(T &), std::unordered_map<std::string, std::function<void(T &, std::string_view const &)>> &ret) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    ret[name] = [f](T &t, std::string_view const &v) {
                        parseSimpleField_internal<F>(v, f(t));
                    };
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsCharArray) {                    
                    ret[name] = [f](T &t, std::string_view const &v) {
                        auto &r = CsvSingleLayerWrapperHelper<F>::ref(f(t));
                        std::memset(r.data(), 0, ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength);
                        std::memcpy(r.data(), v.data(), std::min(ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength, v.length()));
                    };
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            ret[name+"["+std::to_string(ii)+"]"] = [f,ii](T &t, std::string_view const &v) {
                                parseSimpleField_internal<F>(v, (CsvSingleLayerWrapperHelper<F>::ref(f(t)))[ii]);
                            };
                        }
                    } else {
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            auto insideMap = createParserMap<BT>(name+"["+std::to_string(ii)+"]");
                            for (auto const &item : insideMap) {
                                ret[item.first] = [f,ii,g=item.second](T &t, std::string_view const &v) {
                                    g((CsvSingleLayerWrapperHelper<F>::ref(f(t)))[ii], v);
                                };
                            }
                        }
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsOptional) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        ret[name] = [f](T &t, std::string_view const &v) {
                            BT x;
                            if (parseSimpleField_internal<BT>(v, x)) {
                                CsvSingleLayerWrapperHelper<F>::ref(f(t)) = std::move(x);
                            } else {
                                CsvSingleLayerWrapperHelper<F>::ref(f(t)) = std::nullopt;
                            }
                        };
                    } else {
                        auto insideMap = createParserMap<BT>(name);
                        for (auto const &item : insideMap) {
                            ret[item.first] = [f,g=item.second](T &t, std::string_view const &v) {
                                auto &r = CsvSingleLayerWrapperHelper<F>::ref(f(t));
                                if (!r) {
                                    r = BT {};
                                    StructFieldInfoBasedInitializer<BT>::initialize(*r);
                                }
                                g(*r, v);
                            };
                        }
                    }
                } else if constexpr (IsTuple<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::Value) {
                    createOneTupleParser<T, F, 0>(name, f, ret);
                } else {
                    using BT = typename CsvSingleLayerWrapperHelper<F>::UnderlyingType;
                    auto insideMap = createParserMap<BT>(name);
                    for (auto const &item : insideMap) {
                        ret[item.first] = [f,g=item.second](T &t, std::string_view const &v) {
                            g(CsvSingleLayerWrapperHelper<F>::ref(f(t)), v);
                        };
                    }
                }        
            }
            template <class T, int FieldCount, int FieldIndex>
            static std::tuple<bool,std::size_t> parse_internal(std::vector<std::string_view> const &parts, T &t, int currentIdx, bool hasGoodSoFar) {
                if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto res = parseOne<F>(parts, StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::access(CsvSingleLayerWrapperHelper<T>::ref(t)), currentIdx);
                    if (std::get<0>(res)) {
                        hasGoodSoFar = true;
                    }
                    return parse_internal<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(parts, (CsvSingleLayerWrapperHelper<T>::ref(t)), currentIdx+std::get<1>(res), hasGoodSoFar);
                } else {
                    return {hasGoodSoFar, currentIdx};
                }
            }
            template <class T, int FieldCount, int FieldIndex>
            static std::tuple<bool,std::size_t> parse_internal_with_idx_dict(std::vector<std::string_view> const &parts, T &t, int currentIdx, bool hasGoodSoFar, std::vector<std::size_t> const &idxDict) {
                if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto res = parseOneWithIdxDict<F>(parts, StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::access(t), currentIdx, idxDict);
                    if (std::get<0>(res)) {
                        hasGoodSoFar = true;
                    }
                    return parse_internal_with_idx_dict<T,FieldCount,FieldIndex+1>(parts, t, currentIdx+std::get<1>(res), hasGoodSoFar, idxDict);
                } else {
                    return {hasGoodSoFar, currentIdx};
                }
            }
            template <class T, int FieldCount, int FieldIndex>
            static void createParserMap_internal(std::string const &prefix, std::unordered_map<std::string, std::function<void(T &, std::string_view const &)>> &ret) {
                if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto s = std::string(StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES[FieldIndex]);
                    if (prefix != "") {
                        s = prefix+"."+s;
                    }
                    createOneFieldParser_internal(s, &StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::access, ret);
                    createParserMap_internal<T,FieldCount,FieldIndex+1>(prefix, ret);
                }
            }
        public:
            //the return value is whether some components have value, and how many items it consumed
            template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static std::tuple<bool,std::size_t> parse(std::vector<std::string_view> const &parts, T &output, std::size_t currentIdx) {
                return parse_internal<T,StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(parts, output, currentIdx, false);
            }
            template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static std::tuple<bool,std::size_t> parse_with_idx_dict(std::vector<std::string_view> const &parts, T &output, std::size_t currentIdx, std::vector<std::size_t> const &idxDict) {
                return parse_internal_with_idx_dict<T,StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(parts, output, currentIdx, false, idxDict);
            }
            template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static std::unordered_map<std::string, std::function<void(T &, std::string_view const &)>> createParserMap(std::string const &prefix) {
                std::unordered_map<std::string, std::function<void(T &, std::string_view const &)>> ret;
                createParserMap_internal<T,StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(prefix, ret); 
                return ret;
            }
        };
    }

    template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && internal::StructFieldInfoCsvSupportChecker<T>::IsComposite>>
    class StructFieldInfoBasedSimpleCsvInput {
    private:
        static std::optional<std::string> readOneLine(std::istream &is) {
            std::string s;
            bool gotData = false;
            while (!is.eof()) {
                std::getline(is, s);
                boost::trim(s);
                if (s == "") {
                    continue;
                }
                if (s[0] == '#') {
                    continue;
                }
                gotData = true;
                break;
            }
            if (gotData) {
                return s;
            } else {
                return std::nullopt;
            }
        }
        static void split(std::string const &input, std::vector<std::string_view> &output, char delim=',') {
            const char *p = input.data();
            const char *end = p+input.length();
            const char *q = p;
            bool inStringField = false;
            while (p < end && *p != '\0') {
                if (!inStringField) {
                    if (q == end || *q == '\0') {
                        output.push_back(std::string_view(p, q-p));
                        break;
                    } else if (*q == delim) {
                        output.push_back(std::string_view(p, q-p));
                        ++q;
                        p = q;
                    } else if (*q == '"') {
                        if (p != q) {
                            throw std::runtime_error("format error in line '"+input+"': string literal not starting at beginning of field");
                        }
                        inStringField = true;
                        ++q;
                    } else {
                        ++q;
                    }
                } else {
                    if (*q == '\\') {
                        q += 2;
                    } else if (*q == '"') {
                        ++q;
                        if (q != end && *q != delim && *q != '\0') {
                            throw std::runtime_error("format error in line '"+input+"': string literal not terminating at end of field");
                        }
                        output.push_back(std::string_view(p,q-p));
                        ++q;
                        p = q;
                        inStringField = false;
                    } else {
                        ++q;
                    }
                }
            }
        }
    public:
        static std::optional<std::vector<std::string_view>> rawReadOneLine(std::istream &is, char delim=',') {
            auto s = readOneLine(is);
            if (!s) {
                return std::nullopt;
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            return {std::move(parts)};
        }
        static std::unordered_map<std::string, std::size_t> readHeader(std::istream &is, char delim=',') {
            auto s = readOneLine(is);
            if (!s) {
                return {};
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            std::unordered_map<std::string, std::size_t> res;
            for (std::size_t ii=0; ii<parts.size(); ++ii) {
                if (parts[ii] != "") {
                    if (parts[ii][0] == '"') {
                        std::string p;
#ifdef _MSC_VER
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[ii].data(), parts[ii].length())
#else
                        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[ii].begin(), parts[ii].size())
#endif
                            >> std::quoted(p);
                        res[p] = ii;
                    } else {
                        res[std::string(parts[ii])] = ii;
                    }
                }
            }
            return res;
        }
        static std::unordered_map<std::string, std::size_t> translateHeaderDict(
            std::unordered_map<std::string, std::size_t> const &originalDict 
            , std::unordered_map<std::string, std::string> const &structFieldNameToFileColNameDict
        ) {
            std::unordered_map<std::string, std::size_t> originalCopy = originalDict;
            std::unordered_map<std::string, std::size_t> ret;
            for (auto const &item : structFieldNameToFileColNameDict) {
                auto iter = originalDict.find(item.second);
                if (iter != originalDict.end()) {
                    ret[item.first] = iter->second;
                    originalCopy.erase(iter->first);
                }
            }
            for (auto const &item : originalCopy) {
                ret[item.first] = item.second;
            }
            return ret;
        }
        static std::vector<std::size_t> headerDictToIdxDict(std::unordered_map<std::string, std::size_t> const &headerDict) {
            std::vector<std::string> fieldNames;
            StructFieldInfoBasedSimpleCsvOutput<T>::collectFieldNames(fieldNames);
            std::vector<std::size_t> output;
            output.resize(fieldNames.size());
            for (std::size_t ii=0; ii<fieldNames.size(); ++ii) {
                auto iter = headerDict.find(fieldNames[ii]);
                if (iter == headerDict.end()) {
                    output[ii] = std::numeric_limits<std::size_t>::max();
                } else {
                    output[ii] = iter->second;
                }
            }
            return output;
        }
        static bool readOne(std::istream &is, T &t, char delim=',') {
            basic::struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);
            auto s = readOneLine(is);
            if (!s) {
                return false;
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            internal::StructFieldInfoBasedSimpleCsvInputImpl::parse<T>(parts, t, 0);
            return true;
        }
        static bool readOneWithIdxDict(std::istream &is, T &t, std::vector<std::size_t> const &idxDict, char delim=',') {
            basic::struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);
            auto s = readOneLine(is);
            if (!s) {
                return false;
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            internal::StructFieldInfoBasedSimpleCsvInputImpl::parse_with_idx_dict<T>(parts, t, 0, idxDict);
            return true;
        }
        template <class OutIter>
        static void readInto(std::istream &is, OutIter iter, StructFieldInfoBasedCsvInputOption option = StructFieldInfoBasedCsvInputOption::IgnoreHeader, std::unordered_map<std::string, std::string> const &structFieldNameToFileColNameDict=std::unordered_map<std::string,std::string>()) {
            std::vector<std::size_t> idxDict;
            switch (option) {
            case StructFieldInfoBasedCsvInputOption::IgnoreHeader:
                readHeader(is);
                break;
            case StructFieldInfoBasedCsvInputOption::HasNoHeader:
                break;
            case StructFieldInfoBasedCsvInputOption::UseHeaderAsDict:
                idxDict = headerDictToIdxDict(translateHeaderDict(readHeader(is), structFieldNameToFileColNameDict));
                break;
            default:
                readHeader(is);
                break;
            }
            std::vector<T> ret;
            T item;
            while (true) {
                if (option == StructFieldInfoBasedCsvInputOption::UseHeaderAsDict) {
                    if (!readOneWithIdxDict(is, item, idxDict)) {
                        return;
                    }
                } else {
                    if (!readOne(is, item)) {
                        return;
                    }
                }
                *iter++ = std::move(item);
            }
        }
        static void readOneNameValuePair(T &t, std::string_view const &name, std::string_view const &value) {
            static auto parserMap = internal::StructFieldInfoBasedSimpleCsvInputImpl::createParserMap<T>("");
            auto iter = parserMap.find(std::string(name));
            if (iter == parserMap.end()) {
                return;
            }
            (iter->second)(t, value);
        }
    };

    template <class M>
    class StructFieldInfoBasedCsvImporterFactory {
    public:
        template <class T, class TimeExtractor, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && internal::StructFieldInfoCsvSupportChecker<T>::IsComposite>>
        static auto createImporter(std::istream &is, TimeExtractor &&timeExtractor, StructFieldInfoBasedCsvInputOption option = StructFieldInfoBasedCsvInputOption::IgnoreHeader, std::unordered_map<std::string, std::string> const &structFieldNameToFileColNameDict=std::unordered_map<std::string,std::string>()) 
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            class LocalI {
            private:
                std::istream *is_;
                TimeExtractor timeExtractor_;
                StructFieldInfoBasedCsvInputOption option_;
                std::unordered_map<std::string,std::string> structFieldNameToFileColNameDict_;
                std::vector<std::size_t> idxDict_;
                using I = StructFieldInfoBasedSimpleCsvInput<T>;
                T t_;
                bool hasData_;
                bool started_;
                void read() {
                    if (option_ == StructFieldInfoBasedCsvInputOption::UseHeaderAsDict) {
                        hasData_ = I::readOneWithIdxDict(*is_, t_, idxDict_);
                    } else {
                        hasData_ = I::readOne(*is_, t_);
                    }
                }
            public:
                LocalI(std::istream *is, TimeExtractor &&timeExtractor, StructFieldInfoBasedCsvInputOption option, std::unordered_map<std::string,std::string> const &structFieldNameToFileColNameDict) 
                    : is_(is), timeExtractor_(std::move(timeExtractor)), option_(option), structFieldNameToFileColNameDict_(structFieldNameToFileColNameDict), idxDict_(), t_(), hasData_(false), started_(false)
                {}
                LocalI(LocalI const &) = delete;
                LocalI &operator=(LocalI const &) = delete;
                LocalI(LocalI &&) = default;
                LocalI &operator=(LocalI &&) = delete;
                std::tuple<bool, typename M::template Data<T>> operator()(typename M::EnvironmentType *env) {
                    if (!started_) {
                        switch (option_) {
                        case StructFieldInfoBasedCsvInputOption::IgnoreHeader:
                            I::readHeader(*is_);
                            break;
                        case StructFieldInfoBasedCsvInputOption::HasNoHeader:
                            break;
                        case StructFieldInfoBasedCsvInputOption::UseHeaderAsDict:
                            idxDict_ = I::headerDictToIdxDict(I::translateHeaderDict(I::readHeader(*is_), structFieldNameToFileColNameDict_));
                            break;
                        default:
                            I::readHeader(*is_);
                            break;
                        }
                        read();
                        started_ = true;
                    }
                    if (!hasData_) {
                        return {false, std::nullopt};
                    }
                    std::tuple<bool, typename M::template Data<T>> ret {
                        true
                        , typename M::template InnerData<T> {
                            env 
                            , {
                                timeExtractor_(env, t_)
                                , std::move(t_)
                                , false
                            }
                        }
                    };
                    read();
                    if (!hasData_) {
                        std::get<0>(ret) = false;
                        std::get<1>(ret)->timedData.finalFlag = true;
                    }
                    return std::move(ret);
                }
            };
            return M::template uniformSimpleImporter<T>(LocalI(&is, std::move(timeExtractor), option, structFieldNameToFileColNameDict));
        }
    };
    template <class M>
    class StructFieldInfoBasedCsvExporterFactory {
    public:
        template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && internal::StructFieldInfoCsvSupportChecker<T>::IsComposite>>
        static auto createExporter(std::ostream &os, bool dontWriteHeader=false) 
            -> std::shared_ptr<typename M::template Exporter<T>>
        {
            class LocalE final : public M::template AbstractExporter<T> {
            private:
                std::ostream *os_;
                bool dontWriteHeader_;
                using O = StructFieldInfoBasedSimpleCsvOutput<T>;
            public:
                LocalE(std::ostream *os, bool dontWriteHeader)
                    : os_(os), dontWriteHeader_(dontWriteHeader)
                {}
                virtual ~LocalE() = default;
                virtual void start(typename M::EnvironmentType *env) override final {
                    if (!dontWriteHeader_) {
                        O::writeHeader(*os_);
                    }
                }
                virtual void handle(typename M::template InnerData<T> &&data) override final {
                    O::writeData(*os_, data.timedData.value);
                }
            };
            return M::template exporter<T>(new LocalE(&os, dontWriteHeader));
        }
    };

    template <class T>
    constexpr bool IsStructFieldInfoBasedCsvCompatibleStruct = (internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && internal::StructFieldInfoCsvSupportChecker<T>::IsComposite);

} } } } }

#endif
