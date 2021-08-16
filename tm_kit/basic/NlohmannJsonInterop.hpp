#ifndef TM_KIT_BASIC_NLOHMANN_JSON_INTEROP_HPP_
#define TM_KIT_BASIC_NLOHMANN_JSON_INTEROP_HPP_

/*
 * Explanation:
 *
 * While nlohmann-json supports very flexible integration of arbitrary
 * types with JSON, there is the requirement that the from_json and 
 * to_json functions be put in the original namespace as the types. This
 * is not particularly difficult since nlohmann-json provides a macro
 * to effect that, and I could have included that macro call in the macros
 * of SerializationHelperMacros.cpp.
 * 
 * However, for such a scheme to work, it would be best to put the macro call
 * in ALL the generated types, but if the types use for example std::optional
 * or std::variant, then we will have a problem.
 * 
 * Therefore, I have decided for now to provide a template-based solution so that
 * only the types that are naturally mappable to JSON will use this solution. 
 */

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>

#include <nlohmann/json.hpp>
#include <iomanip>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace nlohmann_json_interop {

    template <class T, typename Enable=void>
    class JsonEncoder {};

    template <class IntType>
    class JsonEncoder<IntType, std::enable_if_t<
        (std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType>)
        , void
    >> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, IntType data) {
            if (key) {
                output[*key] = data;
            } else {
                output = data;
            }
        }
    };
    template <>
    class JsonEncoder<bool, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, bool data) {
            if (key) {
                output[*key] = data;
            } else {
                output = data;
            }
        }
    };
    template <class T>
    class JsonEncoder<T, std::enable_if_t<std::is_enum_v<T>, void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, T data) {
            if (key) {
                output[*key] = static_cast<std::underlying_type_t<T>>(data);
            } else {
                output = static_cast<std::underlying_type_t<T>>(data);
            }
        }
    };
    template <>
    class JsonEncoder<std::string, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::string const &data) {
            if (key) {
                output[*key] = data;
            } else {
                output = data;
            }
        }
    };
    template <>
    class JsonEncoder<std::string_view, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::string_view const &data) {
            if (key) {
                output[*key] = std::string(data);
            } else {
                output = std::string(data);
            }
        }
    };
    template <std::size_t N>
    class JsonEncoder<std::array<char,N>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::array<char,N> const &data) {
            std::size_t ii = 0;
            for (; ii<N; ++ii) {
                if (data[ii] == '\0') {
                    break;
                }
            }
            if (key) {
                output[*key] = std::string(data,ii);
            } else {
                output = std::string(data,ii);
            }
        }
    };
    template <>
    class JsonEncoder<ByteData, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, ByteData const &data) {
            auto &o = (key?output[*key]:output);
            for (auto c : data.content) {
                o.push_back((unsigned int) c);
            }
        }
    };
    template <>
    class JsonEncoder<ByteDataView, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, ByteDataView const &data) {
            auto &o = (key?output[*key]:output);
            for (auto c : data.content) {
                o.push_back((unsigned int) c);
            }
        }
    };
    template <std::size_t N>
    class JsonEncoder<std::array<unsigned char,N>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::array<unsigned char,N> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto c : data) {
                o.push_back((unsigned int) c);
            }
        }
    };
    template <>
    class JsonEncoder<std::tm, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::tm const &data) {
            std::ostringstream oss;
            oss << std::put_time(&data, "%Y-%m-%dT%H:%M:%S");
            if (key) {
                output[*key] = oss.str();
            } else {
                output = oss.str();
            }
        }
    };
    template <>
    class JsonEncoder<std::chrono::system_clock::time_point, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::chrono::system_clock::time_point const &data) {
            if (key) {
                output[*key] = infra::withtime_utils::localTimeString(data);
            } else {
                output = infra::withtime_utils::localTimeString(data);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::vector<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::vector<T> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                o.push_back(nlohmann::json());
                JsonEncoder<T>::write(o.back(), std::nullopt, item);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::list<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::list<T> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                o.push_back(nlohmann::json());
                JsonEncoder<T>::write(o.back(), std::nullopt, item);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::valarray<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::valarray<T> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                o.push_back(item);
            }
        }
    };
    template <class T, std::size_t N>
    class JsonEncoder<std::array<T,N>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::array<T,N> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                o.push_back(nlohmann::json());
                JsonEncoder<T>::write(o.back(), std::nullopt, item);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::map<std::string,T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::map<std::string,T> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                JsonEncoder<T>::write(o, item.first, item.second);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::unordered_map<std::string,T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::unordered_map<std::string,T> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                JsonEncoder<T>::write(o, item.first, item.second);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::vector<std::pair<std::string,T>>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::vector<std::pair<std::string,T>> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                JsonEncoder<T>::write(o, item.first, item.second);
            }
        }
    };
    template <class T>
    class JsonEncoder<std::optional<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::optional<T> const &data) {
            if (data) {
                JsonEncoder<T>::write(output, key, *data);
            }
        }
    };
    template <>
    class JsonEncoder<VoidStruct, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, VoidStruct const &data) {
        }
    };
    template <int32_t N>
    class JsonEncoder<ConstType<N>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, ConstType<N> const &data) {
        }
    };
    template <class T>
    class JsonEncoder<SingleLayerWrapper<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapper<T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };
    template <int32_t N, class T>
    class JsonEncoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };
    template <class M, class T>
    class JsonEncoder<SingleLayerWrapperWithTypeMark<M,T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };

    template <class T>
    class JsonEncoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void write_impl(nlohmann::json &output, T const &data) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                JsonEncoder<F>::write(
                    output
                    , std::string(StructFieldInfo<T>::FIELD_NAMES[FieldIndex])
                    , data.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer())
                );
                write_impl<FieldCount,FieldIndex+1>(output, data);
            }
        }
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, T const &data) {
            write_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>((key?output[*key]:output), data);
        }
    };

    template <class T, typename Enable=void>
    class JsonDecoder {};

    template <class IntType>
    class JsonDecoder<IntType, std::enable_if_t<
        (std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType>)
        , void
    >> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, IntType &data) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                data = boost::lexical_cast<IntType>(s);
            } else {
                i.get_to(data);
            }
        }
    };
    template <>
    class JsonDecoder<bool, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, bool &data) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                data = boost::lexical_cast<bool>(s);
            } else {
                i.get_to(data);
            }
        }
    };
    template <class T>
    class JsonDecoder<T, std::enable_if_t<std::is_enum_v<T>, void>> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, T &data) {
            std::underlying_type_t<T> t;
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                t = boost::lexical_cast<std::underlying_type_t<T>>(s);
            } else {
                i.get_to(t);
            }
            data = static_cast<T>(t);
        }
    };
    template <>
    class JsonDecoder<std::string, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::string &data) {
            if (key) {
                input.at(*key).get_to(data);
            } else {
                input.get_to(data);
            }
        }
    };
    template <std::size_t N>
    class JsonDecoder<std::array<char,N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<char,N> &data) {
            std::memset(data.data(), 0, N);
            std::string s;
            if (key) {
                input.at(*key).get_to(s);
            } else {
                input.get_to(s);
            }
            std::memcpy(data.data(), s.data(), std::min(s.length(), N));
        }
    };
    template <>
    class JsonDecoder<ByteData, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, ByteData &data) {
            std::vector<unsigned int> x;
            if (key) {
                input.at(*key).get_to(x);
            } else {
                input.get_to(x);
            }
            data.content.resize(x.size());
            for (std::size_t ii=0; ii<x.size(); ++ii) {
                data.content[ii] = (char) x[ii];
            }
        }
    };
    template <std::size_t N>
    class JsonDecoder<std::array<unsigned char,N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<unsigned char,N> &data) {
            std::memset(data, 0, N);
            std::vector<unsigned int> x;
            if (key) {
                input.at(*key).get_to(x);
            } else {
                input.get_to(x);
            }
            for (std::size_t ii=0; ii<x.size() && ii<N; ++ii) {
                data[ii] = (char) x[ii];
            }
        }
    };
    template <>
    class JsonDecoder<std::tm, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::tm &data) {
            std::string s;
            if (key) {
                input.at(*key).get_to(s);
            } else {
                input.get_to(s);
            }
            std::stringstream ss(s);
            ss >> std::get_time(&data, "%Y-%m-%dT%H:%M:%S");
        }
    };
    template <>
    class JsonDecoder<std::chrono::system_clock::time_point, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::chrono::system_clock::time_point &data) {
            std::string s;
            if (key) {
                input.at(*key).get_to(s);
            } else {
                input.get_to(s);
            }
            data = infra::withtime_utils::parseLocalTime(std::string_view(s));
        }
    };
    template <class T>
    class JsonDecoder<std::vector<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::vector<T> &data) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i) {
                JsonDecoder<T>::read(item, std::nullopt, data[ii]);
                ++ii;
            }
        }
    };
    template <class T>
    class JsonDecoder<std::list<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::list<T> &data) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i) {
                data.push_back(T {});
                JsonDecoder<T>::read(item, std::nullopt, data.back());
            }
        }
    };
    template <class T>
    class JsonDecoder<std::valarray<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::valarray<T> &data) {
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i) {
                JsonDecoder<T>::read(item, std::nullopt, data[ii]);
                ++ii;
            }
        }
    };
    template <class T, std::size_t N>
    class JsonDecoder<std::array<T,N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<T,N> &data) {
            auto const &i = (key?input.at(*key):input);
            std::size_t ii = 0;
            for (auto const &item : i) {
                if (ii < N) {
                    JsonDecoder<T>::read(item, std::nullopt, data[ii]);
                } else {
                    break;
                }
                ++ii;
            }
            for (; ii<N; ++ii) {
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data[ii]);
            }
        }
    };
    template <class T>
    class JsonDecoder<std::map<std::string,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::map<std::string,T> &data) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i.items()) {
                auto iter = data.insert({item.key(), T{}});
                JsonDecoder<T>::read(item.value(), std::nullopt, iter->second);
            }
        }
    };
    template <class T>
    class JsonDecoder<std::unordered_map<std::string,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::unordered_map<std::string,T> &data) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i.items()) {
                auto iter = data.insert({item.key(), T{}});
                JsonDecoder<T>::read(item.value(), std::nullopt, iter->second);
            }
        }
    };
    template <class T>
    class JsonDecoder<std::vector<std::pair<std::string,T>>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::vector<std::pair<std::string,T>> &data) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i.items()) {
                data[ii].first = item.key();
                JsonDecoder<T>::read(item.value(), std::nullopt, data[ii].second);
                ++ii;
            }
        }
    };
    template <class T>
    class JsonDecoder<std::optional<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::optional<T> &data) {
            if (key) {
                if (input.contains(*key)) {
                    data = T {};
                    JsonDecoder<T>::read(input.at(*key), std::nullopt, *data);
                } else {
                    data = std::nullopt;
                }
            } else {
                if (!input.is_null()) {
                    data = T {};
                    JsonDecoder<T>::read(input, std::nullopt, *data);
                } else {
                    data = std::nullopt;
                }
            }
        }
    };
    template <>
    class JsonDecoder<VoidStruct, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, VoidStruct &data) {
        }
    };
    template <int32_t N>
    class JsonDecoder<ConstType<N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, ConstType<N> &data) {
        }
    };
    template <class T>
    class JsonDecoder<SingleLayerWrapper<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapper<T> &data) {
            JsonDecoder<T>::read(input, key, data.value);
        }
    };
    template <int32_t N, class T>
    class JsonDecoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> &data) {
            JsonDecoder<T>::read(input, key, data.value);
        }
    };
    template <class M, class T>
    class JsonDecoder<SingleLayerWrapperWithTypeMark<M,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> &data) {
            JsonDecoder<T>::read(input, key, data.value);
        }
    };

    template <class T>
    class JsonDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void read_impl(nlohmann::json const &input, T &data) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                std::string s {StructFieldInfo<T>::FIELD_NAMES[FieldIndex]};
                if (input.contains(s)) {
                    JsonDecoder<F>::read(input, s, data.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()));
                }
                read_impl<FieldCount,FieldIndex+1>(input, data);
            }
        }
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, T &data) {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data);
            read_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>((key?input.at(*key):input), data);
        }
    };
    
    template <class T, typename=void>
    class Json {};

    template <class T>
    class Json<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        T t_;
    public:
        Json() = default;
        Json(T const &t) : t_(t) {}
        Json(T &&t) : t_(std::move(t)) {}
        Json(Json const &p) = default;
        Json(Json &&p) = default;
        Json &operator=(Json const &p) = default;
        Json &operator=(Json &&p) = default;
        ~Json() = default;

        Json &operator=(T const &t) {
            t_ = t;
            return *this;
        }
        Json &operator=(T &&t) {
            t_ = std::move(t);
            return *this;
        }

        void SerializeToStream(std::ostream &os) const {
            nlohmann::json output;
            JsonEncoder<T>::write(output, std::nullopt, t_);
            os << output;
        }
        void SerializeToString(std::string *s) const {
            nlohmann::json output;
            JsonEncoder<T>::write(output, std::nullopt, t_);
            return output.dump();
        }
        bool ParseFromStringView(std::string_view const &s) {
            nlohmann::json x;
#ifdef _MSC_VER
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
            >> x;
            JsonDecoder<T>::read(x, std::nullopt, t_);
            return true;
        }
        bool ParseFromString(std::string const &s) {
            return ParseFromStringView(std::string_view(s));
        }
        bool ParseFromStream(std::istream &s) {
            nlohmann::json x;
            s >> x;
            JsonDecoder<T>::read(x, std::nullopt, t_);
            return true;
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
        T *operator->() {
            return &t_;
        }
        T const *operator->() const {
            return &t_;
        }
        static void runSerialize(T const &t, std::ostream &os) {
            nlohmann::json output;
            JsonEncoder<T>::write(output, std::nullopt, t);
            os << output;
        }
        static bool runDeserialize(T &t, std::string_view const &input) {
            nlohmann::json x;
#ifdef _MSC_VER
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(input.data(), input.length())
#else
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(input.begin(), input.size())
#endif
            >> x;
            JsonDecoder<T>::read(x, std::nullopt, t);
            return true;
        }
    };
    template <class T>
    class Json<T *, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        T *t_;
    public:
        Json() : t_(nullptr) {}
        Json(T *t) : t_(t) {}
        Json(Json const &p) = default;
        Json(Json &&p) = default;
        Json &operator=(Json const &p) = default;
        Json &operator=(Json &&p) = default;
        ~Json() = default;

        void SerializeToStream(std::ostream &os) const {
            if (t_) {
                nlohmann::json output;
                JsonEncoder<T>::write(output, std::nullopt, *t_);
                os << output;
            }
        }
        void SerializeToString(std::string *s) const {
            if (t_) {
                nlohmann::json output;
                JsonEncoder<T>::write(output, std::nullopt, *t_);
                return output.dump();
            }
        }
        bool ParseFromStringView(std::string_view const &s) {
            if (t_) {
                nlohmann::json x;
#ifdef _MSC_VER
                boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                >> x;
                JsonDecoder<T>::read(x, std::nullopt, *t_);
                return true;
            } else {
                return false;
            }
        }
        bool ParseFromString(std::string const &s) {
            return ParseFromStringView(std::string_view(s));
        }
        bool ParseFromStream(std::istream &s) {
            if (t_) {
                nlohmann::json x;
                s >> x;
                JsonDecoder<T>::read(x, std::nullopt, *t_);
                return true;
            } else {
                return false;
            }
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

#endif
