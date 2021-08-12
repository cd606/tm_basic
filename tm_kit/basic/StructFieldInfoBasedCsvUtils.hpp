#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>

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
            || std::is_same_v<F, std::tm>
            || std::is_same_v<F, std::chrono::system_clock::time_point>
            || std::is_same_v<F, std::string>
            || (CsvSingleLayerWrapperHelper<F>::Value && is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>)
        ;
        template <class T>
        class ArrayAndOptionalChecker {
        public:
            static constexpr bool IsArray = false;
            static constexpr bool IsOptional = false;
            static constexpr std::size_t ArrayLength = 0;
            using BaseType = T;
        };
        template <class X, std::size_t N>
        class ArrayAndOptionalChecker<std::array<X,N>> {
        public:
            static constexpr bool IsArray = true;
            static constexpr bool IsOptional = false;
            static constexpr std::size_t ArrayLength = N;
            using BaseType = X;
        };
        template <class X>
        class ArrayAndOptionalChecker<std::optional<X>> {
        public:
            static constexpr bool IsArray = false;
            static constexpr bool IsOptional = true;
            static constexpr std::size_t ArrayLength = 0;
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
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsArray) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::IsGoodForCsv;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsOptional) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::IsGoodForCsv;
                } else {
                    return StructFieldInfoCsvSupportChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsGoodForCsv;
                }
            }
            static constexpr std::size_t fieldCount() {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsArray) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::CsvFieldCount*ArrayAndOptionalChecker<T>::ArrayLength;
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::IsOptional) {
                    return StructFieldInfoCsvSupportChecker<typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::CsvFieldCount;
                } else {
                    return StructFieldInfoCsvSupportChecker<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::CsvFieldCount;
                }
            }
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForCsv = fieldIsGood();
            static constexpr std::size_t CsvFieldCount = fieldCount();
        };
        template <class T>
        class StructFieldInfoCsvSupportChecker<T, true> {
        private:
            template <class F>
            static constexpr bool fieldIsGood() {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
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
        public:
            static constexpr bool IsComposite = true;
            static constexpr bool IsGoodForCsv = checkGood<StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>();
            static constexpr std::size_t CsvFieldCount = totalFieldCount<StructFieldInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0,0>();
        };

        class StructFieldInfoBasedSimpleCsvOutputImpl {
        private:
            template <class F>
            static void collectSingleFieldName(std::string const &prefix, std::string const &thisField, std::vector<std::string> &output) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
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
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::tm>) {
                    os << std::put_time(&CsvSingleLayerWrapperHelper<ColType>::constRef(x), "%Y-%m-%dT%H:%M:%S");
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::chrono::system_clock::time_point>) {
                    os << infra::withtime_utils::localTimeString(CsvSingleLayerWrapperHelper<ColType>::constRef(x));
                } else {
                    os << CsvSingleLayerWrapperHelper<ColType>::constRef(x);
                }
            }
            template <class F>
            static void writeSingleField(std::ostream &os, F const &f) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    writeSimpleField_internal<F>(os, f);
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
                } else {
                    StructFieldInfoBasedSimpleCsvOutputImpl::writeData<F>(
                        os, f
                    );
                }
            }
            template <class T, int FieldCount, int FieldIndex, typename=std::enable_if_t<StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && StructFieldInfoCsvSupportChecker<T>::IsComposite>>
            static void writeData_internal(std::ostream &os, T const &t) {
                if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                    if constexpr (FieldIndex != 0) {
                        os << ',';
                    }
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto CsvSingleLayerWrapperHelper<T>::UnderlyingType::*p = StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::fieldPointer();
                    writeSingleField<F>(os, (CsvSingleLayerWrapperHelper<T>::constRef(t)).*p);
                    writeData_internal<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(os, (CsvSingleLayerWrapperHelper<T>::constRef(t)));
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
        static void writeData(std::ostream &os, T const &t) {
            internal::StructFieldInfoBasedSimpleCsvOutputImpl::writeData<T>(os, t);
            os << '\n';
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
                    return false;
                }
                if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::string>) {
#ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                        >> std::quoted(CsvSingleLayerWrapperHelper<ColType>::ref(x));
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::tm>) {
#ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                        >> std::get_time(&CsvSingleLayerWrapperHelper<ColType>::ref(x), "%Y-%m-%dT%H:%M:%S");
                } else if constexpr (std::is_same_v<typename CsvSingleLayerWrapperHelper<ColType>::UnderlyingType,std::chrono::system_clock::time_point>) {
                    CsvSingleLayerWrapperHelper<ColType>::ref(x) = infra::withtime_utils::parseLocalTime(s);
                } else {
#ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                        >> CsvSingleLayerWrapperHelper<ColType>::ref(x);
                }
                return true;
            }
            template <class F>
            static std::tuple<bool,std::size_t> parseOne(std::vector<std::string_view> const &parts, F &output, std::size_t currentIdx) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    return {parseSimpleField_internal<F>(parts[currentIdx], output), 1};
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    bool someGood = false;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
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
                } else {
                    auto res = StructFieldInfoBasedSimpleCsvInputImpl::template parse<F>(parts, output, currentIdx);
                    return {std::get<0>(res), std::get<1>(res)-currentIdx};
                }        
            }
            template <class F>
            static std::tuple<bool,std::size_t> parseOneWithIdxDict(std::vector<std::string_view> const &parts, F &output, std::size_t currentIdx, std::vector<std::size_t> const &idxDict) {
                if constexpr (is_simple_csv_field_v<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    auto realIdx = idxDict[currentIdx];
                    if (realIdx != std::numeric_limits<std::size_t>::max()) {
                        return {parseSimpleField_internal<F>(parts[realIdx], output), 1};
                    } else {
                        return {false, 1};
                    }
                } else if constexpr (ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    bool someGood = false;
                    if constexpr (is_simple_csv_field_v<BT>) {
                        for (std::size_t ii=0; ii<ArrayAndOptionalChecker<typename CsvSingleLayerWrapperHelper<F>::UnderlyingType>::ArrayLength; ++ii) {
                            auto realIdx = idxDict[currentIdx+ii];
                            if (realIdx != std::numeric_limits<std::size_t>::max()) {
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
                        if (realIdx != std::numeric_limits<std::size_t>::max()) {
                            if (parseSimpleField_internal<BT>(parts[currentIdx], v)) {
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
                } else {
                    auto res = StructFieldInfoBasedSimpleCsvInputImpl::template parse_with_idx_dict<F>(parts, output, currentIdx, idxDict);
                    return {std::get<0>(res), std::get<1>(res)-currentIdx};
                }        
            }
            template <class T, int FieldCount, int FieldIndex>
            static std::tuple<bool,std::size_t> parse_internal(std::vector<std::string_view> const &parts, T &t, int currentIdx, bool hasGoodSoFar) {
                if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto CsvSingleLayerWrapperHelper<T>::UnderlyingType::*p = StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::fieldPointer();
                    auto res = parseOne<F>(parts, ((CsvSingleLayerWrapperHelper<T>::ref(t)).*p), currentIdx);
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
                    auto T::*p = StructFieldTypeInfo<typename CsvSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::fieldPointer();
                    auto res = parseOneWithIdxDict<F>(parts, (t.*p), currentIdx, idxDict);
                    if (std::get<0>(res)) {
                        hasGoodSoFar = true;
                    }
                    return parse_internal_with_idx_dict<T,FieldCount,FieldIndex+1>(parts, t, currentIdx+std::get<1>(res), hasGoodSoFar, idxDict);
                } else {
                    return {hasGoodSoFar, currentIdx};
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
            const char *q = p;
            bool inStringField = false;
            while (*p != '\0') {
                if (!inStringField) {
                    if (*q == '\0') {
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
                        if (*q != delim && *q != '\0') {
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
                    std::string p;
#ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[ii].data(), parts[ii].length())
#else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(parts[ii].begin(), parts[ii].size())
#endif
                        >> std::quoted(p);
                    res[p] = ii;
                }
            }
            return res;
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
        static void readInto(std::istream &is, OutIter iter, StructFieldInfoBasedCsvInputOption option = StructFieldInfoBasedCsvInputOption::IgnoreHeader) {
            std::vector<std::size_t> idxDict;
            switch (option) {
            case StructFieldInfoBasedCsvInputOption::IgnoreHeader:
                readHeader(is);
                break;
            case StructFieldInfoBasedCsvInputOption::HasNoHeader:
                break;
            case StructFieldInfoBasedCsvInputOption::UseHeaderAsDict:
                idxDict = headerDictToIdxDict(readHeader(is));
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
    };

    template <class M>
    class StructFieldInfoBasedCsvImporterFactory {
    public:
        template <class T, class TimeExtractor, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv && internal::StructFieldInfoCsvSupportChecker<T>::IsComposite>>
        static auto createImporter(std::istream &is, TimeExtractor &&timeExtractor, StructFieldInfoBasedCsvInputOption option = StructFieldInfoBasedCsvInputOption::IgnoreHeader) 
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            class LocalI {
            private:
                std::istream *is_;
                TimeExtractor timeExtractor_;
                StructFieldInfoBasedCsvInputOption option_;
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
                LocalI(std::istream *is, TimeExtractor &&timeExtractor, StructFieldInfoBasedCsvInputOption option) 
                    : is_(is), timeExtractor_(std::move(timeExtractor)), option_(option), idxDict_(), t_(), hasData_(false), started_(false)
                {}
                LocalI(LocalI const &) = delete;
                LocalI &operator=(LocalI const &) = delete;
                LocalI(LocalI &&) = default;
                LocalI &operator=(LocalI &&) = default;
                std::tuple<bool, typename M::template Data<T>> operator()(typename M::EnvironmentType *env) {
                    if (!started_) {
                        switch (option_) {
                        case StructFieldInfoBasedCsvInputOption::IgnoreHeader:
                            I::readHeader(*is_);
                            break;
                        case StructFieldInfoBasedCsvInputOption::HasNoHeader:
                            break;
                        case StructFieldInfoBasedCsvInputOption::UseHeaderAsDict:
                            idxDict_ = I::headerDictToIdxDict(I::readHeader(*is_));
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
            return M::template uniformSimpleImporter<T>(LocalI(&is, std::move(timeExtractor), option));
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

} } } } }

#endif