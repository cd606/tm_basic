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
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/EncodableThroughProxy.hpp>

#include <nlohmann/json.hpp>
#include <simdjson.h>
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
        (std::is_integral_v<IntType> && std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType> && !std::is_same_v<IntType,char>)
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
        (std::is_integral_v<IntType> && std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType> && !std::is_same_v<IntType,char>)
        , void
    >> {
        static constexpr bool value = true;
    };
    template <class FloatType>
    class JsonEncoder<FloatType, std::enable_if_t<
        std::is_floating_point_v<FloatType>
        , void
    >> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, FloatType data) {
            if (key) {
                output[*key] = data;
            } else {
                output = data;
            }
        }
    };
    template <class FloatType>
    struct JsonWrappable<FloatType, std::enable_if_t<
        std::is_floating_point_v<FloatType>
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
    //for the enums that has string representation, we use the ConvertibleWithString encoder/decoder
    template <class T>
    class JsonEncoder<T, std::enable_if_t<std::is_enum_v<T> && !bytedata_utils::IsEnumWithStringRepresentation<T>::value, void>> {
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
    struct JsonWrappable<T, std::enable_if_t<std::is_enum_v<T> && !bytedata_utils::IsEnumWithStringRepresentation<T>::value, void>> {
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
    template <class T>
    class JsonEncoder<T, std::enable_if_t<(!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && ConvertibleWithString<T>::value, void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, T const &data) {
            if (key) {
                output[*key] = ConvertibleWithString<T>::toString(data);
            } else {
                output = ConvertibleWithString<T>::toString(data);
            }
        }
    };
    template <class T>
    struct JsonWrappable<T, std::enable_if_t<(!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && ConvertibleWithString<T>::value, void>> {
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
    class JsonEncoder<std::deque<T>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::deque<T> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                o.push_back(nlohmann::json());
                JsonEncoder<T>::write(o.back(), std::nullopt, item);
            }
        }
    };
    template <class T>
    struct JsonWrappable<std::deque<T>, void> {
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

    template <class T>
    class JsonEncoder<std::vector<std::tuple<std::string,T>>, void> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::vector<std::tuple<std::string,T>> const &data) {
            auto &o = (key?output[*key]:output);
            for (auto const &item : data) {
                JsonEncoder<T>::write(o, std::get<0>(item), std::get<1>(item));
            }
        }
    };
    template <class T>
    struct JsonWrappable<std::vector<std::tuple<std::string,T>>, void> {
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
        static void write(nlohmann::json &/*output*/, std::optional<std::string> const &/*key*/, VoidStruct const &/*data*/) {
        }
    };
    template <>
    struct JsonWrappable<VoidStruct, void> {
        static constexpr bool value = true;
    };
    template <>
    class JsonEncoder<std::monostate, void> {
    public:
        static void write(nlohmann::json &/*output*/, std::optional<std::string> const &/*key*/, std::monostate const &/*data*/) {
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
            if (key) {
                output[*key] = N;
            } else {
                output = N;
            }
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

    struct JsonVariantTypeMark {};

    template <class... Ts>
    using JsonVariant = SingleLayerWrapperWithTypeMark<JsonVariantTypeMark, std::variant<std::monostate,Ts...>>;

    template <class M, class T>
    class JsonEncoder<SingleLayerWrapperWithTypeMark<M,T>, std::enable_if_t<!std::is_same_v<M,JsonVariantTypeMark>, void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> const &data) {
            JsonEncoder<T>::write(output, key, data.value);
        }
    };
    template <class M, class T>
    struct JsonWrappable<SingleLayerWrapperWithTypeMark<M,T>, std::enable_if_t<!std::is_same_v<M,JsonVariantTypeMark>, void>> {
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
    struct JsonWrappable<JsonVariant<Ts...>, void> {
    private:
        template <std::size_t Index>
        static constexpr bool value_internal() {
            if constexpr (Index < sizeof...(Ts)) {
                if constexpr (!JsonWrappable<std::variant_alternative_t<Index+1,std::variant<std::monostate,Ts...>>>::value) {
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
    class JsonEncoder<JsonVariant<Ts...>, std::enable_if_t<JsonWrappable<JsonVariant<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static void write_impl(nlohmann::json &output, JsonVariant<Ts...> const &data) {
            if constexpr (Index >= 0 && Index < sizeof...(Ts)) {
                if (Index+1 == data.value.index()) {
                    using F = std::variant_alternative_t<Index+1,std::variant<std::monostate,Ts...>>;
                    JsonEncoder<F>::write(
                        output
                        , std::nullopt
                        , std::get<Index+1>(data.value)
                    );
                } else {
                    write_impl<Index+1>(output, data);
                }
            }
        }
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, JsonVariant<Ts...> const &data) {
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
    class JsonEncoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo && (!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) &&  !ConvertibleWithString<T>::value, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void write_impl(nlohmann::json &output, T const &data) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                JsonEncoder<F>::write(
                    output
                    , std::string(StructFieldInfo<T>::FIELD_NAMES[FieldIndex])
                    , StructFieldTypeInfo<T,FieldIndex>::constAccess(data)
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
    struct JsonWrappable<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo && (!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && !ConvertibleWithString<T>::value, void>> {
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

    template <class T>
    class JsonEncoder<T, std::enable_if_t<bytedata_utils::ProtobufStyleSerializableChecker<T>::IsProtobufStyleSerializable(), void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, T const &data) {
            ByteData b;
            data.SerializeToString(&(b.content));
            JsonEncoder<ByteData>::write(output, key, b);
        }
    };
    template <class T>
    struct JsonWrappable<T, std::enable_if_t<bytedata_utils::ProtobufStyleSerializableChecker<T>::IsProtobufStyleSerializable(), void>> {
    public:
        static constexpr bool value = true;
    };

    using JsonFieldMapping = std::unordered_map<
        std::type_index, std::unordered_map<std::string, std::string>
    >; //key in the inside map is structure field name, value is JSON name

    template <class T, typename Enable=void>
    class JsonDecoder {};

    template <class IntType>
    class JsonDecoder<IntType, std::enable_if_t<
        (std::is_integral_v<IntType> && std::is_arithmetic_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType> && !std::is_same_v<IntType,char>)
        , void
    >> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, IntType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                try {
                    data = boost::lexical_cast<IntType>(s);
                    return true;
                } catch (boost::bad_lexical_cast const &) {
                    data = (IntType) 0;
                    return false;
                }
            } else if (i.is_null()) {
                data = (IntType) 0;
                return false;
            } else {
                i.get_to(data);
                return true;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, IntType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    if (input[*key].is_string()) {
                        try {
                            data = boost::lexical_cast<IntType>((std::string_view) input[*key]);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (IntType) 0;
                            return false;
                        }
                    } else {
                        if constexpr (std::is_signed_v<IntType>) {
                            data = static_cast<IntType>((int64_t) input[*key]);
                            return true;
                        } else {
                            data = static_cast<IntType>((uint64_t) input[*key]);
                            return true;
                        }
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (IntType) 0;
                    return false;
                }
            } else {
                try {
                    if (input.is_string()) {
                        try {
                            data = boost::lexical_cast<IntType>((std::string_view) input);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (IntType) 0;
                            return false;
                        }
                    } else {
                        if constexpr (std::is_signed_v<IntType>) {
                            data = static_cast<IntType>((int64_t) input);
                            return true;
                        } else {
                            data = static_cast<IntType>((uint64_t) input);
                            return true;
                        }
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (IntType) 0;
                    return false;
                }
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, IntType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    auto x = input.get_object()[*key];
                    auto x_s = x.get_string();
                    if (x_s.error() != simdjson::INCORRECT_TYPE) {
                        try {
                            data = boost::lexical_cast<IntType>((std::string_view) x_s.value());
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (IntType) 0;
                            return false;
                        }
                    } else {
                        if constexpr (std::is_signed_v<IntType>) {
                            data = static_cast<IntType>(x.get_int64());
                            return true;
                        } else {
                            data = static_cast<IntType>(x.get_uint64());
                            return true;
                        }
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (IntType) 0;
                    return false;
                }
            } else {
                try {
                    auto i_s = input.get_string();
                    if (i_s.error() != simdjson::INCORRECT_TYPE) {
                        try {
                            data = boost::lexical_cast<IntType>((std::string_view) i_s.value());
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (IntType) 0;
                            return false;
                        }
                    } else {
                        if constexpr (std::is_signed_v<IntType>) {
                            data = static_cast<IntType>(input.get_int64());
                            return true;
                        } else {
                            data = static_cast<IntType>(input.get_uint64());
                            return true;
                        }
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (IntType) 0;
                    return false;
                }
            }
        }
    };
    template <class FloatType>
    class JsonDecoder<FloatType, std::enable_if_t<
        std::is_floating_point_v<FloatType>
        , void
    >> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, FloatType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                try {
                    data = boost::lexical_cast<FloatType>(s);
                    return true;
                } catch (boost::bad_lexical_cast const &) {
                    data = (FloatType) 0;
                    return false;
                }
            } else if (i.is_null()) {
                data = (FloatType) 0;
                return false;
            } else {
                i.get_to(data);
                return true;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, FloatType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    if (input[*key].is_string()) {
                        try {
                            data = boost::lexical_cast<FloatType>((std::string_view) input[*key]);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (FloatType) 0;
                            return false;
                        }
                    } else {
                        data = static_cast<FloatType>((double) input[*key]);
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (FloatType) 0;
                    return false;
                }
            } else {
                try {
                    if (input.is_string()) {
                        try {
                            data = boost::lexical_cast<FloatType>((std::string_view) input);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (FloatType) 0;
                            return false;
                        }
                    } else {
                        data = static_cast<FloatType>((double) input);
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (FloatType) 0;
                    return false;
                }
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, FloatType &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    auto x = input.get_object()[*key];
                    auto x_s = x.get_string();
                    if (x_s.error() != simdjson::INCORRECT_TYPE) {
                        try {
                            data = boost::lexical_cast<FloatType>(x_s);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (FloatType) 0;
                            return false;
                        }
                    } else {
                        data = static_cast<FloatType>(x.get_double());
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (FloatType) 0;
                    return false;
                }
            } else {
                try {
                    auto i_s = input.get_string();
                    if (i_s.error() != simdjson::INCORRECT_TYPE) {
                        try {
                            data = boost::lexical_cast<FloatType>(i_s);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = (FloatType) 0;
                            return false;
                        }
                    } else {
                        data = static_cast<FloatType>(input.get_double());
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = (FloatType) 0;
                    return false;
                }
            }
        }
    };
    template <>
    class JsonDecoder<bool, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, bool &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                try {
                    data = boost::lexical_cast<bool>(s);
                    return true;
                } catch (boost::bad_lexical_cast const &) {
                    data = false;
                    return false;
                }
            } else if (i.is_null()) {
                data = false;
                return false;
            } else {
                i.get_to(data);
                return true;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, bool &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    if (input[*key].is_string()) {
                        try {
                            data = boost::lexical_cast<bool>((std::string_view) input[*key]);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = false;
                            return false;
                        }
                    } else {
                        data = (bool) input[*key];
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = false;
                    return false;
                }
            } else {
                try {
                    if (input.is_string()) {
                        try {
                            data = boost::lexical_cast<bool>((std::string_view) input);
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = false;
                            return false;
                        }
                    } else {
                        data = (bool) input;
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = false;
                    return false;
                }
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, bool &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    auto x = input.get_object()[*key];
                    auto x_s = x.get_string();
                    if (x_s.error() != simdjson::INCORRECT_TYPE) {
                        try {
                            data = boost::lexical_cast<bool>(x_s.value());
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = false;
                            return false;
                        }
                    } else {
                        data = x.get_bool().value();
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = false;
                    return false;
                }
            } else {
                try {
                    auto i_s = input.get_string();
                    if (i_s.error() != simdjson::INCORRECT_TYPE) {
                        try {
                            data = boost::lexical_cast<bool>(i_s.value());
                            return true;
                        } catch (boost::bad_lexical_cast) {
                            data = false;
                            return false;
                        }
                    } else {
                        data = input.get_bool().value();
                        return true;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = false;
                    return false;
                }
            }
        }
    };
    template <class T>
    class JsonDecoder<T, std::enable_if_t<std::is_enum_v<T> && !bytedata_utils::IsEnumWithStringRepresentation<T>::value, void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            std::underlying_type_t<T> t;
            auto const &i = (key?input.at(*key):input);
            bool ret = false;
            if (i.is_string()) {
                std::string s;
                i.get_to(s);
                try {
                    t = boost::lexical_cast<std::underlying_type_t<T>>(s);
                    ret = true;
                } catch (boost::bad_lexical_cast const &) {
                    t = std::underlying_type_t<T> (0);
                    ret = false;
                }
            } else if (i.is_null()) {
                t = std::underlying_type_t<T> (0);
                ret = false;
            } else {
                i.get_to(t);
                ret = true;
            }
            data = static_cast<T>(t);
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::underlying_type_t<T> u;
            auto ret = JsonDecoder<std::underlying_type_t<T>>::read_simd(input, key, u, mapping);
            if (ret) {
                data = static_cast<T>(u);
                return true;
            } else {
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::underlying_type_t<T> u;
            auto ret = JsonDecoder<std::underlying_type_t<T>>::read_simd_ondemand(input, key, u, mapping);
            if (ret) {
                data = static_cast<T>(u);
                return true;
            } else {
                return false;
            }
        }
    };
    template <>
    class JsonDecoder<std::string, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::string &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                data = "";
                return true;
            } else {
                i.get_to(data);
                return true;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::string &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                try {
                    if (input[*key].is_null()) {
                        data = "";
                        return true;
                    } else if (input[*key].is_string()) {
                        data = (std::string) ((std::string_view) input[*key]);
                        return true;
                    } else if (input[*key].is_uint64()) {
                        data = boost::lexical_cast<std::string>((uint64_t) input[*key]);
                        return true;
                    } else if (input[*key].is_int64()) {
                        data = boost::lexical_cast<std::string>((int64_t) input[*key]);
                        return true;
                    } else if (input[*key].is_double()) {
                        data = boost::lexical_cast<std::string>((double) input[*key]);
                        return true;
                    } else if (input[*key].is_bool()) {
                        data = boost::lexical_cast<std::string>((bool) input[*key]);
                        return true;
                    } else {
                        data = "";
                        return false;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = "";
                    return false;
                }
            } else {
                try {
                    if (input.is_null()) {
                        data = "";
                        return true;
                    } else if (input.is_string()) {
                        data = (std::string) ((std::string_view) input);
                        return true;
                    } else if (input.is_uint64()) {
                        data = boost::lexical_cast<std::string>((uint64_t) input);
                        return true;
                    } else if (input.is_int64()) {
                        data = boost::lexical_cast<std::string>((int64_t) input);
                        return true;
                    } else if (input.is_double()) {
                        data = boost::lexical_cast<std::string>((double) input);
                        return true;
                    } else if (input.is_bool()) {
                        data = boost::lexical_cast<std::string>((bool) input);
                        return true;
                    } else {
                        data = "";
                        return false;
                    }
                } catch (simdjson::simdjson_error const &) {
                    data = "";
                    return false;
                }
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::string &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                auto x = input.get_object()[*key];
                try {
                    auto x_s = x.get_string();
                    if (x_s.error() != simdjson::INCORRECT_TYPE) {
                        data = (std::string) (x_s.value());
                        return true;
                    } 
                    auto x_ui = x.get_uint64();
                    if (x_ui.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(x_ui.value());
                        return true;
                    } 
                    auto x_i = x.get_int64();
                    if (x_i.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(x_i.value());
                        return true;
                    } 
                    auto x_d = x.get_double();
                    if (x_d.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(x_d.value());
                        return true;
                    } 
                    auto x_b = x.get_bool();
                    if (x_b.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(x_b.value());
                        return true;
                    }
                    data = "";
                    return false;
                } catch (simdjson::simdjson_error const &) {
                    data = "";
                    return false;
                }
            } else {
                try {
                    auto i_s = input.get_string();
                    if (i_s.error() != simdjson::INCORRECT_TYPE) {
                        data = (std::string) (i_s.value());
                        return true;
                    } 
                    auto i_ui = input.get_uint64();
                    if (i_ui.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(i_ui.value());
                        return true;
                    } 
                    auto i_i = input.get_int64();
                    if (i_i.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(i_i.value());
                        return true;
                    } 
                    auto i_d = input.get_double();
                    if (i_d.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(i_d.value());
                        return true;
                    } 
                    auto i_b = input.get_bool();
                    if (i_b.error() != simdjson::INCORRECT_TYPE) {
                        data = boost::lexical_cast<std::string>(i_b.value());
                        return true;
                    } 
                    data = "";
                    return false;
                } catch (simdjson::simdjson_error const &) {
                    data = "";
                    return false;
                }
            }
        }
    };
    template <class T>
    class JsonDecoder<T, std::enable_if_t<(!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && ConvertibleWithString<T>::value, void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                data = ConvertibleWithString<T>::fromString("");
                return true;
            } else if (i.is_string()) {
                std::string s;
                i.get_to(s);
                data = ConvertibleWithString<T>::fromString(s);
                return true;
            } else {
                std::string s = i.dump();
                data = ConvertibleWithString<T>::fromString(s);
                return true;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
            if (ret) {
                data = ConvertibleWithString<T>::fromString(s);
                return true;
            } else {
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
            if (ret) {
                data = ConvertibleWithString<T>::fromString(s);
                return true;
            } else {
                return false;
            }
        }
    };
    template <std::size_t N>
    class JsonDecoder<std::array<char,N>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<char,N> &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            std::memset(data.data(), 0, N);
            std::string s;
            bool ret = false;
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                s = "";
                ret = false;
            } else {
                i.get_to(s);
                ret = true;
            }
            std::memcpy(data.data(), s.data(), std::min(s.length(), N));
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::array<char, N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
            if (ret) {
                std::memcpy(data.data(), s.data(), std::min(s.length(), N));
                return true;
            } else {
                std::memset(data.data(), 0, N);
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::array<char, N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
            if (ret) {
                std::memcpy(data.data(), s.data(), std::min(s.length(), N));
                return true;
            } else {
                std::memset(data.data(), 0, N);
                return false;
            }
        }
    };
    template <>
    class JsonDecoder<ByteData, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, ByteData &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            std::vector<unsigned int> x;
            bool ret = false;
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                x.clear();
                ret = false;
            } else {
                i.get_to(x);
                ret = true;
            }
            data.content.resize(x.size());
            for (std::size_t ii=0; ii<x.size(); ++ii) {
                data.content[ii] = (char) x[ii];
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, ByteData &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::vector<unsigned int> x;
            try {
                if (key) {
                    for (uint64_t item : input[*key].get_array()) {
                        x.push_back(static_cast<unsigned int>(item));
                    }
                } else {
                    for (uint64_t item : input.get_array()) {
                        x.push_back(static_cast<unsigned int>(item));
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                data = ByteData {};
                return false;
            }
            data.content.resize(x.size());
            for (std::size_t ii=0; ii<x.size(); ++ii) {
                data.content[ii] = (char) x[ii];
            }
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, ByteData &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::vector<unsigned int> x;
            try {
                if (key) {
                    for (auto item : input.get_object()[*key].get_array()) {
                        x.push_back(static_cast<unsigned int>(item.get_uint64()));
                    }
                } else {
                    for (auto item : input.get_array()) {
                        x.push_back(static_cast<unsigned int>(item.get_uint64()));
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                data = ByteData {};
                return false;
            }
            data.content.resize(x.size());
            for (std::size_t ii=0; ii<x.size(); ++ii) {
                data.content[ii] = (char) x[ii];
            }
            return true;
        }
    };
    template <std::size_t N>
    class JsonDecoder<std::array<unsigned char,N>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<unsigned char,N> &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            bool ret = false;
            std::memset(data, 0, N);
            std::vector<unsigned int> x;
            auto const &i = (key?input.at(*key):input);
            if (i.is_null()) {
                x.clear();
                ret = false;
            } else {
                i.get_to(x);
                ret = true;
            }
            for (std::size_t ii=0; ii<x.size() && ii<N; ++ii) {
                data[ii] = (char) x[ii];
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::array<unsigned char, N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::memset(data.data(), 0, N);
            std::vector<unsigned int> x;
            try {
                if (key) {
                    for (uint64_t item : input[*key].get_array()) {
                        x.push_back(static_cast<unsigned int>(item));
                    }
                } else {
                    for (uint64_t item : input.get_array()) {
                        x.push_back(static_cast<unsigned int>(item));
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                return false;
            }
            for (std::size_t ii=0; ii<x.size() && ii<N; ++ii) {
                data[ii] = (char) x[ii];
            }
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::array<unsigned char, N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::memset(data.data(), 0, N);
            std::vector<unsigned int> x;
            try {
                if (key) {
                    for (auto item : input.get_object()[*key].get_array()) {
                        x.push_back(static_cast<unsigned int>(item.get_uint64()));
                    }
                } else {
                    for (auto item : input.get_array()) {
                        x.push_back(static_cast<unsigned int>(item.get_uint64()));
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                return false;
            }
            for (std::size_t ii=0; ii<x.size() && ii<N; ++ii) {
                data[ii] = (char) x[ii];
            }
            return true;
        }
    };
    template <>
    class JsonDecoder<std::tm, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::tm &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            bool ret = false;
            std::string s;
            if (key) {
                if (!input.at(*key).is_string()) {
                    ret = false;
                } else {
                    input.at(*key).get_to(s);
                    ret = true;
                }
            } else {
                if (!input.is_string()) {
                    ret = false;
                } else {
                    input.get_to(s);
                    ret = true;
                }
            }
            if (ret) {
                std::stringstream ss(s);
                ss >> std::get_time(&data
                    , ((s.length()==8)?"%Y%m%d":((s.length()==10)?"%Y-%m-%d":"%Y-%m-%dT%H:%M:%S")));
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::tm &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
            if (ret) {
                std::stringstream ss(s);
                ss >> std::get_time(&data
                    , ((s.length()==8)?"%Y%m%d":((s.length()==10)?"%Y-%m-%d":"%Y-%m-%dT%H:%M:%S")));
                return true;
            } else {
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::tm &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
            if (ret) {
                std::stringstream ss(s);
                ss >> std::get_time(&data
                    , ((s.length()==8)?"%Y%m%d":((s.length()==10)?"%Y-%m-%d":"%Y-%m-%dT%H:%M:%S")));
                return true;
            } else {
                return false;
            }
        }
    };
    template <>
    class JsonDecoder<DateHolder, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, DateHolder &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            bool ret = false;
            std::string s;
            if (key) {
                if (input.at(*key).is_null()) {
                    s = "";
                    ret = true;
                } else if (!input.at(*key).is_string()) {
                    ret = false;
                } else {
                    input.at(*key).get_to(s);
                    ret = true;
                }
            } else {
                if (input.is_null()) {
                    s = "";
                    ret = true;
                } else if (!input.is_string()) {
                    ret = false;
                } else {
                    input.get_to(s);
                    ret = true;
                }
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
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, DateHolder &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
            if (!ret) {
                data = DateHolder {0, 0, 0};
                return false;
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
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, DateHolder &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
            if (!ret) {
                data = DateHolder {0, 0, 0};
                return false;
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
            return ret;
        }
    };
    template <>
    class JsonDecoder<std::chrono::system_clock::time_point, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::chrono::system_clock::time_point &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            bool ret = false;
            std::string s;
            if (key) {
                if (!input.at(*key).is_string()) {
                    ret = false;
                } else {
                    input.at(*key).get_to(s);
                    ret = true;
                }
            } else {
                if (!input.is_string()) {
                    ret = false;
                } else {
                    input.get_to(s);
                    ret = true;
                }
            }
            if (ret) {
                data = infra::withtime_utils::parseLocalTime(std::string_view(s));
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::chrono::system_clock::time_point &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
            if (ret) {
                data = infra::withtime_utils::parseLocalTime(std::string_view(s));
                return true;
            } else {
                data = std::chrono::system_clock::time_point {};
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::chrono::system_clock::time_point &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::string s;
            auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
            if (ret) {
                data = infra::withtime_utils::parseLocalTime(std::string_view(s));
                return true;
            } else {
                data = std::chrono::system_clock::time_point {};
                return false;
            }
        }
    };
    template <class T>
    class JsonDecoder<std::vector<T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::vector<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            bool ret = true;
            for (auto const &item : i) {
                if (!JsonDecoder<T>::read(item, std::nullopt, data[ii], mapping)) {
                    ret = false;
                }
                ++ii;
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::vector<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto const &arr = input[*key].get_array();
                    data.resize(arr.size());
                    std::size_t ii = 0;
                    for (auto const &item : arr) {
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data[ii], mapping)) {
                            ret = false;
                        }
                        ++ii;
                    }
                } else {
                    auto const &arr = input.get_array();
                    data.resize(arr.size());
                    std::size_t ii = 0;
                    for (auto const &item : arr) {
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data[ii], mapping)) {
                            ret = false;
                        }
                        ++ii;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::vector<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_array():input.get_array())) {
                    data.push_back(T {});
                    if (!JsonDecoder<T>::read_simd_ondemand(item, std::nullopt, data.back(), mapping)) {
                        ret = false;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::list<T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::list<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            bool ret = true;
            for (auto const &item : i) {
                data.push_back(T {});
                if (!JsonDecoder<T>::read(item, std::nullopt, data.back(), mapping)) {
                    ret = false;
                }
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::list<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto const &arr = input[*key].get_array();
                    for (auto const &item : arr) {
                        data.push_back(T {});
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data.back(), mapping)) {
                            ret = false;
                        }
                    }
                } else {
                    auto const &arr = input.get_array();
                    for (auto const &item : arr) {
                        data.push_back(T {});
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data.back(), mapping)) {
                            ret = false;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                return false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::list<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_array():input.get_array())) {
                    data.push_back(T {});
                    if (!JsonDecoder<T>::read_simd_ondemand(item, std::nullopt, data.back(), mapping)) {
                        ret = false;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::deque<T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::deque<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            bool ret = true;
            for (auto const &item : i) {
                data.push_back(T {});
                if (!JsonDecoder<T>::read(item, std::nullopt, data.back(), mapping)) {
                    ret = false;
                }
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::deque<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto const &arr = input[*key].get_array();
                    for (auto const &item : arr) {
                        data.push_back(T {});
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data.back(), mapping)) {
                            ret = false;
                        }
                    }
                } else {
                    auto const &arr = input.get_array();
                    for (auto const &item : arr) {
                        data.push_back(T {});
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data.back(), mapping)) {
                            ret = false;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                return false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::deque<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_array():input.get_array())) {
                    data.push_back(T {});
                    if (!JsonDecoder<T>::read_simd_ondemand(item, std::nullopt, data.back(), mapping)) {
                        ret = false;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::valarray<T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::valarray<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            bool ret = true;
            std::size_t ii = 0;
            for (auto const &item : i) {
                if (!JsonDecoder<T>::read(item, std::nullopt, data[ii], mapping)) {
                    ret = false;
                }
                ++ii;
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::valarray<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            bool ret = true;
            std::size_t ii = 0;
            try {
                if (key) {
                    auto const &arr = input[*key].get_array();
                    data.resize(arr.size());
                    for (simdjson::dom::element const &item : arr) {
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data[ii], mapping)) {
                            ret = false;
                        }
                        ++ii;
                    }
                } else {
                    auto const &arr = input.get_array();
                    data.resize(arr.size());
                    for (simdjson::dom::element const &item : arr) {
                        if (!JsonDecoder<T>::read_simd(item, std::nullopt, data[ii], mapping)) {
                            ret = false;
                        }
                        ++ii;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                return false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::valarray<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::vector<T> v;
            if (!JsonDecoder<std::vector<T>>::read_simd_ondemand(input, key, v, mapping)) {
                return false;
            }
            data.resize(v.size());
            std::size_t ii = 0;
            for (auto &&item : std::move(v)) {
                data[ii] = std::move(item);
                ++ii;
            }
            return true;
        }
    };
    template <class T, std::size_t N>
    class JsonDecoder<std::array<T,N>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::array<T,N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            std::size_t ii = 0;
            bool ret = true;
            for (auto const &item : i) {
                if (ii < N) {
                    if (!JsonDecoder<T>::read(item, std::nullopt, data[ii], mapping)) {
                        ret = false;
                    }
                } else {
                    break;
                }
                ++ii;
            }
            if (ii < N) {
                ret = false;
            }
            for (; ii<N; ++ii) {
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data[ii]);
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::array<T, N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::size_t ii = 0;
            bool ret = true;
            try {
                if (key) {
                    for (simdjson::dom::element const &item : input[*key].get_array()) {
                        if (ii < N) {
                            if (!JsonDecoder<T>::read_simd(item, std::nullopt, data[ii], mapping)) {
                                ret = false;
                            }
                        } else {
                            break;
                        }
                        ++ii;
                    }
                } else {
                    for (simdjson::dom::element const &item : input.get_array()) {
                        if (ii < N) {
                            if (!JsonDecoder<T>::read_simd(item, std::nullopt, data[ii], mapping)) {
                                ret = false;
                            }
                        } else {
                            break;
                        }
                        ++ii;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            if (ii < N) {
                ret = false;
            }
            for (; ii<N; ++ii) {
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data[ii]);
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::array<T, N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::size_t ii = 0;
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_array():input.get_array())) {
                    if (ii < N) {
                        if (!JsonDecoder<T>::read_simd_ondemand(item, std::nullopt, data[ii], mapping)) {
                            ret = false;
                        }
                    } else {
                        break;
                    }
                    ++ii;
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            if (ii < N) {
                ret = false;
            }
            for (; ii<N; ++ii) {
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data[ii]);
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::map<std::string,T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::map<std::string,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i.items()) {
                auto iter = data.insert({item.key(), T{}}).first;
                if (!JsonDecoder<T>::read(item.value(), std::nullopt, iter->second, mapping)) {
                    ret = false;
                }
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::map<std::string, T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    for (auto const &[k, d] : input[*key].get_object()) {
                        auto iter = data.insert({(std::string) k, T{}}).first;
                        if (!JsonDecoder<T>::read_simd(d, std::nullopt, iter->second, mapping)) {
                            ret = false;
                        }
                    }
                } else {
                    for (auto const &[k, d] : input.get_object()) {
                        auto iter = data.insert({(std::string) k, T{}}).first;
                        if (!JsonDecoder<T>::read_simd(d, std::nullopt, iter->second, mapping)) {
                            ret = false;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::map<std::string, T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_object():input.get_object())) {
                    auto iter = data.insert({std::string {item.unescaped_key().value()}, T{}}).first;
                    auto d = item.value();
                    if (!JsonDecoder<T>::read_simd_ondemand(d, std::nullopt, iter->second, mapping)) {
                        ret = false;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::unordered_map<std::string,T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::unordered_map<std::string,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            auto const &i = (key?input.at(*key):input);
            for (auto const &item : i.items()) {
                auto iter = data.insert({item.key(), T{}}).first;
                if (!JsonDecoder<T>::read(item.value(), std::nullopt, iter->second, mapping)) {
                    ret = false;
                }
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::unordered_map<std::string, T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    for (auto const &[k, d] : input[*key].get_object()) {
                        auto iter = data.insert({(std::string) k, T{}}).first;
                        if (!JsonDecoder<T>::read_simd(d, std::nullopt, iter->second, mapping)) {
                            ret = false;
                        }
                    }
                } else {
                    for (auto const &[k, d] : input.get_object()) {
                        auto iter = data.insert({(std::string) k, T{}}).first;
                        if (!JsonDecoder<T>::read_simd(d, std::nullopt, iter->second, mapping)) {
                            ret = false;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::unordered_map<std::string, T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_object():input.get_object())) {
                    auto iter = data.insert({std::string {item.unescaped_key().value()}, T{}}).first;
                    auto d = item.value();
                    if (!JsonDecoder<T>::read_simd_ondemand(d, std::nullopt, iter->second, mapping)) {
                        ret = false;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class K, class D, class Cmp>
    class JsonDecoder<std::map<K,D,Cmp>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<D>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::map<K,D,Cmp> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            bool ret = true;
            if (i.is_array()) {
                for (auto const &item : i) {
                    K k;
                    D d;
                    if (!JsonDecoder<K>::read(item[0], std::nullopt, k, mapping)) {
                        ret = false;
                    }
                    if (!JsonDecoder<D>::read(item[1], std::nullopt, d, mapping)) {
                        ret = false;
                    }
                    data[k] = d;
                }
            } else {
                for (auto const &item : i.items()) {
                    K k;
                    D d;
                    if (!JsonDecoder<K>::read(item.key(), std::nullopt, k, mapping)) {
                        ret = false;
                    }
                    if (!JsonDecoder<D>::read(item.value(), std::nullopt, d, mapping)) {
                        ret = false;
                    }
                    data[k] = d;
                }
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::map<K,D,Cmp> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto const &x = input[*key];
                    if (x.is_array()) {
                        for (auto const &item : x.get_array()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read_simd(item.at(0), std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(item.at(1), std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto const &[k, d] : x.get_object()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {(std::string_view) k}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                } else {
                    if (input.is_array()) {
                        for (auto const &item : input.get_array()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read_simd(item.at(0), std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(item.at(1), std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto const &[k, d] : input.get_object()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {(std::string_view) k}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::map<K,D,Cmp> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto x = input.get_object()[*key];
                    auto arr = x.get_array();
                    if (arr.error() != simdjson::INCORRECT_TYPE) {
                        for (auto item : arr.value()) {
                            auto iter = item.get_array().begin();
                            K k1;
                            D d1;
                            auto k = *iter;
                            if (!JsonDecoder<K>::read_simd_ondemand(k, std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            ++iter;
                            auto d = *iter;
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto item : x.get_object().value()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {item.unescaped_key().value()}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            auto d = item.value();
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                } else {
                    auto arr = input.get_array();
                    if (arr.error() != simdjson::INCORRECT_TYPE) {
                        for (auto item : arr.value()) {
                            auto iter = item.get_array().begin();
                            K k1;
                            D d1;
                            auto k = *iter;
                            if (!JsonDecoder<K>::read_simd_ondemand(k, std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            ++iter;
                            auto d = *iter;
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto item : input.get_object().value()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {item.unescaped_key().value()}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            auto d = item.value();
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class K, class D, class Hash>
    class JsonDecoder<std::unordered_map<K,D,Hash>, std::enable_if_t<!std::is_same_v<K,std::string>,void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<D>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::unordered_map<K,D,Hash> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            auto const &i = (key?input.at(*key):input);
            bool ret = true;
            if (i.is_array()) {
                for (auto const &item : i) {
                    K k;
                    D d;
                    if (!JsonDecoder<K>::read(item[0], std::nullopt, k, mapping)) {
                        ret = false;
                    }
                    if (!JsonDecoder<D>::read(item[1], std::nullopt, d, mapping)) {
                        ret = false;
                    }
                    data[k] = d;
                }
            } else {
                for (auto const &item : i.items()) {
                    K k;
                    D d;
                    if (!JsonDecoder<K>::read(item.key(), std::nullopt, k, mapping)) {
                        ret = false;
                    }
                    if (!JsonDecoder<D>::read(item.value(), std::nullopt, d, mapping)) {
                        ret = false;
                    }
                    data[k] = d;
                }
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::unordered_map<K,D,Hash> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto const &x = input[*key];
                    if (x.is_array()) {
                        for (auto const &item : x.get_array()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read_simd(item.at(0), std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(item.at(1), std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto const &[k, d] : x.get_object()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {(std::string_view) k}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                } else {
                    if (input.is_array()) {
                        for (auto const &item : input.get_array()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read_simd(item.at(0), std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(item.at(1), std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto const &[k, d] : input.get_object()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {(std::string_view) k}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            if (!JsonDecoder<D>::read_simd(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::unordered_map<K,D,Hash> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto x = input.get_object()[*key];
                    auto arr = x.get_array();
                    if (arr.error() != simdjson::INCORRECT_TYPE) {
                        for (auto item : arr.value()) {
                            auto iter = item.get_array().begin();
                            K k1;
                            D d1;
                            auto k = *iter;
                            if (!JsonDecoder<K>::read_simd_ondemand(k, std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            ++iter;
                            auto d = *iter;
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto item : x.get_object().value()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {item.unescaped_key().value()}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            auto d = item.value();
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                } else {
                    auto arr = input.get_array();
                    if (arr.error() != simdjson::INCORRECT_TYPE) {
                        for (auto item : arr.value()) {
                            auto iter = item.get_array().begin();
                            K k1;
                            D d1;
                            auto k = *iter;
                            if (!JsonDecoder<K>::read_simd_ondemand(k, std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            ++iter;
                            auto d = *iter;
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    } else {
                        for (auto item : input.get_object().value()) {
                            K k1;
                            D d1;
                            if (!JsonDecoder<K>::read((nlohmann::json {nlohmann::json::string_t {item.unescaped_key().value()}})[0], std::nullopt, k1, mapping)) {
                                ret = false;
                            }
                            auto d = item.value();
                            if (!JsonDecoder<D>::read_simd_ondemand(d, std::nullopt, d1, mapping)) {
                                ret = false;
                            }
                            data[k1] = d1;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::vector<std::tuple<std::string,T>>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::vector<std::tuple<std::string,T>> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            auto const &i = (key?input.at(*key):input);
            data.resize(i.size());
            std::size_t ii = 0;
            for (auto const &item : i.items()) {
                std::get<0>(data[ii]) = item.key();
                if (!JsonDecoder<T>::read(item.value(), std::nullopt, std::get<1>(data[ii]), mapping)) {
                    ret = false;
                }
                ++ii;
            }
            return ret;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::vector<std::tuple<std::string, T>> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                if (key) {
                    auto const &obj = input[*key].get_object();
                    data.resize(obj.size());
                    std::size_t ii = 0;
                    for (auto const &[k, d] : obj) {
                        std::get<0>(data[ii]) = k;
                        if (!JsonDecoder<T>::read_simd(d, std::nullopt, std::get<1>(data[ii]), mapping)) {
                            ret = false;
                        }
                        ++ii;
                    }
                } else {
                    auto const &obj = input.get_object();
                    data.resize(obj.size());
                    std::size_t ii = 0;
                    for (auto const &[k, d] : obj) {
                        std::get<0>(data[ii]) = k;
                        if (!JsonDecoder<T>::read_simd(d, std::nullopt, std::get<1>(data[ii]), mapping)) {
                            ret = false;
                        }
                        ++ii;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::vector<std::tuple<std::string, T>> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.clear();
            bool ret = true;
            try {
                for (auto item : (key?input.get_object()[*key].get_object():input.get_object())) {
                    data.push_back({});
                    std::get<0>(data.back()) = item.unescaped_key().value();
                    auto d = item.value();
                    if (!JsonDecoder<T>::read_simd_ondemand(d, std::nullopt, std::get<1>(data.back()), mapping)) {
                        ret = false;
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                ret = false;
            }
            return ret;
        }
    };
    template <class T>
    class JsonDecoder<std::optional<T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::optional<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                if (input.contains(*key)) {
                    data = T {};
                    if (!JsonDecoder<T>::read(input.at(*key), std::nullopt, *data, mapping)) {
                        data = std::nullopt;
                    }
                } else {
                    data = std::nullopt;
                }
            } else {
                if (!input.is_null()) {
                    data = T {};
                    if (!JsonDecoder<T>::read(input, std::nullopt, *data, mapping)) {
                        data = std::nullopt;
                    }
                } else {
                    data = std::nullopt;
                }
            }
            return true;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::optional<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            try {
                if (key) {
                    if (input[*key].error() == simdjson::NO_SUCH_FIELD) {
                        data = std::nullopt;
                    } else if (input[*key].is_null()) {
                        data = std::nullopt;
                    } else {
                        data = T {};
                        if (!JsonDecoder<T>::read_simd(input[*key].value(), std::nullopt, *data, mapping)) {
                            data = std::nullopt;
                        }
                    }
                } else {
                    if (input.is_null()) {
                        data = std::nullopt;
                    } else {
                        data = T {};
                        if (!JsonDecoder<T>::read_simd(input, std::nullopt, *data, mapping)) {
                            data = std::nullopt;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                data = std::nullopt;
            }
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::optional<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            try {
                if (key) {
                    auto x = input.get_object()[*key];
                    if (x.error() == simdjson::NO_SUCH_FIELD) {
                        data = std::nullopt;
                    } else if (x.is_null()) {
                        data = std::nullopt;
                    } else {
                        data = T {};
                        if (!JsonDecoder<T>::read_simd_ondemand(x.value(), std::nullopt, *data, mapping)) {
                            data = std::nullopt;
                        }
                    }
                } else {
                    if (input.is_null()) {
                        data = std::nullopt;
                    } else {
                        data = T {};
                        if (!JsonDecoder<T>::read_simd_ondemand(input, std::nullopt, *data, mapping)) {
                            data = std::nullopt;
                        }
                    }
                }
            } catch (simdjson::simdjson_error const &) {
                data = std::nullopt;
            }
            return true;
        }
    };
    template <>
    class JsonDecoder<VoidStruct, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &/*input*/, std::optional<std::string> const &/*key*/, VoidStruct &/*data*/, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            return true;
        }
        static bool read_simd(simdjson::dom::element const &/*input*/, std::optional<std::string> const &/*key*/, VoidStruct &/*data*/, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &/*input*/, std::optional<std::string> const &/*key*/, VoidStruct &/*data*/, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            return true;
        }
    };
    template <>
    class JsonDecoder<std::monostate, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &/*input*/, std::optional<std::string> const &/*key*/, std::monostate &/*data*/, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            return true;
        }
        static bool read_simd(simdjson::dom::element const &/*input*/, std::optional<std::string> const &/*key*/, std::monostate &/*data*/, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &/*input*/, std::optional<std::string> const &/*key*/, std::monostate &/*data*/, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            return true;
        }
    };
    
    template <int32_t N>
    class JsonDecoder<ConstType<N>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, ConstType<N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            int32_t x;
            bool ret = JsonDecoder<int32_t>::read(input, key, x, mapping);
            if (!ret) {
                return false;
            }
            if (x != N) {
                return false;
            }
            return true;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, ConstType<N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            int32_t x;
            bool ret = JsonDecoder<int32_t>::read_simd(input, key, x, mapping);
            if (!ret) {
                return false;
            }
            if (x != N) {
                return false;
            }
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, ConstType<N> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            int32_t x;
            bool ret = JsonDecoder<int32_t>::read_simd_ondemand(input, key, x, mapping);
            if (!ret) {
                return false;
            }
            if (x != N) {
                return false;
            }
            return true;
        }
    };
    template <class T>
    class JsonDecoder<SingleLayerWrapper<T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapper<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read(input, key, data.value, mapping);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, SingleLayerWrapper<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd(input, key, data.value, mapping);
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, SingleLayerWrapper<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd_ondemand(input, key, data.value, mapping);
        }
    };
    template <int32_t N, class T>
    class JsonDecoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read(input, key, data.value, mapping);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd(input, key, data.value, mapping);
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, SingleLayerWrapperWithID<N,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd_ondemand(input, key, data.value, mapping);
        }
    };
    template <class M, class T>
    class JsonDecoder<SingleLayerWrapperWithTypeMark<M,T>, std::enable_if_t<!std::is_same_v<M,JsonVariantTypeMark>, void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read(input, key, data.value, mapping);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd(input, key, data.value, mapping);
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, SingleLayerWrapperWithTypeMark<M,T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd_ondemand(input, key, data.value, mapping);
        }
    };

    template <class... Ts>
    class JsonDecoder<std::variant<Ts...>, std::enable_if_t<JsonWrappable<std::variant<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static bool read_impl(nlohmann::json const &input, int index, std::variant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index == index) {
                    using F = std::variant_alternative_t<Index, std::variant<Ts...>>;
                    data.template emplace<Index>();
                    return JsonDecoder<F>::read(input, std::nullopt, std::get<Index>(data), mapping);
                } else {
                    return read_impl<Index+1>(input, index, data, mapping);
                }
            } else {
                return false;
            }
        }
        template <std::size_t Index>
        static bool read_impl_simd(simdjson::dom::element const &input, int index, std::variant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index == index) {
                    using F = std::variant_alternative_t<Index, std::variant<Ts...>>;
                    data.template emplace<Index>();
                    return JsonDecoder<F>::read_simd(input, std::nullopt, std::get<Index>(data), mapping);
                } else {
                    return read_impl_simd<Index+1>(input, index, data, mapping);
                }
            } else {
                return false;
            }
        }
        template <std::size_t Index, class X>
        static bool read_impl_simd_ondemand(X &input, int index, std::variant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index == index) {
                    using F = std::variant_alternative_t<Index, std::variant<Ts...>>;
                    data.template emplace<Index>();
                    return JsonDecoder<F>::read_simd_ondemand(input, std::nullopt, std::get<Index>(data), mapping);
                } else {
                    return read_impl_simd_ondemand<Index+1,X>(input, index, data, mapping);
                }
            } else {
                return false;
            }
        }
        template <std::size_t Index>
        static void fillFieldNameMapping_internal(JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                using F = std::variant_alternative_t<Index, std::variant<Ts...>>;
                JsonDecoder<F>::fillFieldNameMapping(mapping);
                fillFieldNameMapping_internal<Index+1>(mapping);
            }
        }
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            fillFieldNameMapping_internal<0>(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::variant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            auto const &i = (key?input.at(*key):input);
            if (i.contains("index") && i.contains("content")) {
                std::size_t index;
                bool ret = JsonDecoder<std::size_t>::read(i, "index", index, mapping);
                if (!ret) {
                    return false;
                }
                return read_impl<0>(i.at("content"), index, data, mapping);
            } else {
                return false;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::variant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                if (input[*key].error() == simdjson::NO_SUCH_FIELD) {
                    return false;
                }
                simdjson::dom::element i = input[*key];
                if (i["index"].error() != simdjson::NO_SUCH_FIELD 
                    && i["content"].error() != simdjson::NO_SUCH_FIELD)
                {
                    std::size_t index;
                    bool ret = JsonDecoder<std::size_t>::read_simd(i, "index", index, mapping);
                    if (!ret) {
                        return false;
                    }
                    return read_impl_simd<0>(i["content"], index, data, mapping);
                } else {
                    return false;
                }
            } else {
                if (input["index"].error() != simdjson::NO_SUCH_FIELD 
                    && input["content"].error() != simdjson::NO_SUCH_FIELD)
                {
                    std::size_t index;
                    bool ret = JsonDecoder<std::size_t>::read_simd(input, "index", index, mapping);
                    if (!ret) {
                        return false;
                    }
                    return read_impl_simd<0>(input["content"], index, data, mapping);
                } else {
                    return false;
                }
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::variant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                auto x = input.get_object()[*key];
                if (x.error() == simdjson::NO_SUCH_FIELD) {
                    return false;
                }
                auto i = x.get_object();
                if (i["index"].error() != simdjson::NO_SUCH_FIELD 
                    && i["content"].error() != simdjson::NO_SUCH_FIELD)
                {
                    std::size_t index;
                    auto idx = i["index"].value();
                    bool ret = JsonDecoder<std::size_t>::read_simd_ondemand(idx, std::nullopt, index, mapping);
                    if (!ret) {
                        return false;
                    }
                    auto cont = i["content"].value();
                    return read_impl_simd_ondemand<0>(cont, index, data, mapping);
                } else {
                    return false;
                }
            } else {
                if (input["index"].error() != simdjson::NO_SUCH_FIELD 
                    && input["content"].error() != simdjson::NO_SUCH_FIELD)
                {
                    std::size_t index;
                    auto idx = input["index"].value();
                    bool ret = JsonDecoder<std::size_t>::read_simd_ondemand(idx, std::nullopt, index, mapping);
                    if (!ret) {
                        return false;
                    }
                    auto cont = input["content"].value();
                    return read_impl_simd_ondemand<0>(cont, index, data, mapping);
                } else {
                    return false;
                }
            }
        }
    };
    template <class... Ts>
    class JsonDecoder<JsonVariant<Ts...>, std::enable_if_t<JsonWrappable<JsonVariant<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static bool read_impl(nlohmann::json const &input, JsonVariant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index+1 == data.value.index()) {
                    using F = std::variant_alternative_t<Index+1, std::variant<std::monostate,Ts...>>;
                    return JsonDecoder<F>::read(input, std::nullopt, std::get<Index+1>(data.value), mapping);
                } else {
                    return read_impl<Index+1>(input, data, mapping);
                }
            } else {
                return false;
            }
        }
        template <std::size_t Index>
        static bool read_impl_simd(simdjson::dom::element const &input, JsonVariant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index+1 == data.value.index()) {
                    using F = std::variant_alternative_t<Index+1, std::variant<std::monostate,Ts...>>;
                    return JsonDecoder<F>::read_simd(input, std::nullopt, std::get<Index+1>(data.value), mapping);
                } else {
                    return read_impl_simd<Index+1>(input, data, mapping);
                }
            } else {
                return false;
            }
        }
        template <std::size_t Index, class X>
        static bool read_impl_simd_ondemand(X &input, JsonVariant<Ts...> &data, JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                if (Index+1 == data.value.index()) {
                    using F = std::variant_alternative_t<Index+1, std::variant<std::monostate,Ts...>>;
                    return JsonDecoder<F>::read_simd_ondemand(input, std::nullopt, std::get<Index+1>(data.value), mapping);
                } else {
                    return read_impl_simd_ondemand<Index+1,X>(input, data, mapping);
                }
            } else {
                return false;
            }
        }
        template <std::size_t Index>
        static void fillFieldNameMapping_internal(JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                using F = std::variant_alternative_t<Index+1, std::variant<std::monostate,Ts...>>;
                JsonDecoder<F>::fillFieldNameMapping(mapping);
                fillFieldNameMapping_internal<Index+1>(mapping);
            }
        }
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            fillFieldNameMapping_internal<0>(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, JsonVariant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (data.value.index() == 0) {
                return false;
            }
            auto const &i = (key?input.at(*key):input);
            return read_impl<0>(i, data, mapping);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, JsonVariant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (data.value.index() == 0) {
                return false;
            }
            if (key) {
                if (input[*key].error() == simdjson::NO_SUCH_FIELD) {
                    return false;
                }
                simdjson::dom::element i = input[*key];
                return read_impl_simd<0>(i, data, mapping);
            } else {
                return read_impl_simd<0>(input, data, mapping);
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, JsonVariant<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (data.value.index() == 0) {
                return false;
            }
            if (key) {
                auto x = input.get_object()[*key];
                if (x.error() == simdjson::NO_SUCH_FIELD) {
                    return false;
                }
                return read_impl_simd_ondemand<0>(x, data, mapping);
            } else {
                return read_impl_simd_ondemand<0>(input, data, mapping);
            }
        }
    };
    template <class... Ts>
    class JsonDecoder<std::tuple<Ts...>, std::enable_if_t<JsonWrappable<std::tuple<Ts...>>::value, void>> {
    private:
        template <std::size_t Index>
        static bool read_impl(nlohmann::json const &input, std::tuple<Ts...> &data, JsonFieldMapping const &mapping, bool retSoFar) {
            if constexpr (Index < sizeof...(Ts)) {
                using F = std::tuple_element_t<Index, std::tuple<Ts...>>;
                bool ret = JsonDecoder<F>::read(input[Index], std::nullopt, std::get<Index>(data), mapping);
                return read_impl<Index+1>(input, data, mapping, (retSoFar && ret));
            } else {
                return retSoFar;
            }
        }
        template <std::size_t Index>
        static bool read_impl_simd(simdjson::dom::element const &input, std::tuple<Ts...> &data, JsonFieldMapping const &mapping, bool retSoFar) {
            if constexpr (Index < sizeof...(Ts)) {
                using F = std::tuple_element_t<Index, std::tuple<Ts...>>;
                bool ret = JsonDecoder<F>::read_simd(input.get_array().at(Index), std::nullopt, std::get<Index>(data), mapping);
                return read_impl_simd<Index+1>(input, data, mapping, (retSoFar && ret));
            } else {
                return retSoFar;
            }
        }
        template <std::size_t Index>
        static bool read_impl_simd_ondemand(simdjson::ondemand::array_iterator iter, simdjson::ondemand::array_iterator endIter, std::tuple<Ts...> &data, JsonFieldMapping const &mapping, bool retSoFar) {
            if constexpr (Index < sizeof...(Ts)) {
                if (iter == endIter) {
                    return false;
                }
                using F = std::tuple_element_t<Index, std::tuple<Ts...>>;
                auto x = *iter;
                bool ret = JsonDecoder<F>::read_simd_ondemand(x, std::nullopt, std::get<Index>(data), mapping);
                ++iter;
                return read_impl_simd_ondemand<Index+1>(iter, endIter, data, mapping, (retSoFar && ret));
            } else {
                return retSoFar;
            }
        }
        template <std::size_t Index>
        static void fillFieldNameMapping_internal(JsonFieldMapping const &mapping) {
            if constexpr (Index < sizeof...(Ts)) {
                using F = std::tuple_element_t<Index, std::tuple<Ts...>>;
                JsonDecoder<F>::fillFieldNameMapping(mapping);
                fillFieldNameMapping_internal<Index+1>(mapping);
            }
        }
    public:
         static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            fillFieldNameMapping_internal<0>(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::tuple<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return read_impl<0>((key?input.at(*key):input), data, mapping, true);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::tuple<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                return read_impl_simd<0>(input[*key], data, mapping, true);
            } else {
                return read_impl_simd<0>(input, data, mapping, true);
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::tuple<Ts...> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                auto x = input.get_object()[*key].get_array().value();
                return read_impl_simd_ondemand<0>(x.begin(), x.end(), data, mapping, true);
            } else {
                auto x = input.get_array().value();
                return read_impl_simd_ondemand<0>(x.begin(), x.end(), data, mapping, true);
            }
        }
    };

    template <class T>
    class JsonDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo && (!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && !ConvertibleWithString<T>::value, void>> {
    private:
        static bool s_fieldNameMappingFilled;
        static std::array<std::string, StructFieldInfo<T>::FIELD_NAMES.size()> s_fieldNameMapping;
        static std::array<std::function<void(T &)>, StructFieldInfo<T>::FIELD_NAMES.size()> s_fieldPreprocessors;
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static bool read_impl(nlohmann::json const &input, T &data, JsonFieldMapping const &mapping, std::unordered_map<std::string, std::string> const *mappingForThisOne, bool retSoFar) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                if (s_fieldPreprocessors[FieldIndex]) {
                    (s_fieldPreprocessors[FieldIndex])(data);
                }
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if (s_fieldNameMappingFilled && mappingForThisOne == nullptr) {
                    std::string const &s = s_fieldNameMapping[FieldIndex];
                    if (input.find(s) == input.end()) {
                        return read_impl<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else {
                        bool ret = JsonDecoder<F>::read(input, s, StructFieldTypeInfo<T,FieldIndex>::access(data), mapping);
                        return read_impl<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, (retSoFar && ret));
                    }
                } else {
                    std::string s {StructFieldInfo<T>::FIELD_NAMES[FieldIndex]};
                    if (mappingForThisOne != nullptr) {
                        auto iter = mappingForThisOne->find(s);
                        if (iter != mappingForThisOne->end()) {
                            s = iter->second;
                        }
                    }
                    if (input.find(s) == input.end()) {
                        return read_impl<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else {
                        bool ret = JsonDecoder<F>::read(input, s, StructFieldTypeInfo<T,FieldIndex>::access(data), mapping);
                        return read_impl<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, (retSoFar && ret));
                    }
                }
            } else {
                return retSoFar;
            }
        }
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static bool read_impl_simd(simdjson::dom::element const &input, T &data, JsonFieldMapping const &mapping, std::unordered_map<std::string, std::string> const *mappingForThisOne, bool retSoFar) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                if (s_fieldPreprocessors[FieldIndex]) {
                    (s_fieldPreprocessors[FieldIndex])(data);
                }
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if (s_fieldNameMappingFilled && mappingForThisOne == nullptr) {
                    std::string const &s = s_fieldNameMapping[FieldIndex];
                    if (input[s].error() == simdjson::NO_SUCH_FIELD) {
                        return read_impl_simd<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else if (input[s].is_null()) {
                        return read_impl_simd<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else {
                        bool ret = JsonDecoder<F>::read_simd(input, s, StructFieldTypeInfo<T,FieldIndex>::access(data), mapping);
                        return read_impl_simd<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, (retSoFar && ret));
                    }
                } else {
                    std::string s {StructFieldInfo<T>::FIELD_NAMES[FieldIndex]};
                    if (mappingForThisOne != nullptr) {
                        auto iter = mappingForThisOne->find(s);
                        if (iter != mappingForThisOne->end()) {
                            s = iter->second;
                        }
                    }
                    if (input[s].error() == simdjson::NO_SUCH_FIELD) {
                        return read_impl_simd<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else if (input[s].is_null()) {
                        return read_impl_simd<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else {
                        bool ret = JsonDecoder<F>::read_simd(input, s, StructFieldTypeInfo<T,FieldIndex>::access(data), mapping);
                        return read_impl_simd<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, (retSoFar && ret));
                    }
                }
            } else {
                return retSoFar;
            }
        }
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static bool read_impl_simd_ondemand(simdjson::ondemand::object &input, T &data, JsonFieldMapping const &mapping, std::unordered_map<std::string, std::string> const *mappingForThisOne, bool retSoFar) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                if (s_fieldPreprocessors[FieldIndex]) {
                    (s_fieldPreprocessors[FieldIndex])(data);
                }
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if (s_fieldNameMappingFilled && mappingForThisOne == nullptr) {
                    std::string const &s = s_fieldNameMapping[FieldIndex];
                    auto x = input[s];
                    if (x.error() == simdjson::NO_SUCH_FIELD) {
                        return read_impl_simd_ondemand<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else if (x.is_null()) {
                        return read_impl_simd_ondemand<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else {
                        bool ret = JsonDecoder<F>::read_simd_ondemand(x, std::nullopt, StructFieldTypeInfo<T,FieldIndex>::access(data), mapping);
                        return read_impl_simd_ondemand<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, (retSoFar && ret));
                    }
                } else {
                    std::string s {StructFieldInfo<T>::FIELD_NAMES[FieldIndex]};
                    if (mappingForThisOne != nullptr) {
                        auto iter = mappingForThisOne->find(s);
                        if (iter != mappingForThisOne->end()) {
                            s = iter->second;
                        }
                    }
                    auto x = input[s];
                    if (x.error() == simdjson::NO_SUCH_FIELD) {
                        return read_impl_simd_ondemand<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else if (x.is_null()) {
                        return read_impl_simd_ondemand<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, retSoFar);
                    } else {
                        bool ret = JsonDecoder<F>::read_simd_ondemand(x, std::nullopt, StructFieldTypeInfo<T,FieldIndex>::access(data), mapping);
                        return read_impl_simd_ondemand<FieldCount,FieldIndex+1>(input, data, mapping, mappingForThisOne, (retSoFar && ret));
                    }
                }
            } else {
                return retSoFar;
            }
        }
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void fillFieldNameMapping_internal(
            std::unordered_map<std::string, std::string> const *mappingForThisOne
            , JsonFieldMapping const &mapping
        ) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                std::string s {StructFieldInfo<T>::FIELD_NAMES[FieldIndex]};
                if (mappingForThisOne != nullptr) {
                    auto iter = mappingForThisOne->find(s);
                    if (iter != mappingForThisOne->end()) {
                        s = iter->second;
                    }
                }
                s_fieldNameMapping[FieldIndex] = s;
                JsonDecoder<F>::fillFieldNameMapping(mapping);
                fillFieldNameMapping_internal<FieldCount,FieldIndex+1>(mappingForThisOne, mapping);
            } else {
                s_fieldNameMappingFilled = true;
            }
        }
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            std::unordered_map<std::string, std::string> const *mappingForThisOne = nullptr;
            if (!mapping.empty()) {
                auto iter = mapping.find(std::type_index(typeid(T)));
                if (iter != mapping.end()) {
                    mappingForThisOne = &(iter->second);
                }
            }
            fillFieldNameMapping_internal<StructFieldInfo<T>::FIELD_NAMES.size(),0>(mappingForThisOne, mapping);
        }
        static void setFieldPreprocessor(std::string_view const &fieldName, std::function<void(T &)> preprocessor) {
            auto idx = StructFieldInfo<T>::getFieldIndex(fieldName);
            if (idx >= 0 && idx < StructFieldInfo<T>::FIELD_NAMES.size()) {
                s_fieldPreprocessors[idx] = preprocessor;
            }
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data);
            std::unordered_map<std::string, std::string> const *mappingForThisOne = nullptr;
            if (!mapping.empty()) {
                auto iter = mapping.find(std::type_index(typeid(T)));
                if (iter != mapping.end()) {
                    mappingForThisOne = &(iter->second);
                }
            }
            return read_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>((key?input.at(*key):input), data, mapping, mappingForThisOne, true);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data);
            std::unordered_map<std::string, std::string> const *mappingForThisOne = nullptr;
            if (!mapping.empty()) {
                auto iter = mapping.find(std::type_index(typeid(T)));
                if (iter != mapping.end()) {
                    mappingForThisOne = &(iter->second);
                }
            }
            if (key) {
                return read_impl_simd<StructFieldInfo<T>::FIELD_NAMES.size(),0>(input[*key], data, mapping, mappingForThisOne, true);
            } else {
                return read_impl_simd<StructFieldInfo<T>::FIELD_NAMES.size(),0>(input, data, mapping, mappingForThisOne, true);
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(data);
            std::unordered_map<std::string, std::string> const *mappingForThisOne = nullptr;
            if (!mapping.empty()) {
                auto iter = mapping.find(std::type_index(typeid(T)));
                if (iter != mapping.end()) {
                    mappingForThisOne = &(iter->second);
                }
            }
            if constexpr (std::is_same_v<std::decay_t<X>, simdjson::ondemand::object>) {
                if (key) {
                    auto x = input[*key].get_object().value();
                    return read_impl_simd_ondemand<StructFieldInfo<T>::FIELD_NAMES.size(),0>(x, data, mapping, mappingForThisOne, true);
                } else {
                    return read_impl_simd_ondemand<StructFieldInfo<T>::FIELD_NAMES.size(),0>(input, data, mapping, mappingForThisOne, true);
                }
            } else {
                if (key) {
                    auto x = input.get_object()[*key].get_object().value();
                    return read_impl_simd_ondemand<StructFieldInfo<T>::FIELD_NAMES.size(),0>(x, data, mapping, mappingForThisOne, true);
                } else {
                    auto x = input.get_object().value();
                    return read_impl_simd_ondemand<StructFieldInfo<T>::FIELD_NAMES.size(),0>(x, data, mapping, mappingForThisOne, true);
                }
            }
        }
    };
    template <class T>
    bool JsonDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo && (!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && !ConvertibleWithString<T>::value, void>>::s_fieldNameMappingFilled = false;
    template <class T>
    std::array<std::string, StructFieldInfo<T>::FIELD_NAMES.size()> JsonDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo && (!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && !ConvertibleWithString<T>::value, void>>::s_fieldNameMapping;
    template <class T>
    std::array<std::function<void(T &)>, StructFieldInfo<T>::FIELD_NAMES.size()> JsonDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo && (!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && (!EncodableThroughProxy<T>::value || !JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value) && !ConvertibleWithString<T>::value, void>>::s_fieldPreprocessors;

    template <class T>
    class JsonDecoder<T, std::enable_if_t<bytedata_utils::ProtobufStyleSerializableChecker<T>::IsProtobufStyleSerializable(), void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            ByteData b;
            bool ret = JsonDecoder<ByteData>::read(input, key, b, mapping);
            if (ret) {
                return data.ParseFromString(b.content);
            } else {
                return false;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            ByteData b;
            bool ret = JsonDecoder<ByteData>::read_simd(input, key, b, mapping);
            if (ret) {
                return data.ParseFromString(b.content);
            } else {
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            ByteData b;
            bool ret = JsonDecoder<ByteData>::read_simd_ondemand(input, key, b, mapping);
            if (ret) {
                return data.ParseFromString(b.content);
            } else {
                return false;
            }
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
            return JsonDecoder<T>::read(x, std::nullopt, t_, mapping);
        }
        bool fromString(std::string const &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return fromStringView(std::string_view(s), mapping);
        }
        bool fromStream(std::istream &s, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            nlohmann::json x;
            s >> x;
            return JsonDecoder<T>::read(x, std::nullopt, t_, mapping);
        }
        bool fromNlohmannJson(nlohmann::json const &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read(x, std::nullopt, t_, mapping);
        }
        bool fromSimdJson(simdjson::dom::element const &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::read_simd(x, std::nullopt, t_, mapping);
        }
        template <class X>
        bool fromSimdJsonOnDemand(X &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return JsonDecoder<T>::template read_simd_ondemand<X>(x, std::nullopt, t_, mapping);
        }
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
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
            return JsonDecoder<T>::read(x, std::nullopt, t, mapping);
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
                return JsonDecoder<T>::read(x, std::nullopt, *t_, mapping);
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
                return JsonDecoder<T>::read(x, std::nullopt, *t_, mapping);
            } else {
                return false;
            }
        }
        bool fromNlohmannJson(nlohmann::json const &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (t_) {
                return JsonDecoder<T>::read(x, std::nullopt, *t_, mapping);
            } else {
                return false;
            }
        }
        bool fromSimdJson(simdjson::dom::element const &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (t_) {
                return JsonDecoder<T>::read_simd(x, std::nullopt, *t_, mapping);
            } else {
                return false;
            }
        }
        template <class X>
        bool fromSimdJsonOnDemand(X &x, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (t_) {
                return JsonDecoder<T>::template read_simd_ondemand<X>(x, std::nullopt, *t_, mapping);
            } else {
                return false;
            }
        }
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
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
    template <class T>
    class Json<T const *, std::enable_if_t<JsonWrappable<T>::value, void>> {
    private:
        T const *t_;
    public:
        Json() : t_(nullptr) {}
        Json(T const *t) : t_(t) {}
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
                
        T const &value() const {
            return *t_;
        }
        T const &operator*() const {
            return *t_;
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
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {}
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, nlohmann::json &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            data = (key?input[*key]:input);
            return true;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, nlohmann::json &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            throw std::runtime_error("Cannot assign simdjson to nlohmann::json");
            return false;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, nlohmann::json &data, JsonFieldMapping const &/*mapping*/=JsonFieldMapping {}) {
            throw std::runtime_error("Cannot assign simdjson to nlohmann::json");
            return false;
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
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, Json<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            return data.fromNlohmannJson((key?input[*key]:input), mapping);
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, Json<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                return data.fromSimdJson(input[*key], mapping);
            } else {
                return data.fromSimdJson(input, mapping);
            }
        }
        template <class X>
        static bool read_simd_ondemand(X const &input, std::optional<std::string> const &key, Json<T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (key) {
                return data.fromSimdJsonOnDemand(input.get_object()[*key], mapping);
            } else {
                return data.fromSimdJsonOnDemand(input, mapping);
            }
        }
    };

    template <class T>
    class JsonEncoder<T, std::enable_if_t<(!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && EncodableThroughProxy<T>::value && JsonWrappable<typename EncodableThroughProxy<T>::EncodeProxyType>::value, void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, T const &data) {
            JsonEncoder<typename EncodableThroughProxy<T>::EncodeProxyType>::write(output, key, EncodableThroughProxy<T>::toProxy(data));
        }
    };
    template <class T>
    struct JsonWrappable<T, std::enable_if_t<(!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && EncodableThroughProxy<T>::value && JsonWrappable<typename EncodableThroughProxy<T>::EncodeProxyType>::value && JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value, void>> {
        static constexpr bool value = true;
    };
    template <class T>
    class JsonDecoder<T, std::enable_if_t<(!EncodableThroughMultipleProxies<T>::value || !JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value) && EncodableThroughProxy<T>::value && JsonWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value, void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<typename EncodableThroughProxy<T>::DecodeProxyType>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            typename EncodableThroughProxy<T>::DecodeProxyType p;
            if (!JsonDecoder<typename EncodableThroughProxy<T>::DecodeProxyType>::read(input, key, p, mapping)) {
                if constexpr (dev::cd606::tm::basic::ConvertibleWithString<T>::value) {
                    auto const &i = (key?input.at(*key):input);
                    if (i.is_null()) {
                        data = ConvertibleWithString<T>::fromString("");
                        return true;
                    } else if (i.is_string()) {
                        std::string s;
                        i.get_to(s);
                        data = ConvertibleWithString<T>::fromString(s);
                        return true;
                    } else {
                        std::string s = i.dump();
                        data = ConvertibleWithString<T>::fromString(s);
                        return true;
                    }
                } else {
                    return false;
                }
            }
            data = EncodableThroughProxy<T>::fromProxy(std::move(p));
            return true;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            typename EncodableThroughProxy<T>::DecodeProxyType p;
            if (!JsonDecoder<typename EncodableThroughProxy<T>::DecodeProxyType>::read_simd(input, key, p, mapping)) {
                if constexpr (dev::cd606::tm::basic::ConvertibleWithString<T>::value) {
                    std::string s;
                    auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
                    if (ret) {
                        data = ConvertibleWithString<T>::fromString(s);
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            data = EncodableThroughProxy<T>::fromProxy(std::move(p));
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            typename EncodableThroughProxy<T>::DecodeProxyType p;
            if (!JsonDecoder<typename EncodableThroughProxy<T>::DecodeProxyType>::template read_simd_ondemand<X>(input, key, p, mapping)) {
                if constexpr (dev::cd606::tm::basic::ConvertibleWithString<T>::value) {
                    std::string s;
                    auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
                    if (ret) {
                        data = ConvertibleWithString<T>::fromString(s);
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            data = EncodableThroughProxy<T>::fromProxy(std::move(p));
            return true;
        }
    };

    template <class T>
    class JsonEncoder<T, std::enable_if_t<EncodableThroughMultipleProxies<T>::value && JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value, void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, T const &data) {
            JsonEncoder<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::write(output, key, EncodableThroughMultipleProxies<T>::toJSONEncodeProxy(data));
        }
    };
    template <class T>
    struct JsonWrappable<T, std::enable_if_t<EncodableThroughMultipleProxies<T>::value && JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value, void>> {
        static constexpr bool value = true;
    };
    template <class T>
    class JsonDecoder<T, std::enable_if_t<EncodableThroughMultipleProxies<T>::value && JsonWrappable<typename EncodableThroughMultipleProxies<T>::JSONEncodeProxyType>::value, void>> {
    private:
        using P = typename EncodableThroughMultipleProxies<T>::DecodeProxyTypes;
        template <std::size_t Idx>
        static void fillFieldNameMappingHelper(JsonFieldMapping const &mapping) {
            if constexpr (Idx>=0 && Idx<std::variant_size_v<P>) {
                if constexpr (JsonWrappable<std::variant_alternative_t<Idx, P>>::value) {
                    JsonDecoder<std::variant_alternative_t<Idx, P>>::fillFieldNameMapping(mapping);
                }
                fillFieldNameMappingHelper<Idx+1>(mapping);
            }
        }
        template <std::size_t Idx>
        static bool readHelper(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping) {
            if constexpr (Idx>=0 && Idx<std::variant_size_v<P>) {
                if constexpr (JsonWrappable<std::variant_alternative_t<Idx, P>>::value) {
                    try {
                        std::variant_alternative_t<Idx, P> x;
                        if (JsonDecoder<std::variant_alternative_t<Idx, P>>::read(input, key, x, mapping)) {
                            data = EncodableThroughMultipleProxies<T>::fromProxy(P {std::move(x)});
                            return true;
                        }
                    } catch (nlohmann::json::exception const &) {                        
                    }
                }
                return readHelper<Idx+1>(input, key, data, mapping);
            } else {
                return false;
            }
        }
        template <std::size_t Idx>
        static bool readSimdHelper(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping) {
            if constexpr (Idx>=0 && Idx<std::variant_size_v<P>) {
                if constexpr (JsonWrappable<std::variant_alternative_t<Idx, P>>::value) {
                    try {
                        std::variant_alternative_t<Idx, P> x;
                        if (JsonDecoder<std::variant_alternative_t<Idx, P>>::read_simd(input, key, x, mapping)) {
                            data = EncodableThroughMultipleProxies<T>::fromProxy(P {std::move(x)});
                            return true;
                        }
                    } catch (simdjson::simdjson_error const &) {                        
                    }
                }
                return readSimdHelper<Idx+1>(input, key, data, mapping);
            } else {
                return false;
            }
        }
        template <class X, std::size_t Idx>
        static bool readSimdOnDemandHelper(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping) {
            if constexpr (Idx>=0 && Idx<std::variant_size_v<P>) {
                if constexpr (JsonWrappable<std::variant_alternative_t<Idx, P>>::value) {
                    try {
                        std::variant_alternative_t<Idx, P> x;
                        if (JsonDecoder<std::variant_alternative_t<Idx, P>>::read_simd_ondemand(input, key, x, mapping)) {
                            data = EncodableThroughMultipleProxies<T>::fromProxy(P {std::move(x)});
                            return true;
                        }
                    } catch (simdjson::simdjson_error const &) {                        
                    }
                }
                return readSimdOnDemandHelper<X, Idx+1>(input, key, data, mapping);
            } else {
                return false;
            }
        }
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            fillFieldNameMappingHelper<0>(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (readHelper<0>(input, key, data, mapping)) {
                return true;
            }
            if constexpr (dev::cd606::tm::basic::ConvertibleWithString<T>::value) {
                auto const &i = (key?input.at(*key):input);
                if (i.is_null()) {
                    data = ConvertibleWithString<T>::fromString("");
                    return true;
                } else if (i.is_string()) {
                    std::string s;
                    i.get_to(s);
                    data = ConvertibleWithString<T>::fromString(s);
                    return true;
                } else {
                    std::string s = i.dump();
                    data = ConvertibleWithString<T>::fromString(s);
                    return true;
                }
            } else {
                return false;
            }
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (readSimdHelper<0>(input, key, data, mapping)) {
                return true;
            }
            if constexpr (dev::cd606::tm::basic::ConvertibleWithString<T>::value) {
                std::string s;
                auto ret = JsonDecoder<std::string>::read_simd(input, key, s, mapping);
                if (ret) {
                    data = ConvertibleWithString<T>::fromString(s);
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, T &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            if (readSimdOnDemandHelper<X, 0>(input, key, data, mapping)) {
                return true;
            }
            if constexpr (dev::cd606::tm::basic::ConvertibleWithString<T>::value) {
                std::string s;
                auto ret = JsonDecoder<std::string>::read_simd_ondemand(input, key, s, mapping);
                if (ret) {
                    data = ConvertibleWithString<T>::fromString(s);
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    };

    template <class T>
    class JsonEncoder<std::shared_ptr<const T>, std::enable_if_t<JsonWrappable<T>::value, void>> {
    public:
        static void write(nlohmann::json &output, std::optional<std::string> const &key, std::shared_ptr<const T> const &data) {
            JsonEncoder<T>::write(output, key, *data);
        }
    };
    template <class T>
    struct JsonWrappable<std::shared_ptr<const T>, std::enable_if_t<JsonWrappable<T>::value, void>> {
        static constexpr bool value = true;
    };
    template <class T>
    class JsonDecoder<std::shared_ptr<const T>, std::enable_if_t<JsonWrappable<T>::value, void>> {
    public:
        static void fillFieldNameMapping(JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            JsonDecoder<T>::fillFieldNameMapping(mapping);
        }
        static bool read(nlohmann::json const &input, std::optional<std::string> const &key, std::shared_ptr<const T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.reset();
            auto x = std::make_shared<T>();
            if (!JsonDecoder<T>::read(input, key, *x, mapping)) {
                return false;
            }
            data = std::const_pointer_cast<const T>(x);
            return true;
        }
        static bool read_simd(simdjson::dom::element const &input, std::optional<std::string> const &key, std::shared_ptr<const T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.reset();
            auto x = std::make_shared<T>();
            if (!JsonDecoder<T>::read_simd(input, key, *x, mapping)) {
                return false;
            }
            data = std::const_pointer_cast<const T>(x);
            return true;
        }
        template <class X>
        static bool read_simd_ondemand(X &input, std::optional<std::string> const &key, std::shared_ptr<const T> &data, JsonFieldMapping const &mapping=JsonFieldMapping {}) {
            data.reset();
            auto x = std::make_shared<T>();
            if (!JsonDecoder<T>::template read_simd_ondemand<X>(input, key, *x, mapping)) {
                return false;
            }
            data = std::const_pointer_cast<const T>(x);
            return true;
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
