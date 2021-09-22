#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_FLAT_PACK_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_FLAT_PACK_UTILS_HPP_

#include <tm_kit/basic/StructFieldInfoBasedCsvUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {

    namespace internal {
        template <class T>
        using FlatPackSingleLayerWrapperHelper = CsvSingleLayerWrapperHelper<T>;

        template <class F>
        constexpr bool is_simple_flatpack_field_v = 
            std::is_arithmetic_v<F>
            || std::is_empty_v<F>
            || std::is_same_v<F, std::string>
            || std::is_same_v<F, ByteData>
            || (FlatPackSingleLayerWrapperHelper<F>::Value && is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>)
        ;

        template <class T, bool IsStructWithFieldInfo=StructFieldInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::HasGeneratedStructFieldInfo>
        class StructFieldInfoFlatPackSupportChecker {
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForFlatPack = false;
            static constexpr std::size_t FlatPackFieldCount = 0;
        };
        template <class T>
        class StructFieldInfoFlatPackSupportChecker<T, false> {
        private:
            static constexpr bool fieldIsGood() {
                if constexpr (is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>) {
                    return true;
                } else if constexpr (ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::IsArray) {
                    return StructFieldInfoFlatPackSupportChecker<typename ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::IsGoodForFlatPack;
                } else {
                    return StructFieldInfoFlatPackSupportChecker<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::IsGoodForFlatPack;
                }
            }
            static constexpr std::size_t fieldCount() {
                if constexpr (is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::IsArray) {
                    return StructFieldInfoFlatPackSupportChecker<typename ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::BaseType>::FlatPackFieldCount*ArrayAndOptionalChecker<T>::ArrayLength;
                } else {
                    return StructFieldInfoFlatPackSupportChecker<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::FlatPackFieldCount;
                }
            }
        public:
            static constexpr bool IsComposite = false;
            static constexpr bool IsGoodForFlatPack = fieldIsGood();
            static constexpr std::size_t FlatPackFieldCount = fieldCount();
        };
        template <class T>
        class StructFieldInfoFlatPackSupportChecker<T, true> {
        private:
            template <class F>
            static constexpr bool fieldIsGood() {
                if constexpr (is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    return true;
                } else if constexpr (ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    return StructFieldInfoFlatPackSupportChecker<typename ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::IsGoodForFlatPack;
                } else {
                    return StructFieldInfoFlatPackSupportChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::IsGoodForFlatPack;
                }
            }
            template <class F>
            static constexpr std::size_t fieldCount() {
                if constexpr (is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    return 1;
                } else if constexpr (ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    return StructFieldInfoFlatPackSupportChecker<typename ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType>::FlatPackFieldCount*ArrayAndOptionalChecker<F>::ArrayLength;
                } else {
                    return StructFieldInfoFlatPackSupportChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::FlatPackFieldCount;
                }
            }

            template <int FieldCount, int FieldIdx>
            static constexpr bool checkGood() {
                if constexpr (FieldIdx>=0 && FieldIdx<FieldCount) {
                    if constexpr (!fieldIsGood<typename StructFieldTypeInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldIdx>::TheType>()) {
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
                    return totalFieldCount<FieldCount,FieldIdx+1,SoFar+fieldCount<typename StructFieldTypeInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldIdx>::TheType>()>();
                } else {
                    return SoFar;
                }
            }
        public:
            static constexpr bool IsComposite = true;
            static constexpr bool IsGoodForFlatPack = checkGood<StructFieldInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>();
            static constexpr std::size_t FlatPackFieldCount = totalFieldCount<StructFieldInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0,0>();
        };

        class StructFieldInfoBasedSimpleFlatPackOutputImpl {
        private:
            template <class ColType>
            static void writeSimpleField_internal(std::ostream &os, ColType const &x) {
                if constexpr (std::is_empty_v<typename FlatPackSingleLayerWrapperHelper<ColType>::UnderlyingType>) {
                    //do nothing
                } else if constexpr (std::is_same_v<typename FlatPackSingleLayerWrapperHelper<ColType>::UnderlyingType, std::string>) {
                    auto const &r = FlatPackSingleLayerWrapperHelper<ColType>::constRef(x);
                    uint32_t l = (uint32_t) r.length();
                    os.write(reinterpret_cast<char const *>(&l), sizeof(uint32_t));
                    os.write(r.data(), l);
                } else if constexpr (std::is_same_v<typename FlatPackSingleLayerWrapperHelper<ColType>::UnderlyingType, ByteData>) {
                    auto const &r = FlatPackSingleLayerWrapperHelper<ColType>::constRef(x);
                    uint32_t l = (uint32_t) r.content.length();
                    os.write(reinterpret_cast<char const *>(&l), sizeof(uint32_t));
                    os.write(r.content.data(), l);
                } else {
                    os.write(
                        reinterpret_cast<char const *>(&(FlatPackSingleLayerWrapperHelper<ColType>::constRef(x)))
                        , sizeof(typename FlatPackSingleLayerWrapperHelper<ColType>::UnderlyingType)
                    );
                }
            }
            template <class F>
            static void writeSingleField(std::ostream &os, F const &f) {
                if constexpr (is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    writeSimpleField_internal<F>(os, f);
                } else if constexpr (ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    for (auto const &item : FlatPackSingleLayerWrapperHelper<F>::constRef(f)) {
                        if constexpr (is_simple_flatpack_field_v<BT>) {
                            writeSimpleField_internal<BT>(os, item);
                        } else {
                            writeSingleField<BT>(os, item);
                        }
                    }
                } else {
                    StructFieldInfoBasedSimpleFlatPackOutputImpl::writeData<F>(
                        os, f
                    );
                }
            }
            template <class T, int FieldCount, int FieldIndex, typename=std::enable_if_t<StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && StructFieldInfoFlatPackSupportChecker<T>::IsComposite>>
            static void writeData_internal(std::ostream &os, T const &t) {
                if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto FlatPackSingleLayerWrapperHelper<T>::UnderlyingType::*p = StructFieldTypeInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::fieldPointer();
                    writeSingleField<F>(os, (FlatPackSingleLayerWrapperHelper<T>::constRef(t)).*p);
                    writeData_internal<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(os, (FlatPackSingleLayerWrapperHelper<T>::constRef(t)));
                }
            }
        public:
            template <class T, typename=std::enable_if_t<StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && StructFieldInfoFlatPackSupportChecker<T>::IsComposite>>
            static void writeData(std::ostream &os, T const &t) {
                writeData_internal<T,StructFieldInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(os, t);
            }
        };
    }

    template <class T, typename=std::enable_if_t<internal::StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && internal::StructFieldInfoFlatPackSupportChecker<T>::IsComposite>>
    class StructFieldInfoBasedSimpleFlatPackOutput {
    public:
        static void writeData(std::ostream &os, T const &t) {
            internal::StructFieldInfoBasedSimpleFlatPackOutputImpl::writeData<T>(os, t);
        }
        template <class Iter>
        static void writeDataCollection(std::ostream &os, Iter begin, Iter end) {
            for (Iter iter = begin; iter != end; ++iter) {
                writeData(os, *iter);
            }
        }
    };

    namespace internal {
        class StructFieldInfoBasedSimpleFlatPackInputImpl {
        private:
            template <class ColType>
            static std::optional<size_t> parseSimpleField_internal(std::string_view const &s, std::size_t start, ColType &x) {
                using F = typename FlatPackSingleLayerWrapperHelper<ColType>::UnderlyingType;
                if constexpr (std::is_empty_v<F>) {
                    return 0;
                } else if constexpr (std::is_same_v<F, std::string>) {
                    if (s.length() < start+sizeof(uint32_t)) {
                        return std::nullopt;
                    }
                    uint32_t l;
    #ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data()+start, s.length()-start)
    #else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin()+start, s.size()-start)
    #endif
                    .read(reinterpret_cast<char *>(&l), sizeof(uint32_t));
                    if (s.length() < start+sizeof(uint32_t)+l) {
                        return std::nullopt;
                    }
                    FlatPackSingleLayerWrapperHelper<ColType>::ref(x) =
                        std::string(s.data()+start+sizeof(uint32_t), l);
                    return sizeof(uint32_t)+l;
                } else if constexpr (std::is_same_v<F, ByteData>) {
                    if (s.length() < start+sizeof(uint32_t)) {
                        return std::nullopt;
                    }
                    uint32_t l;
    #ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data()+start, s.length()-start)
    #else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin()+start, s.size()-start)
    #endif
                    .read(reinterpret_cast<char *>(&l), sizeof(uint32_t));
                    if (s.length() < start+sizeof(uint32_t)+l) {
                        return std::nullopt;
                    }
                    FlatPackSingleLayerWrapperHelper<ColType>::ref(x).content =
                        std::string(s.data()+start+sizeof(uint32_t), l);
                    return sizeof(uint32_t)+l;
                } else {
                    if (s.length() < start+sizeof(F)) {
                        return std::nullopt;
                    }
    #ifdef _MSC_VER
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data()+start, s.length()-start)
    #else
                    boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin()+start, s.size()-start)
    #endif
                    .read(
                        reinterpret_cast<char *>(&FlatPackSingleLayerWrapperHelper<ColType>::ref(x))
                        , sizeof(F)
                    );
                    return sizeof(F);
                }
            }
            template <class F>
            static std::optional<std::size_t> parseOne(std::string_view const &input, std::size_t start, F &output) {
                if constexpr (is_simple_flatpack_field_v<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>) {
                    return parseSimpleField_internal<F>(input, start, output);
                } else if constexpr (ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::IsArray) {
                    using BT = typename ArrayAndOptionalChecker<typename FlatPackSingleLayerWrapperHelper<F>::UnderlyingType>::BaseType;
                    if constexpr (is_simple_flatpack_field_v<BT>) {
                        std::size_t pos = 0;
                        for (auto &item : FlatPackSingleLayerWrapperHelper<F>::ref(output)) {
                            auto res = parseSimpleField_internal<BT>(input, start+pos, item);
                            if (!res) {
                                return std::nullopt;
                            }
                            pos += *res;
                        }
                        return pos;
                    } else {
                        std::size_t pos = 0;
                        for (auto &item : FlatPackSingleLayerWrapperHelper<F>::ref(output)) {
                            auto res = parseOne<BT>(input, start+pos, item);
                            if (!res) {
                                return std::nullopt;
                            }
                            pos += *res;
                        }
                        return pos;
                    }
                } else {
                    return StructFieldInfoBasedSimpleFlatPackInputImpl::template parse<F>(input, start, output);
                }        
            }
            template <class T, int FieldCount, int FieldIndex>
            static std::optional<std::size_t> parse_internal(std::string_view input, std::size_t start, T &t, std::size_t soFar) {
                if constexpr (FieldCount >=0 && FieldIndex < FieldCount) {
                    using F = typename StructFieldTypeInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::TheType;
                    auto FlatPackSingleLayerWrapperHelper<T>::UnderlyingType::*p = StructFieldTypeInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldIndex>::fieldPointer();
                    auto res = parseOne<F>(input, start, ((FlatPackSingleLayerWrapperHelper<T>::ref(t)).*p));
                    if (!res) {
                        return std::nullopt;
                    }
                    return parse_internal<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType,FieldCount,FieldIndex+1>(input, start+*res, (FlatPackSingleLayerWrapperHelper<T>::ref(t)), soFar+*res);
                } else {
                    return soFar;
                }
            }
        public:
            template <class T, typename=std::enable_if_t<internal::StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && StructFieldInfoFlatPackSupportChecker<T>::IsComposite>>
            static std::optional<std::size_t> parse(std::string_view const &input, std::size_t start, T &output) {
                return parse_internal<T,StructFieldInfo<typename FlatPackSingleLayerWrapperHelper<T>::UnderlyingType>::FIELD_NAMES.size(),0>(input, start, output, 0);
            }
        };
    }

    template <class T, typename=std::enable_if_t<internal::StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && internal::StructFieldInfoFlatPackSupportChecker<T>::IsComposite>>
    class StructFieldInfoBasedSimpleFlatPackInput {
    public:
        static std::optional<std::size_t> readOne(std::string_view const &input, std::size_t start, T &t) {
            basic::struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);
            return internal::StructFieldInfoBasedSimpleFlatPackInputImpl::parse<T>(input, start, t);
            return true;
        }
        template <class OutIter>
        static void readInto(std::string_view const &input, std::size_t start, OutIter iter) {
            std::size_t idx = start;
            while (idx < input.length()) {
                T item;
                auto res = readOne(input, idx, item);
                if (!res) {
                    break;
                }
                *iter++ = item;
                idx += *res;
            }
        }
    };

    template <class T, typename=void>
    class FlatPack {};

    template <class T>
    class FlatPack<T, std::enable_if_t<internal::StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && internal::StructFieldInfoFlatPackSupportChecker<T>::IsComposite, void>> {
    private:
        T t_;
    public:
        FlatPack() = default;
        FlatPack(T const &t) : t_(t) {}
        FlatPack(T &&t) : t_(std::move(t)) {}
        FlatPack(FlatPack const &p) = default;
        FlatPack(FlatPack &&p) = default;
        FlatPack &operator=(FlatPack const &p) = default;
        FlatPack &operator=(FlatPack &&p) = default;
        ~FlatPack() = default;

        FlatPack &operator=(T const &t) {
            t_ = t;
            return *this;
        }
        FlatPack &operator=(T &&t) {
            t_ = std::move(t);
            return *this;
        }

        void SerializeToStream(std::ostream &os) const {
            StructFieldInfoBasedSimpleFlatPackOutput<T>::writeData(os, t_);
        }
        void SerializeToString(std::string *s) const {
            std::ostringstream oss;
            StructFieldInfoBasedSimpleFlatPackOutput<T>::writeData(oss, t_);
            *s = oss.str();
        }
        bool ParseFromStringView(std::string_view const &s) {
            auto res = StructFieldInfoBasedSimpleFlatPackInput<T>::readOne(s, 0, t_);
            if (res) {
                return true;
            } else {
                return false;
            }
        }
        bool ParseFromString(std::string const &s) {
            return ParseFromStringView(std::string_view(s));
        }
        T const &value() const {
            return t_;
        }
        T &value() {
            return t_;
        }
        T &operator*() {
            return t_;
        }
        T const &operator*() const {
            return t_;
        }
        T &&moveValue() && {
            return std::move(t_);
        }
        T *operator->() {
            return &t_;
        }
        T const *operator->() const {
            return &t_;
        }
        static void runSerialize(T const &t, std::ostream &os) {
            StructFieldInfoBasedSimpleFlatPackOutput<T>::writeData(os, t);
        }
        static bool runDeserialize(T &t, std::string_view const &input) {
            auto res = StructFieldInfoBasedSimpleFlatPackInput<T>::readOne(input, 0, t);
            if (res) {
                return true;
            } else {
                return false;
            }
        }
    };
    template <class T>
    class FlatPack<T *, std::enable_if_t<internal::StructFieldInfoFlatPackSupportChecker<T>::IsGoodForFlatPack && internal::StructFieldInfoFlatPackSupportChecker<T>::IsComposite, void>> {
    private:
        T *t_;
    public:
        FlatPack() : t_(nullptr) {}
        FlatPack(T *t) : t_(t) {}
        FlatPack(FlatPack const &p) = default;
        FlatPack(FlatPack &&p) = default;
        FlatPack &operator=(FlatPack const &p) = default;
        FlatPack &operator=(FlatPack &&p) = default;
        ~FlatPack() = default;

        void writeToStream(std::ostream &os) const {
            if (t_) {
                StructFieldInfoBasedSimpleFlatPackOutput<T>::writeData(os, *t_);
            }
        }
        void writeToString(std::string *s) const {
            if (t_) {
                std::ostringstream oss;
                StructFieldInfoBasedSimpleFlatPackOutput<T>::writeData(oss, *t_);
                *s = oss.str();
            }
        }
        bool fromStringView(std::string_view const &s) {
            if (t_) {
                auto res = StructFieldInfoBasedSimpleFlatPackInput<T>::readOne(s, 0, *t_);
                if (res) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        bool fromString(std::string const &s) {
            return fromStringView(std::string_view(s));
        }
        T const &value() const {
            return *t_;
        }
        T &value() {
            return *t_;
        }
        T &operator*() {
            return *t_;
        }
        T const &operator*() const {
            return *t_;
        }
        T *operator->() {
            return t_;
        }
        T const *operator->() const {
            return t_;
        }
    };
} } } } }

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils {
    //allow JSON wrapper to be used for encode/decode
    //without exposing proto-style methods
    template <class T>
    struct RunSerializer<struct_field_info_utils::FlatPack<T>, void> {
        static std::string apply(struct_field_info_utils::FlatPack<T> const &data) {
            std::string s;
            data.writeToString(&s);
            return s;
        }
    };
    template <class T>
    struct RunDeserializer<struct_field_info_utils::FlatPack<T>, void> {
        static std::optional<struct_field_info_utils::FlatPack<T>> apply(std::string_view const &data) {
            TriviallySerializable<T> t;
            if (t.fromStringView(data)) {
                return {std::move(t)};
            } else {
                return std::nullopt;
            }
        }
        static std::optional<struct_field_info_utils::FlatPack<T>> apply(std::string const &data) {
            return apply(std::string_view {data});
        }
        static bool applyInPlace(struct_field_info_utils::FlatPack<T> &output, std::string_view const &data) {
            return output.fromStringView(data);
        }
        static bool applyInPlace(struct_field_info_utils::FlatPack<T> &output, std::string const &data) {
            return output.fromStringView(std::string_view {data});
        }
    };
} } } } }

#endif