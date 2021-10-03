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

    template <class T, typename Enable=void>
    struct JsonWrappable {
        static constexpr bool value = false;
    };

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
    template <class IntType>
    struct JsonWrappable<IntType, std::enable_if_t<
        (std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType>)
        , void
    >> {
        static constexpr bool value = true;
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
    template <>
    struct JsonWrappable<bool, void> {
        static constexpr bool value = true;
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
    template <class T>
    struct JsonWrappable<T, std::enable_if_t<std::is_enum_v<T>, void>> {
        static constexpr bool value = true;
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
    struct JsonWrappable<std::string, void> {
        static constexpr bool value = true;
    };
    //For same reason as in proto_interop, string_view and ByteDataView are not
    //marked as Json wrappable
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
    template <std::size_t N>
    struct JsonWrappable<std::array<char,N>, void> {
        static constexpr bool value = true;
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
    struct JsonWrappable<ByteData, void> {
        static constexpr bool value = true;
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
    template <std::size_t N>
    struct JsonWrappable<std::array<unsigned char,N>, void> {
        static constexpr bool value = true;
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
    struct JsonWrappable<std::tm, void> {
        static constexpr bool value = true;
    };
    template <>
    class JsonEncoder<DateHolder, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, DateHolder const &data) {
            std::ostringstream oss;
            oss << data;
            if (key) {
                output[*key] = oss.str();
            } else {
                output = oss.str();
            }
        }
    };
    template <>
    struct JsonWrappable<DateHolder, void> {
        static constexpr bool value = true;
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
    template <>
    struct JsonWrappable<std::chrono::system_clock::time_point, void> {
        static constexpr bool value = true;
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
    struct JsonWrappable<std::vector<T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
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
    struct JsonWrappable<std::list<T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
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
    template <class T>
    struct JsonWrappable<std::valarray<T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
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
    template <class T, std::size_t N>
    struct JsonWrappable<std::array<T,N>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
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
    struct JsonWrappable<std::map<std::string,T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
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
    struct JsonWrappable<std::unordered_map<std::string,T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
    };
    template <class K, class D, class Cmp>
    class JsonEncoder<std::map<K,D,Cmp>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::map<K,D,Cmp> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                nlohmann::json x;
                x.push_back(nlohmann::json());
                JsonEncoder<K>::write(x.back(), std::nullopt, item.first);
                x.push_back(nlohmann::json());
                JsonEncoder<D>::write(x.back(), std::nullopt, item.second);
                o.push_back(std::move(x));
            }
        }
    };
    template <class K, class D, class Cmp>
    struct JsonWrappable<std::map<K,D,Cmp>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
        static constexpr bool value = JsonWrappable<K>::value && JsonWrappable<D>::value;
    };
    template <class K, class D, class Hash>
    class JsonEncoder<std::unordered_map<K,D,Hash>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::unordered_map<K,D,Hash> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                nlohmann::json x;
                x.push_back(nlohmann::json());
                JsonEncoder<K>::write(x.back(), std::nullopt, item.first);
                x.push_back(nlohmann::json());
                JsonEncoder<D>::write(x.back(), std::nullopt, item.second);
                o.push_back(std::move(x));
            }
        }
    };
    template <class K, class D, class Hash>
    struct JsonWrappable<std::unordered_map<K,D,Hash>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
        static constexpr bool value = JsonWrappable<K>::value && JsonWrappable<D>::value;
    };
    //This is a special case, to represent a vector as JSON map
    //If really a vector like this is needed, std::vector<std::tuple>
    //can be used instead.
    template <class T>
    class JsonEncoder<std::vector<std::pair<std::string,T>>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::vector<std::pair<std::string,T>> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                JsonEncoder<T>::write(o, std::get<0>(item), std::get<1>(item));
            }
        }
    };
    template <class T>
    struct JsonWrappable<std::vector<std::pair<std::string,T>>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
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
    template <class T>
    struct JsonWrappable<std::optional<T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
    };
    template <>
    class JsonEncoder<VoidStruct, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, VoidStruct const &data) {
        }
    };
    template <>
    struct JsonWrappable<VoidStruct, void> {
        static constexpr bool value = true;
    };
    template <>
    class JsonEncoder<std::monostate, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::monostate const &data) {
        }
    };
    template <>
    struct JsonWrappable<std::monostate, void> {
        static constexpr bool value = true;
    };
    template <int32_t N>
    class JsonEncoder<ConstType<N>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, ConstType<N> const &data) {
        }
    };
    template <int32_t N>
    struct JsonWrappable<ConstType<N>, void> {
        static constexpr bool value = true;
    };
    template <class T>
    class JsonEncoder<SingleLayerWrapper<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapper<T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };
    template <class T>
    struct JsonWrappable<SingleLayerWrapper<T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
    };
    template <int32_t N, class T>
    class JsonEncoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };
    template <int32_t N, class T>
    struct JsonWrappable<SingleLayerWrapperWithID<N,T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
    };
    template <class M, class T>
    class JsonEncoder<SingleLayerWrapperWithTypeMark<M,T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };
    template <class M, class T>
    struct JsonWrappable<SingleLayerWrapperWithTypeMark<M,T>, void> {
        static constexpr bool value = JsonWrappable<T>::value;
    };

    template <class... Ts>
    struct JsonWrappable<std::variant<Ts...>, void> {
    private:
        template <std::size_t Index>
        static constexpr bool value_internal() {
            if constexpr (Index < sizeof...(Ts)) {
                if constexpr (!JsonWrappable<std::variant_alternative_t<Index,std::variant<Ts...>>>::value) {
                    return false;
                } else {
                    return value_internal<Index+1>();
                }
            } else {
                return true;
            }
        }
    public:
        static constexpr bool value = value_internal<0>();
    };
    template <class... Ts>
    class JsonEncoder<std::variant<Ts...>, std::enable_if_t<JsonWrappable<std::variant<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static void write_impl(nlohmann::json &output, std::variant<Ts...> const &data) {
            if constexpr (Index >= 0 && Index < sizeof...(Ts)) {
                if (Index == data.index()) {
                    using F = std::variant_alternative_t<Index,std::variant<Ts...>>;
                    output["index"] = Index;
                    JsonEncoder<F>::write(
                        output
                        , "content"
                        , std::get<Index>(data)
                    );
                } else {
                    write_impl<Index+1>(output, data);
                }
            }
        }
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::variant<Ts...> const &data) {
            if constexpr (sizeof...(Ts) >= 0) {
                write_impl<0>((key?output[*key]:output), data);
            }
        }
    };
    template <class... Ts>
    struct JsonWrappable<std::tuple<Ts...>, void> {
    public:
        static constexpr bool value = JsonWrappable<std::variant<Ts...>,void>::value;
    };
    template <class... Ts>
    class JsonEncoder<std::tuple<Ts...>, std::enable_if_t<JsonWrappable<std::tuple<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static void write_impl(nlohmann::json &output, std::tuple<Ts...> const &data) {
            if constexpr (Index >= 0 && Index < sizeof...(Ts)) {
                using F = std::tuple_element_t<Index, std::tuple<Ts...>>;
                output.push_back(nlohmann::json());
                JsonEncoder<F>::write(output.back(), std::nullopt, std::get<Index>(data));
                write_impl<Index+1>(output, data);
            }
        }
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::tuple<Ts...> const &data) {
            if constexpr (sizeof...(Ts) >= 0) {
                write_impl<0>((key?output[*key]:output), data);
            }
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
    template <class T>
    struct JsonWrappable<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static constexpr bool value_internal() {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (!JsonWrappable<F>::value) {
                    return false;
                } else {
                    return value_internal<FieldCount,FieldIndex+1>();
                }
            } else {
                return true;
            }
        }
    public:
        static constexpr bool value = value_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0>();
    };

    using JsonFieldMapping = std::unordered_map<
        std::type_index, std::unordered_map<std::string, std::string>
    >; //key in the inside map is structure field name, value is JSON name

    template <class T, typename Enable=void>
    class JsonDecoder {};

    template <class IntType>
    class JsonDecoder<IntType, std::enable_if_t<
        (std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType>)
        , void
    >> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, IntType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                data = boost::lexical_cast<IntType>(s);
            } else if (i.is_null()) {
                data = (IntType) 0;
            } else {
                i.get_to(data);
            }
        }
    };
    template <>
    class JsonDecoder<bool, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, bool &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                data = boost::lexical_cast<bool>(s);
            } else if (i.is_null()) {
                data = false;
            } else {
                i.get_to(data);
            }
        }
    };
    template <class T>
    class JsonDecoder<T, std::enable_if_t<std::is_enum_v<T>, void>> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::underlying_type_t<T> t;
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                t = boost::lexical_cast<std::underlying_type_t<T>>(s);
            } else if (i.is_null()) {
                t = static_cast<T>(std::underlying_type_t<T> (0));
            } else {
                i.get_to(t);
            }
            data = static_cast<T>(t);
        }
    };
    template <>
    class JsonDecoder<std::string, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::string &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                data = "";
            } else {
                i.get_to(data);
            }
        }
    };
    template <std::size_t N>
    class JsonDecoder<std::array<char,N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<char,N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::memset(data.data(), 0, N);
            std::string s;
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                s = "";
            } else {
                i.get_to(s);
            }
            std::memcpy(data.data(), s.data(), std::min(s.length(), N));
        }
    };
    template <>
    class JsonDecoder<ByteData, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, ByteData &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::vector<unsigned int> x;
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                x.clear();
            } else {
                i.get_to(x);
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
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<unsigned char,N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::memset(data, 0, N);
            std::vector<unsigned int> x;
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                x.clear();
            } else {
                i.get_to(x);
            }
            for (std::size_t ii=0; ii<x.size() && ii<N; ++ii) {
                data[ii] = (char) x[ii];
            }
        }
    };
    template <>
    class JsonDecoder<std::tm, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::tm &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            if (key) {
                input.at(*key).get_to(s);
            } else {
                input.get_to(s);
            }
            std::stringstream ss(s);
            ss >> std::get_time(&data
                , ((s.length()==8)?"%Y%m%d":((s.length()==10)?"%Y-%m-%d":"%Y-%m-%dT%H:%M:%S")));
        }
    };
    template <>
    class JsonDecoder<DateHolder, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, DateHolder &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            if (key) {
                input.at(*key).get_to(s);
            } else {
                input.get_to(s);
            }
            if (s.length() == 8) {
                data.year = (uint16_t) std::stoi(s.substr(0,4));
                data.month = (uint8_t) std::stoi(s.substr(4,2));
                data.day = (uint8_t) std::stoi(s.substr(6,2));
            } else if (s.length() >= 10) {
                data.year = (uint16_t) std::stoi(s.substr(0,4));
                data.month = (uint8_t) std::stoi(s.substr(5,2));
                data.day = (uint8_t) std::stoi(s.substr(8,2));
            } else {
                data.year = 0;
                data.month = 0;
                data.day = 0;
            }
        }
    };
    template <>
    class JsonDecoder<std::chrono::system_clock::time_point, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::chrono::system_clock::time_point &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
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
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::vector<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i) {
                JsonDecoder<T>::read(item, std::nullopt, data[ii], mapping);
                ++ii;
            }
        }
    };
    template <class T>
    class JsonDecoder<std::list<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::list<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i) {
                data.push_back(T {});
                JsonDecoder<T>::read(item, std::nullopt, data.back(), mapping);
            }
        }
    };
    template <class T>
    class JsonDecoder<std::valarray<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::valarray<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i) {
                JsonDecoder<T>::read(item, std::nullopt, data[ii], mapping);
                ++ii;
            }
        }
    };
    template <class T, std::size_t N>
    class JsonDecoder<std::array<T,N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<T,N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            std::size_t ii = 0;
            for (auto const &item : i) {
                if (ii < N) {
                    JsonDecoder<T>::read(item, std::nullopt, data[ii], mapping);
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
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::map<std::string,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i.items()) {
                auto iter = data.insert({item.key(), T{}}).first;
                JsonDecoder<T>::read(item.value(), std::nullopt, iter->second, mapping);
            }
        }
    };
    template <class T>
    class JsonDecoder<std::unordered_map<std::string,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::unordered_map<std::string,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i.items()) {
                auto iter = data.insert({item.key(), T{}}).first;
                JsonDecoder<T>::read(item.value(), std::nullopt, iter->second, mapping);
            }
        }
    };
    template <class K, class D, class Cmp>
    class JsonDecoder<std::map<K,D,Cmp>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::map<K,D,Cmp> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            K k;
            D d;
            int idx = 0;
            for (auto const &item : i) {
                if (idx == 0) {
                    JsonDecoder<K>::read(item, std::nullopt, k, mapping);
                } else if (idx == 1) {
                    JsonDecoder<D>::read(item, std::nullopt, d, mapping);
                } else {
                    break;
                }
                ++idx;
            }
            data[k] = d;
        }
    };
    template <class K, class D, class Hash>
    class JsonDecoder<std::unordered_map<K,D,Hash>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::unordered_map<K,D,Hash> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            K k;
            D d;
            int idx = 0;
            for (auto const &item : i) {
                if (idx == 0) {
                    JsonDecoder<K>::read(item, std::nullopt, k, mapping);
                } else if (idx == 1) {
                    JsonDecoder<D>::read(item, std::nullopt, d, mapping);
                } else {
                    break;
                }
                ++idx;
            }
            data[k] = d;
        }
    };
    template <class T>
    class JsonDecoder<std::vector<std::pair<std::string,T>>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::vector<std::pair<std::string,T>> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i.items()) {
                std::get<0>(data[ii]) = item.key();
                JsonDecoder<T>::read(item.value(), std::nullopt, std::get<1>(data[ii]), mapping);
                ++ii;
            }
        }
    };
    template <class T>
    class JsonDecoder<std::optional<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::optional<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                if (input.contains(*key)) {
                    data = T {};
                    JsonDecoder<T>::read(input.at(*key), std::nullopt, *data, mapping);
                } else {
                    data = std::nullopt;
                }
            } else {
                if (!input.is_null()) {
                    data = T {};
                    JsonDecoder<T>::read(input, std::nullopt, *data, mapping);
                } else {
                    data = std::nullopt;
                }
            }
        }
    };
    template <>
    class JsonDecoder<VoidStruct, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, VoidStruct &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
        }
    };
    template <>
    class JsonDecoder<std::monostate, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::monostate &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
        }
    };
    
    template <int32_t N>
    class JsonDecoder<ConstType<N>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, ConstType<N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
        }
    };
    template <class T>
    class JsonDecoder<SingleLayerWrapper<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapper<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::read(input, key, data.value, mapping);
        }
    };
    template <int32_t N, class T>
    class JsonDecoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::read(input, key, data.value, mapping);
        }
    };
    template <class M, class T>
    class JsonDecoder<SingleLayerWrapperWithTypeMark<M,T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::read(input, key, data.value, mapping);
        }
    };

    template <class... Ts>
    class JsonDecoder<std::variant<Ts...>, std::enable_if_t<JsonWrappable<std::variant<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static void read_impl(nlohmann::json const &input, int index, std::variant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index == index) {
                    using F = std::variant_alternative_t<Index, std::variant<Ts...>>;
                    data.template emplace<Index>();
                    JsonDecoder<F>::read(input, std::nullopt, std::get<Index>(data), mapping);
                } else {
                    read_impl<Index+1>(input, index, data, mapping);
                }
            }
        }
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::variant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.contains("index") && i.contains("content")) {
                std::size_t index;
                JsonDecoder<std::size_t>::read(i, "index", index, mapping);
                read_impl<0>(i.at("content"), index, data, mapping);
            }
        }
    };
    template <class... Ts>
    class JsonDecoder<std::tuple<Ts...>, std::enable_if_t<JsonWrappable<std::tuple<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static void read_impl(nlohmann::json const &input, std::tuple<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                using F = std::tuple_element_t<Index, std::tuple<Ts...>>;
                JsonDecoder<F>::read(input[Index], std::nullopt, std::get<Index>(data), mapping);
                read_impl<Index+1>(input, data, mapping);
            }
        }
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, std::tuple<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            read_impl<0>((key?input.at(*key):input), data, mapping);
        }
    };

    template <class T>
    class JsonDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void read_impl(nlohmann::json const &input, T &data, JsonFieldMapping const &mapping, std::unordered_map<std::string, std::string> const *mappingForThisOne) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                std::string s {StructFieldInfo<T>::FIELD_NAMES[FieldIndex]};
                if (mappingForThisOne != nullptr) {
                    auto iter = mappingForThisOne->find(s);
                    if (iter != mappingForThisOne->end()) {
                        s = iter->second;
                    }
                }
                if (input.contains(s)) {
                    JsonDecoder<F>::read(input, s, data.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()), mapping);
                }
                read_impl<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne);
            }
        }
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data);
            std::unordered_map<std::string, std::string> const *mappingForThisOne = nullptr;
            if (!mapping.empty()) {
                auto iter = mapping.find(std::type_index(typeid(T)));
                if (iter != mapping.end()) {
                    mappingForThisOne = &(iter->second);
                }
            }
            read_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>((key?input.at(*key):input), data, mapping, mappingForThisOne);
        }
    };
    
    template <class T, typename=void>
    class Json {};

    template <class T>
    class Json<T, std::enable_if_t<JsonWrappable<T>::value, void>> {
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

        void toNlohmannJson(nlohmann::json &output) const {
            JsonEncoder<T>::write(output, std::nullopt, t_);
        }
        //The method names are deliberately made different from
        //proto method names
        void writeToStream(std::ostream &os) const {
            nlohmann::json output;
            JsonEncoder<T>::write(output, std::nullopt, t_);
            os << output;
        }
        void writeToString(std::string *s) const {
            nlohmann::json output;
            JsonEncoder<T>::write(output, std::nullopt, t_);
            *s = output.dump();
        }
        bool fromStringView(std::string_view const &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            nlohmann::json x;
#ifdef _MSC_VER
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
            >> x;
            JsonDecoder<T>::read(x, std::nullopt, t_, mapping);
            return true;
        }
        bool fromString(std::string const &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return fromStringView(std::string_view(s), mapping);
        }
        bool fromStream(std::istream &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            nlohmann::json x;
            s >> x;
            JsonDecoder<T>::read(x, std::nullopt, t_, mapping);
            return true;
        }
        bool fromNlohmannJson(nlohmann::json const &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::read(x, std::nullopt, t_, mapping);
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
            nlohmann::json output;
            JsonEncoder<T>::write(output, std::nullopt, t);
            os << output;
        }
        static bool runDeserialize(T &t, std::string_view const &input, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            nlohmann::json x;
#ifdef _MSC_VER
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(input.data(), input.length())
#else
            boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(input.begin(), input.size())
#endif
            >> x;
            JsonDecoder<T>::read(x, std::nullopt, t, mapping);
            return true;
        }
    };
    template <class T>
    class Json<T *, std::enable_if_t<JsonWrappable<T>::value, void>> {
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

        void toNlohmannJson(nlohmann::json &output) const {
            if (t_) {
                JsonEncoder<T>::write(output, std::nullopt, *t_);
            }
        }
        void writeToStream(std::ostream &os) const {
            if (t_) {
                nlohmann::json output;
                JsonEncoder<T>::write(output, std::nullopt, *t_);
                os << output;
            }
        }
        void writeToString(std::string *s) const {
            if (t_) {
                nlohmann::json output;
                JsonEncoder<T>::write(output, std::nullopt, *t_);
                *s = output.dump();
            }
        }
        bool fromStringView(std::string_view const &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (t_) {
                nlohmann::json x;
#ifdef _MSC_VER
                boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.data(), s.length())
#else
                boost::iostreams::stream<boost::iostreams::basic_array_source<char>>(s.begin(), s.size())
#endif
                >> x;
                JsonDecoder<T>::read(x, std::nullopt, *t_, mapping);
                return true;
            } else {
                return false;
            }
        }
        bool fromString(std::string const &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return fromStringView(std::string_view(s), mapping);
        }
        bool fromStream(std::istream &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (t_) {
                nlohmann::json x;
                s >> x;
                JsonDecoder<T>::read(x, std::nullopt, *t_, mapping);
                return true;
            } else {
                return false;
            }
        }
        bool fromNlohmannJson(nlohmann::json const &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (t_) {
                JsonDecoder<T>::read(x, std::nullopt, *t_, mapping);
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

    template <>
    class JsonEncoder<nlohmann::json, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, nlohmann::json const &data) {
            (key?output[*key]:output) = data;
        }
    };
    template <>
    struct JsonWrappable<nlohmann::json, void> {
        static constexpr bool value = true;
    };
    template <>
    class JsonDecoder<nlohmann::json, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, nlohmann::json &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data = (key?input[*key]:input);
        }
    };
    template <class T>
    class JsonEncoder<Json<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, Json<T> const &data) {
            data.toNlohmannJson(key?output[*key]:output);
        }
    };
    template <class T>
    struct JsonWrappable<Json<T>, void> {
        static constexpr bool value = true;
    };
    template <class T>
    class JsonDecoder<Json<T>, void> {
    public:
        static void read(nlohmann::json const &input, std::optional<std::string> const &key, Json<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.fromNlohmannJson((key?input[*key]:input), mapping);
        }
    };

} } } } }

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace bytedata_utils {
    //allow JSON wrapper to be used for encode/decode
    //without exposing proto-style methods
    template <class T>
    struct RunSerializer<nlohmann_json_interop::Json<T>, void> {
        static std::string apply(nlohmann_json_interop::Json<T> const &data) {
            std::ostringstream oss;
            nlohmann_json_interop::Json<T>::runSerialize(*data, oss);
            return oss.str();
        }
    };
    template <class T>
    struct RunDeserializer<nlohmann_json_interop::Json<T>, void> {
        static std::optional<nlohmann_json_interop::Json<T>> apply(std::string_view const &data) {
            T t;
            if (nlohmann_json_interop::Json<T>::runDeserialize(t, data)) {
                return nlohmann_json_interop::Json<T> {t};
            } else {
                return std::nullopt;
            }
        }
        static std::optional<nlohmann_json_interop::Json<T>> apply(std::string const &data) {
            return apply(std::string_view {data});
        }
        static bool applyInPlace(nlohmann_json_interop::Json<T> &output, std::string_view const &data) {
            return output.fromStringView(data);
        }
        static bool applyInPlace(nlohmann_json_interop::Json<T> &output, std::string const &data) {
            return output.fromString(data);
        }
    };
    template <class T>
    struct DirectlySerializableChecker<nlohmann_json_interop::Json<T>> {
        static constexpr bool IsDirectlySerializable() {
            return true;
        }
    };

    //allow nlohmann::json to be used in CBOR<> structure
    template <>
    struct RunCBORSerializer<nlohmann::json, void> {
        static std::string apply(nlohmann::json const &data) {
            std::vector<uint8_t> v = nlohmann::json::to_cbor(data);
            return std::string {reinterpret_cast<char *>(v.data()), v.size()};
        }
        static std::size_t apply(nlohmann::json const &data, char *output) {
            std::vector<uint8_t> v = nlohmann::json::to_cbor(data);
            std::memcpy(output, v.data(), v.size());
            return v.size();
        }
        static std::size_t calculateSize(nlohmann::json const &data) {
            return nlohmann::json::to_cbor(data).size();
        }
    };
    template <>
    struct RunCBORDeserializer<nlohmann::json, void> {
        static std::optional<std::tuple<nlohmann::json, size_t>> apply(std::string_view const &data, size_t start) {
            return std::tuple<nlohmann::json,size_t> {
                nlohmann::json::from_cbor(data.data()+start, data.length()-start), data.length()-start
            };
        }
        static std::optional<size_t> applyInPlace(nlohmann::json &output, std::string_view const &data, size_t start) {
            output = nlohmann::json::from_cbor(data.data()+start, data.length()-start);
            return (size_t) (data.length()-start);
        }
    };
} } } } }

#endif
