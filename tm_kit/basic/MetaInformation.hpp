#ifndef TM_KIT_BASIC_MULTI_APP_META_INFORMATION_HPP_
#define TM_KIT_BASIC_MULTI_APP_META_INFORMATION_HPP_

#include <tm_kit/basic/SerializationHelperMacros.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/NlohmannJsonInterop.hpp>
#include <tm_kit/basic/FixedPrecisionShortDecimal.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    namespace meta_information_helper {
        using BuiltIn = TM_BASIC_CONST_STRING_TYPE("C++ built-in");
        using TM = TM_BASIC_CONST_STRING_TYPE("TM types");
        using Enum = TM_BASIC_CONST_STRING_TYPE("Enum as string");
        using Tuple = TM_BASIC_CONST_STRING_TYPE("Tuple");
        using Variant = TM_BASIC_CONST_STRING_TYPE("Variant");
        using Optional = TM_BASIC_CONST_STRING_TYPE("Optional");
        using Collection = TM_BASIC_CONST_STRING_TYPE("Collection");
        using Dictionary = TM_BASIC_CONST_STRING_TYPE("Dictionary");
        using Structure = TM_BASIC_CONST_STRING_TYPE("Structure");
    }

    struct MetaInformation_BuiltIn;
    struct MetaInformation_TM;
    struct MetaInformation_Enum;
    struct MetaInformation_Tuple;
    struct MetaInformation_Variant;
    struct MetaInformation_Optional;
    struct MetaInformation_Collection; 
    struct MetaInformation_Dictionary;
    struct MetaInformation_Structure;

    using MetaInformation = std::variant<
        dev::cd606::tm::basic::MetaInformation_BuiltIn
        , dev::cd606::tm::basic::MetaInformation_TM
        , dev::cd606::tm::basic::MetaInformation_Enum
        , dev::cd606::tm::basic::MetaInformation_Tuple 
        , dev::cd606::tm::basic::MetaInformation_Variant 
        , dev::cd606::tm::basic::MetaInformation_Optional 
        , dev::cd606::tm::basic::MetaInformation_Collection 
        , dev::cd606::tm::basic::MetaInformation_Dictionary 
        , dev::cd606::tm::basic::MetaInformation_Structure
    >;
    using MetaInformationPtr = std::shared_ptr<MetaInformation const>;

    #define META_INFORMATION_BUILT_IN_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::BuiltIn, DataKind)) \
        ((std::string, DataType))
    #define META_INFORMATION_TM_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::TM, DataKind)) \
        ((std::string, DataType))
    #define META_INFORMATION_ONE_ENUM_VALUE_FIELDS \
        ((std::string, Name)) \
        ((std::string, CppValueName)) \
        ((int64_t, Value)) 
    #define META_INFORMATION_ENUM_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Enum, DataKind)) \
        ((std::string, TypeID)) \
        ((std::string, TypeReferenceName)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformation_OneEnumValue>, EnumInfo))
    #define META_INFORMATION_TUPLE_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Tuple, DataKind)) \
        ((std::string, TypeID)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformationPtr>, TupleContent))
    #define META_INFORMATION_VARIANT_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Variant, DataKind)) \
        ((std::string, TypeID)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformationPtr>, VariantChoices))
    #define META_INFORMATION_OPTIONAL_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Optional, DataKind)) \
        ((std::string, TypeID)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, UnderlyingType))
    #define META_INFORMATION_COLLECTION_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Collection, DataKind)) \
        ((std::string, TypeID)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, UnderlyingType))
    #define META_INFORMATION_DICTIONARY_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Dictionary, DataKind)) \
        ((std::string, TypeID)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, KeyType)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, ValueType))
    #define META_INFORMATION_ONE_STRUCT_FIELD_FIELDS \
        ((std::string, FieldName)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, FieldType))
    #define META_INFORMATION_STRUCTURE_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Structure, DataKind)) \
        ((std::string, TypeID)) \
        ((std::string, TypeReferenceName)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformation_OneStructField>, Fields)) \
        ((bool, EncDecWithFieldNames))
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_BuiltIn, META_INFORMATION_BUILT_IN_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_TM, META_INFORMATION_TM_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_OneEnumValue, META_INFORMATION_ONE_ENUM_VALUE_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Enum, META_INFORMATION_ENUM_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Tuple, META_INFORMATION_TUPLE_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Variant, META_INFORMATION_VARIANT_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Optional, META_INFORMATION_OPTIONAL_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Collection, META_INFORMATION_COLLECTION_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Dictionary, META_INFORMATION_DICTIONARY_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_OneStructField, META_INFORMATION_ONE_STRUCT_FIELD_FIELDS);
    TM_BASIC_CBOR_CAPABLE_STRUCT(MetaInformation_Structure, META_INFORMATION_STRUCTURE_FIELDS);    
} } } }

TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_BuiltIn, META_INFORMATION_BUILT_IN_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_TM, META_INFORMATION_TM_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_OneEnumValue, META_INFORMATION_ONE_ENUM_VALUE_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Enum, META_INFORMATION_ENUM_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Tuple, META_INFORMATION_TUPLE_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Variant, META_INFORMATION_VARIANT_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Optional, META_INFORMATION_OPTIONAL_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Collection, META_INFORMATION_COLLECTION_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Dictionary, META_INFORMATION_DICTIONARY_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_OneStructField, META_INFORMATION_ONE_STRUCT_FIELD_FIELDS);
TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::MetaInformation_Structure, META_INFORMATION_STRUCTURE_FIELDS);

#undef META_INFORMATION_BUILT_IN_FIELDS
#undef META_INFORMATION_TM_FIELDS
#undef META_INFORMATION_ONE_ENUM_VALUE_FIELDS
#undef META_INFORMATION_ENUM_FIELDS
#undef META_INFORMATION_TUPLE_FIELDS
#undef META_INFORMATION_VARIANT_FIELDS
#undef META_INFORMATION_OPTIONAL_FIELDS
#undef META_INFORMATION_COLLECTION_FIELDS
#undef META_INFORMATION_DICTIONARY_FIELDS
#undef META_INFORMATION_ONE_STRUCT_FIELD_FIELDS
#undef META_INFORMATION_STRUCTURE_FIELDS

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T, typename Enable=void>
    class MetaInformationGenerator;

    #define TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_LIST (uint8_t) (uint16_t) (uint32_t) (uint64_t) (int8_t) (int16_t) (int32_t) (int64_t) (bool) (char) (float) (double) (std::string) (std::string_view) (std::chrono::system_clock::time_point) (std::tm) (std::monostate)
    #define TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_M(r, data, elem) \
        template <> \
        class MetaInformationGenerator<elem, void> { \
        public: \
            static MetaInformation generate() { \
                return MetaInformation_BuiltIn { \
                    {} \
                    , BOOST_PP_STRINGIZE(elem) \
                }; \
            } \
        };
    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_M, _, TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_LIST);
    #undef TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_LIST
    #undef TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_M

    template <class T>
    class MetaInformationGenerator<
        T
        , std::enable_if_t<
            std::is_same_v<T, unsigned long long> && std::is_same_v<unsigned long long, uint64_t>
            , void
        >
    > {
    public:
        static MetaInformation generate() { 
            return MetaInformation_BuiltIn { 
                {} 
                , "unsigned long long"
            }; 
        }
    };
    template <class T>
    class MetaInformationGenerator<
        T
        , std::enable_if_t<
            std::is_same_v<T, long long> && !std::is_same_v<long long, int64_t>
            , void
        >
    > {
    public:
        static MetaInformation generate() { 
            return MetaInformation_BuiltIn { 
                {} 
                , "long long"
            }; 
        }
    };
    template <class T>
    class MetaInformationGenerator<
        T
        , std::enable_if_t<
            (std::is_enum_v<T> && !bytedata_utils::IsEnumWithStringRepresentation<T>::value)
            , void
        >
    > {
    public:
        static MetaInformation generate() { 
            return MetaInformation_BuiltIn { 
                {} 
                , "enum"
            }; 
        }
    };
    template <std::size_t N>
    class MetaInformationGenerator<
        std::array<char, N>
        , void
    > {
    public:
        static MetaInformation generate() { 
            return MetaInformation_BuiltIn { 
                {} 
                , "std::string"
            }; 
        }
    };

    #define TM_BASIC_META_INFORMATION_HELPER_TM_LIST (basic::DateHolder)
    #define TM_BASIC_META_INFORMATION_HELPER_TM_M(r, data, elem) \
        template <> \
        class MetaInformationGenerator<elem, void> { \
        public: \
            static MetaInformation generate() { \
                return MetaInformation_TM { \
                    {} \
                    , BOOST_PP_STRINGIZE(elem) \
                }; \
            } \
        };
    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_META_INFORMATION_HELPER_TM_M, _, TM_BASIC_META_INFORMATION_HELPER_TM_LIST);
    #undef TM_BASIC_META_INFORMATION_HELPER_TM_LIST
    #undef TM_BASIC_META_INFORMATION_HELPER_TM_M

    template <std::size_t Precision, typename Underlying>
    class MetaInformationGenerator<FixedPrecisionShortDecimal<Precision, Underlying>, void> {
    public:
        static MetaInformation generate() {
            return MetaInformation_TM {
                {}
                , "basic::FixedPrecisionShortDecimal"
            };
        }
    };

    template <class T>
    class MetaInformationGenerator<
        T
        , std::enable_if_t<
            bytedata_utils::IsEnumWithStringRepresentation<T>::value
            , void
        >
    > {
    public:
        static MetaInformation generate() { 
            std::vector<MetaInformation_OneEnumValue> info;
            std::size_t ii = 0;
            for (auto const &x : bytedata_utils::IsEnumWithStringRepresentation<T>::namesAndValues) {
                info.push_back({
                    std::string {x.first}
                    , std::string {bytedata_utils::IsEnumWithStringRepresentation<T>::cppValueNames[ii]}
                    , static_cast<int64_t>(x.second)
                });
                ++ii;
            }
            return MetaInformation_Enum { 
                {} 
                , std::string {typeid(T).name()}
                , std::string {bytedata_utils::IsEnumWithStringRepresentation<T>::REFERENCE_NAME}
                , std::move(info)
            }; 
        }
    };

    template <>
    class MetaInformationGenerator<
        std::tuple<>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Tuple {
                {}
                , typeid(std::tuple<>).name()
                , {}
            };
        }
    };
    template <typename First, typename... Rest>
    class MetaInformationGenerator<
        std::tuple<First, Rest...>
        , void
    > {
    private:
        template <typename ThisOne, typename... Others>
        static void addToInfo(std::vector<MetaInformationPtr> &output) {
            output.push_back(std::const_pointer_cast<MetaInformation const>(
                std::make_shared<MetaInformation>(
                    MetaInformationGenerator<ThisOne>::generate()
                )
            ));
            if constexpr (sizeof...(Others) > 0) {
                addToInfo<Others...>(output);
            }
        }
    public:
        static MetaInformation generate() {
            std::vector<MetaInformationPtr> cont;
            addToInfo<First, Rest...>(cont);
            return MetaInformation_Tuple {
                {}
                , typeid(std::tuple<First, Rest...>).name()
                , std::move(cont)
            };
        }
    };
    template <typename First, typename... Rest>
    class MetaInformationGenerator<
        std::variant<First, Rest...>
        , void
    > {
    private:
        template <typename ThisOne, typename... Others>
        static void addToInfo(std::vector<MetaInformationPtr> &output) {
            output.push_back(std::const_pointer_cast<MetaInformation const>(
                std::make_shared<MetaInformation>(
                    MetaInformationGenerator<ThisOne>::generate()
                )
            ));
            if constexpr (sizeof...(Others) > 0) {
                addToInfo<Others...>(output);
            }
        }
    public:
        static MetaInformation generate() {
            std::vector<MetaInformationPtr> cont;
            addToInfo<First, Rest...>(cont);
            return MetaInformation_Variant {
                {}
                , typeid(std::variant<First, Rest...>).name()
                , std::move(cont)
            };
        }
    };
    template <class T>
    class MetaInformationGenerator<
        std::optional<T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Optional {
                {}
                , typeid(std::optional<T>).name()
                , MetaInformationGenerator<T>::generate()
            };
        }
    };
    template <class T>
    class MetaInformationGenerator<
        std::vector<T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::vector<T>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class T>
    class MetaInformationGenerator<
        std::list<T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::list<T>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class T, std::size_t N>
    class MetaInformationGenerator<
        std::array<T, N>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::array<T, N>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class T>
    class MetaInformationGenerator<
        std::valarray<T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::valarray<T>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class T>
    class MetaInformationGenerator<
        std::deque<T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::deque<T>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class T, class Cmp>
    class MetaInformationGenerator<
        std::set<T, Cmp>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::set<T, Cmp>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class T, class Hash>
    class MetaInformationGenerator<
        std::unordered_set<T, Hash>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Collection {
                {}
                , typeid(std::unordered_set<T, Hash>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<T>::generate()
                            )
                        )
            };
        }
    };
    template <class K, class V, class Cmp>
    class MetaInformationGenerator<
        std::map<K, V, Cmp>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Dictionary {
                {}
                , typeid(std::map<K, V, Cmp>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<K>::generate()
                            )
                        )
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<V>::generate()
                            )
                        )
            };
        }
    };
    template <class K, class V, class Hash>
    class MetaInformationGenerator<
        std::unordered_map<K, V, Hash>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_Dictionary {
                {}
                , typeid(std::unordered_map<K, V, Hash>).name()
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<K>::generate()
                            )
                        )
                , std::const_pointer_cast<MetaInformation const>(
                        std::make_shared<MetaInformation>(
                            MetaInformationGenerator<V>::generate()
                            )
                        )
            };
        }
    };
    //currently, EncodableThroughProxy is not supported yet because there may be multiple proxies
    template <class T>
    class MetaInformationGenerator<
        T
        , std::enable_if_t<
            !std::is_same_v<T, char> && ConvertibleWithString<T>::value && !bytedata_utils::IsEnumWithStringRepresentation<T>::value && !IsFixedPrecisionShortDecimal<T>::value
            , void
        >
    > {
    public:
        static MetaInformation generate() {
            return MetaInformation_BuiltIn {
                {}
                , "std::string"
            };
        }
    };
    template <class T>
    class MetaInformationGenerator<
        T
        , std::enable_if_t<
            ((!ConvertibleWithString<T>::value) && StructFieldInfo<T>::HasGeneratedStructFieldInfo)
            , void
        >
    > {
    private:
        template <std::size_t I, std::size_t N>
        static void fillFieldInfo(std::vector<MetaInformation_OneStructField> &output) {
            output.push_back(MetaInformation_OneStructField {
                std::string {StructFieldInfo<T>::FIELD_NAMES[I]}
                , std::const_pointer_cast<MetaInformation const>(
                    std::make_shared<MetaInformation>(
                        MetaInformationGenerator<typename StructFieldTypeInfo<T, I>::TheType>::generate()
                    )
                )
            });
            if constexpr (I+1<N) {
                fillFieldInfo<I+1,N>(output);
            }
        }
    public:
        static MetaInformation generate() {
            std::vector<MetaInformation_OneStructField> cont;
            fillFieldInfo<0, StructFieldInfo<T>::FIELD_NAMES.size()>(cont);
            return MetaInformation_Structure {
                {}
                , std::string {typeid(T).name()}
                , std::string {StructFieldInfo<T>::REFERENCE_NAME}
                , std::move(cont)
                , StructFieldInfo<T>::EncDecWithFieldNames
            };
        }
    };

    //wrappers
    template <class T>
    class MetaInformationGenerator<
        std::shared_ptr<T const>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformationGenerator<T>::generate();
        }
    };
    template <class T>
    class MetaInformationGenerator<
        SingleLayerWrapper<T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformationGenerator<T>::generate();
        }
    };
    template <int32_t N, class T>
    class MetaInformationGenerator<
        SingleLayerWrapperWithID<N, T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformationGenerator<T>::generate();
        }
    };
    template <class Mark, class T>
    class MetaInformationGenerator<
        SingleLayerWrapperWithTypeMark<Mark, T>
        , void
    > {
    public:
        static MetaInformation generate() {
            return MetaInformationGenerator<T>::generate();
        }
    };

    namespace nlohmann_json_interop {
        //force to be json wrappable so as to break the recursive dependency problem
        template <>
        struct JsonWrappable<MetaInformation, void> {
            static constexpr bool value = true;
        };
    }

} } } }

#endif
