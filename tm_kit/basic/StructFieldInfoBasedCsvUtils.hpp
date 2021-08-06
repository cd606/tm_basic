#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_CSV_UTILS_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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
            using BaseType = F;
        };
        template <class X, std::size_t N>
        class StructFieldIsGoodForCsv<std::array<X,N>> {
        public:
            static constexpr bool Value = StructFieldIsGoodForCsv<X>::Value;
            static constexpr bool IsArray = true;
            static constexpr std::size_t ArrayLength = N;
            using BaseType = X;
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
            static constexpr bool checkGood() {
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
        template <class ColType>
        static void writeField_internal(std::ostream &os, ColType const &x) {
            if constexpr (std::is_same_v<ColType,std::string>) {
                os << std::quoted(x);
            } else if constexpr (std::is_same_v<ColType,std::tm>) {
                os << std::put_time(&x, "%Y-%m-%dT%H:%M:%S");
            } else if constexpr (std::is_same_v<ColType,std::chrono::system_clock::time_point>) {
                os << infra::withtime_utils::localTimeString(x);
            } else {
                os << x;
            }
        }
        template <int FieldCount, int FieldIndex>
        static void writeData_internal(std::ostream &os, T const &t) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                if constexpr (FieldIndex != 0) {
                    os << ',';
                }
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (internal::StructFieldIsGoodForCsv<ColType>::IsArray) {
                    for (std::size_t ii=0; ii<internal::StructFieldIsGoodForCsv<ColType>::ArrayLength; ++ii) {
                        if (ii > 0) {
                            os << ',';
                        }
                        writeField_internal<typename internal::StructFieldIsGoodForCsv<ColType>::BaseType>(
                            os, (t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()))[ii]
                        );
                    }
                } else {
                    writeField_internal<ColType>(
                        os, t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer())
                    );
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
    
    enum class StructFieldInfoBasedCsvInputOption {
        IgnoreHeader 
        , HasNoHeader
        , UseHeaderAsDict
    };

    template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv>>
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
        template <class ColType>
        static void parseField_internal(std::string_view const &s, ColType &x) {
            if (s == "") {
                return;
            }
            if constexpr (std::is_same_v<ColType,std::string>) {
                std::istringstream(std::string(s)) >> std::quoted(x);
            } else if constexpr (std::is_same_v<ColType,std::tm>) {
                std::istringstream(std::string(s)) >> std::get_time(&x, "%Y-%m-%dT%H:%M:%S");
            } else if constexpr (std::is_same_v<ColType,std::chrono::system_clock::time_point>) {
                x = infra::withtime_utils::parseLocalTime(std::string(s));
            } else {
                x = boost::lexical_cast<ColType>(std::string(s));
            }
        }
        template <int FieldCount, int FieldIndex>
        static void parse_internal(std::vector<std::string_view> const &parts, T &t, int currentIdx) {
            if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (internal::StructFieldIsGoodForCsv<ColType>::IsArray) {
                    for (std::size_t ii=0; ii<internal::StructFieldIsGoodForCsv<ColType>::ArrayLength; ++ii,++currentIdx) {
                        if (currentIdx < parts.size()) {
                            parseField_internal<typename internal::StructFieldIsGoodForCsv<ColType>::BaseType>(
                                parts[currentIdx]
                                , (t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()))[ii]
                            );
                        }
                    }
                } else {
                    if (currentIdx < parts.size()) {
                        parseField_internal<ColType>(parts[currentIdx], t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()));
                    }
                    ++currentIdx;
                }
                parse_internal<FieldCount,FieldIndex+1>(parts,t,currentIdx);
            }
        }
        template <int FieldCount, int FieldIndex>
        static void parse_internal_with_header_dict(std::vector<std::string_view> const &parts, T &t, std::unordered_map<std::string, std::size_t> const &headerDict) {
            if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (internal::StructFieldIsGoodForCsv<ColType>::IsArray) {
                    for (std::size_t ii=0; ii<internal::StructFieldIsGoodForCsv<ColType>::ArrayLength; ++ii) {
                        std::ostringstream oss;
                        oss << StructFieldInfo<T>::FIELD_NAMES[FieldIndex] << '[' << ii << ']';
                        auto iter = headerDict.find(oss.str());
                        if (iter != headerDict.end() && iter->second >= 0 && iter->second < parts.size()) {
                            parseField_internal<typename internal::StructFieldIsGoodForCsv<ColType>::BaseType>(
                                parts[iter->second]
                                , (t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()))[ii]
                            );
                        }
                    }
                } else {
                    auto iter = headerDict.find(std::string(StructFieldInfo<T>::FIELD_NAMES[FieldIndex]));
                    if (iter != headerDict.end() && iter->second >= 0 && iter->second < parts.size()) {
                        parseField_internal<ColType>(parts[iter->second], t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()));
                    }
                }
                parse_internal_with_header_dict<FieldCount,FieldIndex+1>(parts,t,headerDict);
            }
        }
        template <int FieldCount, int FieldIndex>
        static void headerDictToIdxDict_internal(std::unordered_map<std::string, std::size_t> const &headerDict, std::array<std::vector<int>,FieldCount> &output) {
            if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (internal::StructFieldIsGoodForCsv<ColType>::IsArray) {
                    output[FieldIndex].resize(internal::StructFieldIsGoodForCsv<ColType>::ArrayLength);
                    for (std::size_t ii=0; ii<internal::StructFieldIsGoodForCsv<ColType>::ArrayLength; ++ii) {
                        output[FieldIndex][ii] = -1;
                        std::ostringstream oss;
                        oss << StructFieldInfo<T>::FIELD_NAMES[FieldIndex] << '[' << ii << ']';
                        auto iter = headerDict.find(oss.str());
                        if (iter != headerDict.end() && iter->second >= 0) {
                            output[FieldIndex][ii] = iter->second;
                        }
                    }
                } else {
                    output[FieldIndex].resize(1);
                    output[FieldIndex][0] = -1;
                    auto iter = headerDict.find(std::string(StructFieldInfo<T>::FIELD_NAMES[FieldIndex]));
                    if (iter != headerDict.end() && iter->second >= 0) {
                        output[FieldIndex][0] = iter->second;
                    }
                }
                headerDictToIdxDict_internal<FieldCount,FieldIndex+1>(headerDict,output);
            }
        }
        template <int FieldCount, int FieldIndex>
        static void parse_internal_with_idx_dict(std::vector<std::string_view> const &parts, T &t, std::array<std::vector<int>,FieldCount> const &idxDict) {
            if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                using ColType = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (internal::StructFieldIsGoodForCsv<ColType>::IsArray) {
                    for (std::size_t ii=0; ii<internal::StructFieldIsGoodForCsv<ColType>::ArrayLength; ++ii) {
                        auto const &item = idxDict[FieldIndex];
                        if (ii < item.size() && item[ii] >= 0 && item[ii] < parts.size()) {
                            parseField_internal<typename internal::StructFieldIsGoodForCsv<ColType>::BaseType>(
                                parts[item[ii]]
                                , (t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()))[ii]
                            );
                        }
                    }
                } else {
                    auto const &item = idxDict[FieldIndex];
                    if (0 < item.size() && item[0] >= 0 && item[0] < parts.size()) {
                        parseField_internal<ColType>(parts[item[0]], t.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()));
                    }
                }
                parse_internal_with_idx_dict<FieldCount,FieldIndex+1>(parts,t,idxDict);
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
                    std::istringstream(std::string(parts[ii])) >> std::quoted(p);
                    res[p] = ii;
                }
            }
            return res;
        }
        static bool readOne(std::istream &is, T &t, char delim=',') {
            basic::struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);
            auto s = readOneLine(is);
            if (!s) {
                return false;
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            parse_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0>(parts, t, 0);
            return true;
        }
        static bool readOneWithHeaderDict(std::istream &is, T &t, std::unordered_map<std::string, std::size_t> const &headerDict, char delim=',') {
            basic::struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);
            auto s = readOneLine(is);
            if (!s) {
                return false;
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            parse_internal_with_header_dict<StructFieldInfo<T>::FIELD_NAMES.size(),0>(parts, t, headerDict);
            return true;
        }
        static std::array<std::vector<int>,StructFieldInfo<T>::FIELD_NAMES.size()> headerDictToIdxDict(std::unordered_map<std::string, std::size_t> const &headerDict) {
            std::array<std::vector<int>,StructFieldInfo<T>::FIELD_NAMES.size()> ret;
            headerDictToIdxDict_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0>(headerDict, ret);
            return ret;
        }
        static bool readOneWithIdxDict(std::istream &is, T &t, std::array<std::vector<int>,StructFieldInfo<T>::FIELD_NAMES.size()> const &idxDict, char delim=',') {
            basic::struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);
            auto s = readOneLine(is);
            if (!s) {
                return false;
            }
            std::vector<std::string_view> parts;
            split(*s, parts, delim);
            parse_internal_with_idx_dict<StructFieldInfo<T>::FIELD_NAMES.size(),0>(parts, t, idxDict);
            return true;
        }
        template <class OutIter>
        static void readInto(std::istream &is, OutIter iter, StructFieldInfoBasedCsvInputOption option = StructFieldInfoBasedCsvInputOption::IgnoreHeader) {
            std::array<std::vector<int>,StructFieldInfo<T>::FIELD_NAMES.size()> idxDict;
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
        template <class T, class TimeExtractor, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv>>
        static auto createImporter(std::istream &is, TimeExtractor &&timeExtractor, StructFieldInfoBasedCsvInputOption option = StructFieldInfoBasedCsvInputOption::IgnoreHeader) 
            -> std::shared_ptr<typename M::template Importer<T>>
        {
            class LocalI {
            private:
                std::istream *is_;
                TimeExtractor timeExtractor_;
                StructFieldInfoBasedCsvInputOption option_;
                std::array<std::vector<int>,StructFieldInfo<T>::FIELD_NAMES.size()> idxDict_;
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
        template <class T, typename=std::enable_if_t<internal::StructFieldInfoCsvSupportChecker<T>::IsGoodForCsv>>
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