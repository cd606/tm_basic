#ifndef TM_KIT_BASIC_MULTI_APP_META_INFORMATION_HPP_
#define TM_KIT_BASIC_MULTI_APP_META_INFORMATION_HPP_

#include <tm_kit/basic/SerializationHelperMacros.hpp>
#include <tm_kit/basic/ByteData.hpp>

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
        ((int64_t, Value)) 
    #define META_INFORMATION_ENUM_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Enum, DataKind)) \
        ((std::string, TypeID)) \
        ((std::string, TypeReferenceName)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformation_OneEnumValue>, EnumInfo))
    #define META_INFORMATION_TUPLE_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Tuple, DataKind)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformationPtr>, TupleContent))
    #define META_INFORMATION_VARIANT_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Variant, DataKind)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformationPtr>, VariantChoices))
    #define META_INFORMATION_OPTIONAL_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Optional, DataKind)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, UnderlyingType))
    #define META_INFORMATION_COLLECTION_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Collection, DataKind)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, UnderlyingType))
    #define META_INFORMATION_DICTIONARY_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Dictionary, DataKind)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, KeyType)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, DataType))
    #define META_INFORMATION_ONE_STRUCT_FIELD_FIELDS \
        ((std::string, FieldName)) \
        ((dev::cd606::tm::basic::MetaInformationPtr, FieldType))
    #define META_INFORMATION_STRUCTURE_FIELDS \
        ((dev::cd606::tm::basic::meta_information_helper::Structure, DataKind)) \
        ((std::string, TypeID)) \
        ((std::string, TypeReferenceName)) \
        ((std::vector<dev::cd606::tm::basic::MetaInformation_OneStructField>, Fields)) 
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

    #define TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_LIST (uint8_t) (uint16_t) (uint32_t) (uint64_t) (int8_t) (int16_t) (int32_t) (int64_t) (bool) (char) (float) (double) (std::string) (std::string_view) (std::chrono::system_clock::time_point) (std::tm)
    #define TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_M(r, data, elem) \
        template <> \
        class MetaInformationGenerator<elem, void> { \
        public: \
            static MetaInformation generate() { \
                return MetaInformation_BuiltIn { \
                    {} \
                    , #elem \
                }; \
            } \
        };
    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_M, _, TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_LIST);
    #undef TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_LIST
    #undef TM_BASIC_META_INFORMATION_HELPER_BUILT_IN_M

    template <>
    class MetaInformationGenerator<
        unsigned long long
        , std::enable_if_t<
            !std::is_same_v<unsigned long long, uint64_t>
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
    template <>
    class MetaInformationGenerator<
        long long
        , std::enable_if_t<
            !std::is_same_v<long long, int64_t>
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

    #define TM_BASIC_META_INFORMATION_HELPER_TM_LIST (basic::DateHolder)
    #define TM_BASIC_META_INFORMATION_HELPER_TM_M(r, data, elem) \
        template <> \
        class MetaInformationGenerator<elem, void> { \
        public: \
            static MetaInformation generate() { \
                return MetaInformation_TM { \
                    {} \
                    , #elem \
                }; \
            } \
        };
    BOOST_PP_SEQ_FOR_EACH(TM_BASIC_META_INFORMATION_HELPER_TM_M, _, TM_BASIC_META_INFORMATION_HELPER_TM_LIST);
    #undef TM_BASIC_META_INFORMATION_HELPER_TM_LIST
    #undef TM_BASIC_META_INFORMATION_HELPER_TM_M

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
            for (auto const &x : bytedata_utils::IsEnumWithStringRepresentation<T>::namesAndValues) {
                info.push_back({
                    std::string {x.first}
                    , static_cast<int64_t>(x.second)
                });
            }
            return MetaInformation_Enum { 
                {} 
                , std::string {typeid(T).name()}
                , std::string {bytedata_utils::IsEnumWithStringRepresentation<T>::REFERENCE_NAME}
                , std::move(info)
            }; 
        }
    };

} } } }

#endif