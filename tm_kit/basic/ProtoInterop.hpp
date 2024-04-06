#ifndef TM_KIT_BASIC_PROTO_INTEROP_HPP_
#define TM_KIT_BASIC_PROTO_INTEROP_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/ConvertibleWithString.hpp>
#include <tm_kit/basic/EncodableThroughProxy.hpp>
#include <tm_kit/infra/TupleVariantHelper.hpp>

#include <iostream>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace proto_interop {

    struct ZigZag {};
    struct Fixed {};

    using Int32 = int32_t;
    using Int64 = int64_t;
    using UInt32 = uint32_t;
    using UInt64 = uint64_t;
    using SInt32 = SingleLayerWrapperWithTypeMark<ZigZag,int32_t>;
    using SInt64 = SingleLayerWrapperWithTypeMark<ZigZag,int64_t>;
    using Fixed32 = SingleLayerWrapperWithTypeMark<Fixed,uint32_t>;
    using Fixed64 = SingleLayerWrapperWithTypeMark<Fixed,uint64_t>;
    using SFixed32 = SingleLayerWrapperWithTypeMark<Fixed,int32_t>;
    using SFixed64 = SingleLayerWrapperWithTypeMark<Fixed,int64_t>;

    template <class T>
    class IgnoreProxiesForProtoInterop {
    public:
        static constexpr bool value = false;
    };

    namespace internal {
        class VarIntSupport {
        public:
            template <class IntType, typename=std::enable_if_t<std::is_integral_v<IntType> && std::is_unsigned_v<IntType>>>
            static void write(IntType data, std::ostream &os) {
                IntType remaining = data;
                while (remaining >= (uint8_t) 0x80) {
                    os << (uint8_t) (((uint8_t) (remaining & (IntType) 0x7f)) | (uint8_t) 0x80);
                    remaining = remaining >> 7;
                }
                os << (uint8_t) remaining;
            }
            template <class IntType, typename=std::enable_if_t<std::is_integral_v<IntType> && std::is_unsigned_v<IntType>>>
            static std::optional<std::size_t> read(IntType &data, std::string_view const &input, std::size_t start) {
                std::size_t currentPos = start;
                data = 0;
                std::size_t shift = 0;
                while (currentPos < input.length()) {
                    uint8_t b = (uint8_t) (input[currentPos]);
                    bool hasMore = ((b & (uint8_t) 0x80) != 0);
                    data = data | (((IntType) (b & (uint8_t) 0x7f)) << shift);
                    if (!hasMore) {
                        return (currentPos-start+1);
                    }
                    ++currentPos;
                    shift += 7;
                    if (shift >= sizeof(IntType)*8) {
                        return std::nullopt;
                    };
                }
                return std::nullopt;
            }
            static std::optional<std::size_t> actualLength(std::string_view const &input, std::size_t start) {
                uintmax_t x;
                return read<uintmax_t>(x, input, start);
            }
        };

        template <class IntType>
        class ZigZagIntSupport {
        };
        template <>
        class ZigZagIntSupport<int32_t> {
        public:
            static uint32_t encode(int32_t input) {
                return (uint32_t) ((input << 1) ^ (input >> 31));
            }
            static int32_t decode(uint32_t input) {
                return (((int32_t) (input >> 1)) ^ (-((int32_t) (input & 0x1))));
            }
        };
        template <>
        class ZigZagIntSupport<int64_t> {
        public:
            static uint64_t encode(int64_t input) {
                return (uint64_t) ((input << 1) ^ (input >> 63));
            }
            static int64_t decode(uint64_t input) {
                return (((int64_t) (input >> 1)) ^ (-((int64_t) (input & 0x1))));
            }
        };

        enum class ProtoWireType : uint8_t {
            VarInt = 0
            , Fixed64 = 1
            , LengthDelimited = 2
            , StartGroup = 3
            , EndGroup = 4
            , Fixed32 = 5
        };

        template <class T>
        class ProtoWireTypeForSubField {
        public:
            static constexpr ProtoWireType TheType = (
                (std::is_integral_v<T> || std::is_enum_v<T>)?ProtoWireType::VarInt:ProtoWireType::LengthDelimited
                );
        };
        template <>
        class ProtoWireTypeForSubField<float> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::Fixed32;
        };
        template <>
        class ProtoWireTypeForSubField<double> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::Fixed64;
        };
        template <>
        class ProtoWireTypeForSubField<SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::VarInt;
        };
        template <>
        class ProtoWireTypeForSubField<SingleLayerWrapperWithTypeMark<ZigZag,int64_t>> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::VarInt;
        };
        template <>
        class ProtoWireTypeForSubField<SingleLayerWrapperWithTypeMark<Fixed,int32_t>> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::Fixed32;
        };
        template <>
        class ProtoWireTypeForSubField<SingleLayerWrapperWithTypeMark<Fixed,int64_t>> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::Fixed64;
        };
        template <>
        class ProtoWireTypeForSubField<SingleLayerWrapperWithTypeMark<Fixed,uint32_t>> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::Fixed32;
        };
        template <>
        class ProtoWireTypeForSubField<SingleLayerWrapperWithTypeMark<Fixed,uint64_t>> {
        public:
            static constexpr ProtoWireType TheType = ProtoWireType::Fixed64;
        };

        struct FieldHeader {
            ProtoWireType wireType;
            uint64_t fieldNumber;
        };

        class FieldHeaderSupport {
        public:
            static void writeHeader(FieldHeader const &fh, std::ostream &os) {
                VarIntSupport::write<uint64_t>(
                    ((fh.fieldNumber << 3) | (uint64_t) fh.wireType)
                    , os
                );
            }
            static std::optional<std::size_t> readHeader(FieldHeader &fh, std::string_view const &input, std::size_t start, std::size_t *contentLen) {
                uint64_t x;
                auto res = VarIntSupport::read<uint64_t>(x, input, start);
                if (!res) {
                    return std::nullopt;
                }
                uint8_t wt = (uint8_t) (x & (uint64_t) 0x7);
                if (wt > (uint8_t) ProtoWireType::Fixed32) {
                    return std::nullopt;                    
                }
                fh.wireType = (ProtoWireType) wt;
                fh.fieldNumber = (x >> 3);
                *contentLen = 0;
                if (fh.wireType == ProtoWireType::LengthDelimited) {
                    uint64_t contentLenBig;
                    auto res1 = VarIntSupport::read<uint64_t>(contentLenBig, input, start+*res);
                    if (!res1) {
                        return std::nullopt;
                    } else {
                        *contentLen = (std::size_t) contentLenBig;
                        return *res+*res1;
                    }
                } else {
                    return res;
                }
            }
            static std::optional<std::size_t> fieldActualLength(FieldHeader const &fh, std::size_t fieldLen, std::string_view const &input, std::size_t start) {
                switch (fh.wireType) {
                case ProtoWireType::Fixed64:
                    return 8;
                case ProtoWireType::Fixed32:
                    return 4;
                case ProtoWireType::LengthDelimited:
                    return fieldLen;
                case ProtoWireType::VarInt:
                    return VarIntSupport::actualLength(input, start);
                default:
                    return 0;
                }
            }
        };

        template <class T>
        struct is_variant_check : public std::false_type {};

        template <class... Xs>
        struct is_variant_check<std::variant<Xs...>> : public std::true_type {};
    }

    template <class T, typename Enable=void>
    struct ProtoWrappable {
        static constexpr bool value = false;
    };

    template <class T, typename Enable=void>
    class ProtoEncoder {};

    template <class IntType>
    class ProtoEncoder<IntType, std::enable_if_t<
        (std::is_integral_v<IntType> && std::is_unsigned_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType> && !std::is_same_v<IntType,char>)
        , void
    >> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, IntType data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<IntType>(data, os);
        }
    };
    template <class IntType>
    struct ProtoWrappable<IntType, std::enable_if_t<
        (std::is_integral_v<IntType> && std::is_unsigned_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType> && !std::is_same_v<IntType,char>)
        , void
    >> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<int8_t, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, int8_t data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint8_t>((uint8_t) data, os);
        }
    };
    template <>
    struct ProtoWrappable<int8_t, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<int16_t, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, int16_t data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint16_t>((uint16_t) data, os);
        }
    };
    template <>
    struct ProtoWrappable<int16_t, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<bool, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, bool data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && !data) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint8_t>((uint8_t) data, os);
        }
    };
    template <>
    struct ProtoWrappable<bool, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<int32_t, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, int32_t data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint32_t>((uint32_t) data, os);
        }
    };
    template <>
    struct ProtoWrappable<int32_t, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<int64_t, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, int64_t data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>((uint64_t) data, os);
        }
    };
    template <>
    struct ProtoWrappable<int64_t, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<long long, std::conditional_t<
        ((!std::is_same_v<long long, int64_t>) && (sizeof(long long)==8))
        , void
        , std::monostate
    >> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, long long data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<int64_t>::write(fieldNumber, (int64_t) data, os, writeDefaultValue);
        }
    };
    template <>
    struct ProtoWrappable<long long, std::conditional_t<
        ((!std::is_same_v<long long, int64_t>) && (sizeof(long long)==8))
        , void
        , std::monostate
    >> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<SingleLayerWrapperWithTypeMark<ZigZag, int32_t>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithTypeMark<ZigZag, int32_t> const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.value == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint32_t>(internal::ZigZagIntSupport<int32_t>::encode(data.value), os);
        }
    };
    template <>
    struct ProtoWrappable<SingleLayerWrapperWithTypeMark<ZigZag, int32_t>, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<SingleLayerWrapperWithTypeMark<ZigZag, int64_t>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithTypeMark<ZigZag, int64_t> const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.value == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(internal::ZigZagIntSupport<int64_t>::encode(data.value), os);
        }
    };
    template <>
    struct ProtoWrappable<SingleLayerWrapperWithTypeMark<ZigZag, int64_t>, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<SingleLayerWrapperWithTypeMark<Fixed, uint32_t>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithTypeMark<Fixed, uint32_t> const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.value == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::Fixed32, *fieldNumber}
                    , os
                );
            }
            uint32_t x = data.value;
            boost::endian::native_to_little_inplace<uint32_t>(x);
            os.write(reinterpret_cast<char *>(&x), sizeof(uint32_t));
        }
    };
    template <>
    struct ProtoWrappable<SingleLayerWrapperWithTypeMark<Fixed, uint32_t>, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<SingleLayerWrapperWithTypeMark<Fixed, int32_t>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithTypeMark<Fixed, int32_t> const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.value == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::Fixed32, *fieldNumber}
                    , os
                );
            }
            uint32_t x = (uint32_t) data.value;
            boost::endian::native_to_little_inplace<uint32_t>(x);
            os.write(reinterpret_cast<char *>(&x), sizeof(uint32_t));
        }
    };
    template <>
    struct ProtoWrappable<SingleLayerWrapperWithTypeMark<Fixed, int32_t>, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<SingleLayerWrapperWithTypeMark<Fixed, uint64_t>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithTypeMark<Fixed, uint64_t> const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.value == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::Fixed64, *fieldNumber}
                    , os
                );
            }
            uint64_t x = data.value;
            boost::endian::native_to_little_inplace<uint64_t>(x);
            os.write(reinterpret_cast<char *>(&x), sizeof(uint64_t));
        }
    };
    template <>
    struct ProtoWrappable<SingleLayerWrapperWithTypeMark<Fixed, uint64_t>, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<SingleLayerWrapperWithTypeMark<Fixed, int64_t>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithTypeMark<Fixed, int64_t> const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.value == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::Fixed64, *fieldNumber}
                    , os
                );
            }
            uint64_t x = (uint64_t) data.value;
            boost::endian::native_to_little_inplace<uint64_t>(x);
            os.write(reinterpret_cast<char *>(&x), sizeof(uint64_t));
        }
    };
    template <>
    struct ProtoWrappable<SingleLayerWrapperWithTypeMark<Fixed, int64_t>, void> {
        static constexpr bool value = true;
    };
    
    template <class T>
    class ProtoEncoder<T, std::enable_if_t<std::is_enum_v<T>, void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, T data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<std::underlying_type_t<T>>::write(
                fieldNumber, static_cast<std::underlying_type_t<T>>(data), os, true
            );
        }
    };
    template <class T>
    struct ProtoWrappable<T, std::enable_if_t<std::is_enum_v<T>, void>> {
        static constexpr bool value = true;
    };

    template <>
    class ProtoEncoder<float, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, float data, std::ostream &os, bool /*writeDefaultValue*/) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::Fixed32, *fieldNumber}
                    , os
                );
            }
            uint32_t dBuf;
            std::memcpy(&dBuf, &data, 4);
            boost::endian::native_to_little_inplace<uint32_t>(dBuf);
            os.write(reinterpret_cast<char *>(&dBuf), 4);
        }
    };
    template <>
    struct ProtoWrappable<float, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<double, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, double data, std::ostream &os, bool /*writeDefaultValue*/) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::Fixed64, *fieldNumber}
                    , os
                );
            }
            uint64_t dBuf;
            std::memcpy(&dBuf, &data, 8);
            boost::endian::native_to_little_inplace<uint64_t>(dBuf);
            os.write(reinterpret_cast<char *>(&dBuf), 8);
        }
    };
    template <>
    struct ProtoWrappable<double, void> {
        static constexpr bool value = true;
    };
    
    template <>
    class ProtoEncoder<std::string, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::string const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.length() == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(data.length(), os);
            os.write(data.data(), data.length());
        }
    };
    template <>
    struct ProtoWrappable<std::string, void> {
        static constexpr bool value = true;
    };
    
    //string_view is not considered proto wrappable because we cannot decode it
    template <>
    class ProtoEncoder<std::string_view, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::string_view const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.length() == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(data.length(), os);
            os.write(data.data(), data.length());
        }
    };
    
    template <std::size_t N>
    class ProtoEncoder<std::array<char,N>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::array<char,N> const &data, std::ostream &os, bool writeDefaultValue) {
            std::size_t ii = 0;
            for (; ii<N; ++ii) {
                if (data[ii] == '\0') {
                    break;
                }
            }
            if (!writeDefaultValue && ii == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(ii, os);
            os.write(data.data(), ii);
        }
    };
    template <std::size_t N>
    struct ProtoWrappable<std::array<char,N>, void> {
        static constexpr bool value = true;
    };
    template <>
    class ProtoEncoder<ByteData, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, ByteData const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.content.length() == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(data.content.length(), os);
            os.write(data.content.data(), data.content.length());
        }
    };
    template <>
    struct ProtoWrappable<ByteData, void> {
        static constexpr bool value = true;
    };
    
    //ByteDataView is not considered proto wrappable because we cannot decode it
    template <>
    class ProtoEncoder<ByteDataView, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, ByteDataView const &data, std::ostream &os, bool writeDefaultValue) {
            if (!writeDefaultValue && data.content.length() == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(data.content.length(), os);
            os.write(data.content.data(), data.content.length());
        }
    };
    template <std::size_t N>
    class ProtoEncoder<std::array<unsigned char,N>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::array<unsigned char,N> const &data, std::ostream &os, bool writeDefaultValue) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(N, os);
            os.write(data.data(), N);
        }
    };
    template <std::size_t N>
    struct ProtoWrappable<std::array<unsigned char,N>, void> {
        static constexpr bool value = true;
    };

    template <class T>
    class ProtoEncoder<std::vector<T>, std::enable_if_t<
        (
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::vector<T> const &data, std::ostream &os, bool writeDefaultValue) {
            if (data.empty()) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            std::ostringstream ss;
            for (auto const &item : data) {
                ProtoEncoder<T>::write(std::nullopt, item, ss, true);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T>
    class ProtoEncoder<std::vector<T>, std::enable_if_t<
        !(
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::vector<T> const &data, std::ostream &os, bool writeDefaultValue) {
            for (auto const &item : data) {
                ProtoEncoder<T>::write(fieldNumber, item, os, true);
            }
        }
    };
    template <class T>
    struct ProtoWrappable<std::vector<T>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };
    template <class T>
    class ProtoEncoder<std::list<T>, std::enable_if_t<
        (
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::list<T> const &data, std::ostream &os, bool writeDefaultValue) {
            if (data.empty()) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            std::ostringstream ss;
            for (auto const &item : data) {
                ProtoEncoder<T>::write(std::nullopt, item, ss, true);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T>
    class ProtoEncoder<std::list<T>, std::enable_if_t<
        !(
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::list<T> const &data, std::ostream &os, bool writeDefaultValue) {
            for (auto const &item : data) {
                ProtoEncoder<T>::write(fieldNumber, item, os, true);
            }
        }
    };
    template <class T>
    struct ProtoWrappable<std::list<T>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };
    template <class T, std::size_t N>
    class ProtoEncoder<std::array<T,N>, std::enable_if_t<
        (
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::array<T,N> const &data, std::ostream &os, bool writeDefaultValue) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            std::ostringstream ss;
            for (auto const &item : data) {
                ProtoEncoder<T>::write(std::nullopt, item, ss, true);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T, std::size_t N>
    class ProtoEncoder<std::array<T,N>, std::enable_if_t<
        !(
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::array<T,N> const &data, std::ostream &os, bool writeDefaultValue) {
            for (auto const &item : data) {
                ProtoEncoder<T>::write(fieldNumber, item, os, true);
            }
        }
    };
    template <class T, std::size_t N>
    struct ProtoWrappable<std::array<T,N>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };
    
    template <class T>
    class ProtoEncoder<std::valarray<T>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::valarray<T> const &data, std::ostream &os, bool writeDefaultValue) {
            if (data.size() == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
                std::ostringstream ss;
                for (auto const &item : data) {
                    ProtoEncoder<T>::write(std::nullopt, item, ss, true);
                }
                auto cont = ss.str();
                internal::VarIntSupport::write<uint64_t>(cont.length(), os);
                os.write(cont.data(), cont.length());
            } else {
                for (auto const &item : data) {
                    ProtoEncoder<T>::write(std::nullopt, item, os, true);
                }
            }
        }
    };
    template <class T>
    struct ProtoWrappable<std::valarray<T>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };

    template <class K, class D, class Cmp>
    class ProtoEncoder<std::map<K,D,Cmp>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::map<K,D,Cmp> const &data, std::ostream &os, bool writeDefaultValue) {
            if (data.size() == 0) {
                return;
            }
            for (auto const &item : data) {
                if (fieldNumber) {
                    internal::FieldHeaderSupport::writeHeader(
                        internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                        , os
                    );
                    std::ostringstream ss;
                    ProtoEncoder<K>::write(1, item.first, ss, false);
                    ProtoEncoder<D>::write(2, item.second, ss, false);
                    std::string cont = ss.str();
                    internal::VarIntSupport::write<uint64_t>((uint64_t) cont.length(), os);
                    os.write(cont.data(), cont.length());
                } else {
                    ProtoEncoder<K>::write(1, item.first, os, false);
                    ProtoEncoder<D>::write(2, item.second, os, false);
                }
            }
        }
    };
    template <class K, class D, class Cmp>
    struct ProtoWrappable<std::map<K,D,Cmp>, void> {
        static constexpr bool value = ProtoWrappable<K>::value && ProtoWrappable<D>::value;
    };
    template <class K, class D, class Hash>
    class ProtoEncoder<std::unordered_map<K,D,Hash>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::unordered_map<K,D,Hash> const &data, std::ostream &os, bool writeDefaultValue) {
            if (data.size() == 0) {
                return;
            }
            for (auto const &item : data) {
                if (fieldNumber) {
                    internal::FieldHeaderSupport::writeHeader(
                        internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                        , os
                    );
                    std::ostringstream ss;
                    ProtoEncoder<K>::write(1, item.first, ss, false);
                    ProtoEncoder<D>::write(2, item.second, ss, false);
                    std::string cont = ss.str();
                    internal::VarIntSupport::write<uint64_t>((uint64_t) cont.length(), os);
                    os.write(cont.data(), cont.length());
                } else {
                    ProtoEncoder<K>::write(1, item.first, os, false);
                    ProtoEncoder<D>::write(2, item.second, os, false);
                }
            }
        }
    };
    template <class K, class D, class Hash>
    struct ProtoWrappable<std::unordered_map<K,D,Hash>, void> {
        static constexpr bool value = ProtoWrappable<K>::value && ProtoWrappable<D>::value;
    };
    
    template <class T>
    class ProtoEncoder<std::optional<T>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, std::optional<T> const &data, std::ostream &os, bool writeDefaultValue) {
            if (data) {
                ProtoEncoder<T>::write(fieldNumber, *data, os, true);
            }
        }
    };
    template <class T>
    struct ProtoWrappable<std::optional<T>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };
    
    template <>
    class ProtoEncoder<VoidStruct, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> /*fieldNumber*/, VoidStruct const &/*data*/, std::ostream &/*os*/, bool /*writeDefaultValue*/) {
        }
    };
    template <>
    struct ProtoWrappable<VoidStruct, void> {
        static constexpr bool value = true;
    };
    
    template <>
    class ProtoEncoder<std::monostate, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> /*fieldNumber*/, std::monostate const &/*data*/, std::ostream &/*os*/, bool /*writeDefaultValue*/) {
        }
    };
    template <>
    struct ProtoWrappable<std::monostate, void> {
        static constexpr bool value = true;
    };
    
    template <int32_t N>
    class ProtoEncoder<ConstType<N>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, ConstType<N> const &data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<int32_t>::write(fieldNumber, N, os, writeDefaultValue);
        }
    };
    template <int32_t N>
    struct ProtoWrappable<ConstType<N>, void> {
        static constexpr bool value = true;
    };
    template <typename T>
    class ProtoEncoder<ConstStringType<T>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, ConstStringType<T> const &data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<std::string>::write(fieldNumber, std::string {ConstStringType<T>::VALUE}, os, writeDefaultValue);
        }
    };
    template <typename T>
    struct ProtoWrappable<ConstStringType<T>, void> {
        static constexpr bool value = true;
    };

    //single layer wrapper and single layer wrapper with ID are treated differently
    //for protobuf purposes.
    //single layer wrapper of a variant is treated as if a single tuple, i.e. 
    //wrapping a layer of message outside the given type.
    //The use of wrapping is important for e.g. std::vector<std::variant>, since
    //proto does NOT support repeated oneof, therefore std::vector<std::variant>
    //will NOT encode correctly, so we would need a std::vector<SingleLayerWrapper<std::variant>>
    //however, single layer wrapper of a non-variant is discarded for protobuf purposes
    //, as if the underlying value is directly encoded/decoded

    //single layer wrapper with ID is used to assign a different proto field number
    //to the field WITHOUT wrapping another message.

    template <class T>
    class ProtoEncoder<SingleLayerWrapper<T>, std::enable_if_t<
        internal::is_variant_check<T>::value, void
    >> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapper<T> const &data, std::ostream &os, bool writeDefaultValue) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
                std::ostringstream ss;
                ProtoEncoder<T>::write(1, data.value, ss, false);
                std::string cont = ss.str();
                internal::VarIntSupport::write<uint64_t>((uint64_t) cont.length(), os);
                os.write(cont.data(), cont.length());
            } else {
                ProtoEncoder<T>::write(1, data.value, os, false);
            }
        }
    };
    template <class T>
    class ProtoEncoder<SingleLayerWrapper<T>, std::enable_if_t<
        !internal::is_variant_check<T>::value, void
    >> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<T>::thisFieldNumber(inputFieldNumber);
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<T>::nextFieldNumber(inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapper<T> const &data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<T>::write(fieldNumber, data.value, os, writeDefaultValue);
        }
    };
    template <class T>
    struct ProtoWrappable<SingleLayerWrapper<T>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };

    template <int32_t N, class T>
    class ProtoEncoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return (uint64_t) N;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return std::max((uint64_t) N+1,inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithID<N,T> const &data, std::ostream &os, bool writeDefaultValue) {
            std::optional<uint64_t> f = std::nullopt;
            if (fieldNumber) {
                f = (uint64_t) N;
            }
            ProtoEncoder<T>::write(f, data.value, os, writeDefaultValue);
        }
    };
    template <int32_t N, class T>
    struct ProtoWrappable<SingleLayerWrapperWithID<N,T>, void> {
        static constexpr bool value = ProtoWrappable<T>::value;
    };

    template <class... MoreVariants>
    class ProtoEncoder<std::variant<std::monostate,MoreVariants...>, void> {
    private:
        using TheVariant = std::variant<std::monostate,MoreVariants...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreVariants)+1;
        template <std::size_t FieldIndex>
        static constexpr uint64_t nextFieldNumber_internal(uint64_t inputFieldNumber) {
            if constexpr (FieldIndex < FieldCount) {
                return nextFieldNumber_internal<FieldIndex+1>(
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::nextFieldNumber(inputFieldNumber)
                );
            } else {
                return inputFieldNumber;
            }
        }
        template <std::size_t FieldIndex>
        static void write_internal(std::optional<uint64_t> fieldNumber, TheVariant const &data, std::ostream &os) {
            if constexpr (FieldIndex < FieldCount) {
                if (data.index() == FieldIndex) {
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::write(
                        fieldNumber 
                        , std::get<FieldIndex>(data)
                        , os
                        , true
                    );
                } else {
                    std::optional<uint64_t> nextField = std::nullopt;
                    if (fieldNumber) {
                        nextField = ProtoEncoder<
                            std::variant_alternative_t<FieldIndex, TheVariant>
                        >::nextFieldNumber(*fieldNumber);
                    }
                    write_internal<FieldIndex+1>(nextField, data, os);
                }
            }
        }
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return nextFieldNumber_internal<1>(inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, std::variant<std::monostate,MoreVariants...> const &data, std::ostream &os, bool writeDefaultValue) {
            write_internal<1>(fieldNumber, data, os);
        }
    };
    template <class... MoreVariants>
    struct ProtoWrappable<std::variant<std::monostate,MoreVariants...>, void> {
    private:
        using TheVariant = std::variant<std::monostate,MoreVariants...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreVariants)+1;
        template <std::size_t FieldIndex>
        static constexpr bool value_internal() {
            if constexpr (FieldIndex < FieldCount) {
                if (!ProtoWrappable<std::variant_alternative_t<FieldIndex, TheVariant>>::value) {
                    return false;
                } else {
                    return value_internal<FieldIndex+1>();
                }
            } else {
                return true;
            }
        }
    public:
        static constexpr bool value = value_internal<1>();
    };
    template <class FirstVariant, class... MoreVariants>
    class ProtoEncoder<std::variant<FirstVariant,MoreVariants...>, std::enable_if_t<!std::is_same_v<FirstVariant,std::monostate>, void>> {
    private:
        using TheVariant = std::variant<FirstVariant,MoreVariants...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreVariants)+1;
        template <std::size_t FieldIndex>
        static constexpr uint64_t nextFieldNumber_internal(uint64_t inputFieldNumber) {
            if constexpr (FieldIndex < FieldCount) {
                return nextFieldNumber_internal<FieldIndex+1>(
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::nextFieldNumber(inputFieldNumber)
                );
            } else {
                return inputFieldNumber;
            }
        }
        template <std::size_t FieldIndex>
        static void write_internal(std::optional<uint64_t> fieldNumber, TheVariant const &data, std::ostream &os) {
            if constexpr (FieldIndex < FieldCount) {
                if (data.index() == FieldIndex) {
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::write(
                        fieldNumber 
                        , std::get<FieldIndex>(data)
                        , os
                        , true
                    );
                } else {
                    std::optional<uint64_t> nextField = std::nullopt;
                    if (fieldNumber) {
                        nextField = ProtoEncoder<
                            std::variant_alternative_t<FieldIndex, TheVariant>
                        >::nextFieldNumber(*fieldNumber);
                    }
                    write_internal<FieldIndex+1>(nextField, data, os);
                }
            }
        }
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return nextFieldNumber_internal<0>(inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, std::variant<FirstVariant,MoreVariants...> const &data, std::ostream &os, bool writeDefaultValue) {
            write_internal<0>(fieldNumber, data, os);
        }
    };
    template <class FirstVariant, class... MoreVariants>
    struct ProtoWrappable<std::variant<FirstVariant,MoreVariants...>, std::enable_if_t<!std::is_same_v<FirstVariant,std::monostate>, void>> {
    private:
        using TheVariant = std::variant<FirstVariant,MoreVariants...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreVariants)+1;
        template <std::size_t FieldIndex>
        static constexpr bool value_internal() {
            if constexpr (FieldIndex < FieldCount) {
                if (!ProtoWrappable<std::variant_alternative_t<FieldIndex, TheVariant>>::value) {
                    return false;
                } else {
                    return value_internal<FieldIndex+1>();
                }
            } else {
                return true;
            }
        }
    public:
        static constexpr bool value = value_internal<0>();
    };

    template <class FirstItem, class... MoreItems>
    class ProtoEncoder<std::tuple<FirstItem,MoreItems...>, void> {
    private:
        using TheTuple = std::tuple<FirstItem,MoreItems...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreItems)+1;
        template <std::size_t FieldIndex>
        static void write_impl(TheTuple const &data, std::ostream &os) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = std::tuple_element_t<FieldIndex,TheTuple>;
                ProtoEncoder<F>::write(FieldIndex+1, std::get<FieldIndex>(data), os, false);
                write_impl<FieldIndex+1>(data, os);
            }
        }
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, TheTuple const &data, std::ostream &os, bool writeDefaultValue) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
                std::ostringstream ss;
                write_impl<0>(data, ss);
                std::string cont = ss.str();
                internal::VarIntSupport::write<uint64_t>((uint64_t) cont.length(), os);
                os.write(cont.data(), cont.length());
            } else {
                write_impl<0>(data, os);
            }
        }
    };
    template <class FirstItem, class... MoreItems>
    struct ProtoWrappable<std::tuple<FirstItem,MoreItems...>, void> {
    private:
        using TheTuple = std::tuple<FirstItem,MoreItems...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreItems)+1;
        template <std::size_t FieldIndex>
        static constexpr bool value_internal() {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = std::tuple_element_t<FieldIndex,TheTuple>;
                if constexpr (!ProtoWrappable<F>::value) {
                    return false;
                } else if constexpr (internal::is_variant_check<F>::value) {
                    return false;
                } else {
                    return value_internal<FieldIndex+1>();
                }
            } else {
                return true;
            }
        }
    public:
        static constexpr bool value = value_internal<0>();
    };

    template <class T>
    class ProtoEncoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void buildIndices_impl(std::array<uint64_t,FieldCount> &ret, uint64_t current) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                ret[FieldIndex] = ProtoEncoder<F>::thisFieldNumber(current);
                buildIndices_impl<FieldCount,FieldIndex+1>(ret, ProtoEncoder<F>::nextFieldNumber(current));
            }
        }
        static std::array<uint64_t, StructFieldInfo<T>::FIELD_NAMES.size()> buildIndices() {
            std::array<uint64_t, StructFieldInfo<T>::FIELD_NAMES.size()> ret;
            buildIndices_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>(ret,1);
            return ret;
        }
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void write_impl(T const &data, std::ostream &os, std::array<uint64_t, FieldCount> const &indices) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                ProtoEncoder<F>::write(indices[FieldIndex], StructFieldTypeInfo<T,FieldIndex>::constAccess(data), os, false);
                write_impl<FieldCount,FieldIndex+1>(data, os, indices);
            }
        }
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, T const &data, std::ostream &os, bool /*writeDefaultValue*/) {
            static std::array<uint64_t, StructFieldInfo<T>::FIELD_NAMES.size()> indices = buildIndices();
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
                std::ostringstream ss;
                write_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>(data, ss, indices);
                std::string cont = ss.str();
                internal::VarIntSupport::write<uint64_t>((uint64_t) cont.length(), os);
                os.write(cont.data(), cont.length());
            } else {
                write_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>(data, os, indices);
            }
        }
    };
    template <class T>
    struct ProtoWrappable<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static constexpr bool value_internal() {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if (!ProtoWrappable<F>::value) {
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
    class ProtoEncoder<T, std::enable_if_t<
        bytedata_utils::ProtobufStyleSerializableChecker<T>::IsProtobufStyleSerializable()
        , void
    >> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber;
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return inputFieldNumber+1;
        }
        static void write(std::optional<uint64_t> fieldNumber, T const &data, std::ostream &os, bool writeDefaultValue) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
                std::string s;
                data.SerializeToString(&s);
                internal::VarIntSupport::write<uint64_t>((uint64_t) s.length(), os);
                os.write(s.data(), s.length());
            } else {
                std::string s;
                data.SerializeToString(&s);
                os.write(s.data(), s.length());
            }
        }
    };
    template <class T>
    struct ProtoWrappable<T, std::enable_if_t<
        bytedata_utils::ProtobufStyleSerializableChecker<T>::IsProtobufStyleSerializable()
        , void
    >> {
        static constexpr bool value = true;
    };
    
    class IProtoDecoderBase {
    public:
        virtual ~IProtoDecoderBase() = default;
        virtual std::optional<std::size_t> handle(internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) = 0;
        std::optional<std::size_t> handleWithOuterHeader(std::string_view const &input, std::size_t start) {
            if (input.length() == start) {
                return 0;
            }
            internal::FieldHeader fh;
            std::size_t fieldLen;
            auto res = internal::FieldHeaderSupport::readHeader(fh, input, start, &fieldLen);
            if (!res) {
                return std::nullopt;
            }
            return handle(fh, input, start+(*res));
        }
    };
    template <class T>
    class IProtoDecoder : public IProtoDecoderBase {
    private:
        T *output_;
    public:
        IProtoDecoder(T *output) : output_(output) {}
        virtual ~IProtoDecoder() = default;
    protected:
        virtual std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) = 0;
    public:
        std::optional<std::size_t> handle(internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            return read(*output_, fh, input, start);
        }
    };

    template <class T, typename=void>
    class ProtoDecoder {};

    template <class IntType>
    class ProtoDecoder<IntType, std::enable_if_t<(std::is_integral_v<IntType> && std::is_unsigned_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType> && !std::is_same_v<IntType,char>), void>> final : public IProtoDecoder<IntType> {
    public:
        ProtoDecoder(IntType *output, uint64_t baseFieldNumber) : IProtoDecoder<IntType>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(IntType &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                return internal::VarIntSupport::read<IntType>(output, input, start);
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output = (IntType) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output = (IntType) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
            
        }
    };
    template <>
    class ProtoDecoder<int8_t, void> final : public IProtoDecoder<int8_t> {
    public:
        ProtoDecoder(int8_t *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<int8_t>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(int8_t &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output = (int8_t) x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output = (int8_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output = (int8_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<int16_t, void> final : public IProtoDecoder<int16_t> {
    public:
        ProtoDecoder(int16_t *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<int16_t>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(int16_t &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output = (int16_t) x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output = (int16_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output = (int16_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<bool, void> final : public IProtoDecoder<bool> {
    public:
        ProtoDecoder(bool *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<bool>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(bool &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::VarInt) {
                return std::nullopt;
            }
            uint8_t x;
            auto res = internal::VarIntSupport::read<uint8_t>(x, input, start);
            if (res) {
                output = (bool) x;
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<int32_t, void> final : public IProtoDecoder<int32_t> {
    public:
        ProtoDecoder(int32_t *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<int32_t>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(int32_t &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output = (int32_t) x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output = (int32_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output = (int32_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<int64_t, void> final : public IProtoDecoder<int64_t> {
    public:
        ProtoDecoder(int64_t *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<int64_t>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(int64_t &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint64_t x;
                    auto res = internal::VarIntSupport::read<uint64_t>(x, input, start);
                    if (res) {
                        output = (int64_t) x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output = (int64_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output = (int64_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<long long, std::conditional_t<
        ((!std::is_same_v<long long, int64_t>) && (sizeof(long long)==8))
        , void
        , std::monostate
    >> final : public IProtoDecoder<long long> {
    public:
        ProtoDecoder(long long *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<long long>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(long long &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            int64_t x;
            ProtoDecoder<int64_t> dec(&x, 0);
            auto s = dec.handle(fh, input, start);
            if (!s) {
                return std::nullopt;
            }
            output = (long long) x;
            return s;
        }
    };
    template <>
    class ProtoDecoder<SingleLayerWrapperWithTypeMark<ZigZag, int32_t>, void> final : public IProtoDecoder<SingleLayerWrapperWithTypeMark<ZigZag, int32_t>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithTypeMark<ZigZag, int32_t> *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<SingleLayerWrapperWithTypeMark<ZigZag, int32_t>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithTypeMark<ZigZag, int32_t> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output.value = internal::ZigZagIntSupport<int32_t>::decode(x);
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output.value = (int32_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output.value = (int32_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<SingleLayerWrapperWithTypeMark<ZigZag, int64_t>, void> final : public IProtoDecoder<SingleLayerWrapperWithTypeMark<ZigZag, int64_t>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithTypeMark<ZigZag, int64_t> *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithTypeMark<ZigZag, int64_t> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint64_t x;
                    auto res = internal::VarIntSupport::read<uint64_t>(x, input, start);
                    if (res) {
                        output.value = internal::ZigZagIntSupport<int64_t>::decode(x);
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output.value = (int64_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output.value = (int64_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,int32_t>, void> final : public IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,int32_t>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithTypeMark<Fixed,int32_t> *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,int32_t>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithTypeMark<Fixed,int32_t> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output.value = (int32_t) x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output.value = (int32_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output.value = (int32_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,int64_t>, void> final : public IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,int64_t>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithTypeMark<Fixed,int64_t> *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,int64_t>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithTypeMark<Fixed,int64_t> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint64_t x;
                    auto res = internal::VarIntSupport::read<uint64_t>(x, input, start);
                    if (res) {
                        output.value = (int64_t) x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output.value = (int64_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output.value = (int64_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,uint32_t>, void> final : public IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,uint32_t>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithTypeMark<Fixed,uint32_t> *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,uint32_t>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithTypeMark<Fixed,uint32_t> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output.value = x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output.value = x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output.value = (uint32_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };
    template <>
    class ProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,uint64_t>, void> final : public IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,uint64_t>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithTypeMark<Fixed,uint64_t> *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<SingleLayerWrapperWithTypeMark<Fixed,uint64_t>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithTypeMark<Fixed,uint64_t> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            switch (fh.wireType) {
            case internal::ProtoWireType::VarInt:
                {
                    uint64_t x;
                    auto res = internal::VarIntSupport::read<uint64_t>(x, input, start);
                    if (res) {
                        output.value = x;
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (start+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint32_t));
                    boost::endian::little_to_native_inplace<uint32_t>(x);
                    output.value = (uint64_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (start+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint64_t x;
                    std::memcpy(&x, input.data()+start, sizeof(uint64_t));
                    boost::endian::little_to_native_inplace<uint64_t>(x);
                    output.value = x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
    };

    template <class T>
    class ProtoDecoder<T, std::enable_if_t<std::is_enum_v<T>, void>> final : public IProtoDecoder<T> {
    public:
        ProtoDecoder(T *output, uint64_t baseFieldNumber) : IProtoDecoder<T>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            std::underlying_type_t<T> x;
            ProtoDecoder<std::underlying_type_t<T>> dec(&x, 1);
            auto res = dec.handle(fh, input, start);
            if (res) {
                output = (T) x;
            }
            return res;
        }
    };

    template <>
    class ProtoDecoder<float, void> final : public IProtoDecoder<float> {
    public:
        ProtoDecoder(float *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<float>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(float &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::Fixed32) {
                return std::nullopt;
            }
            if (start+sizeof(uint32_t) > input.length()) {
                return std::nullopt;
            }
            uint32_t x;
            std::memcpy(&x, input.data()+start, sizeof(uint32_t));
            boost::endian::little_to_native_inplace<uint32_t>(x);
            std::memcpy(&output, &x, sizeof(uint32_t));
            return sizeof(uint32_t);
        }
    };
    template <>
    class ProtoDecoder<double, void> final : public IProtoDecoder<double> {
    public:
        ProtoDecoder(double *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<double>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(double &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::Fixed64) {
                return std::nullopt;
            }
            if (start+sizeof(uint32_t) > input.length()) {
                return std::nullopt;
            }
            uint64_t x;
            std::memcpy(&x, input.data()+start, sizeof(uint64_t));
            boost::endian::little_to_native_inplace<uint64_t>(x);
            std::memcpy(&output, &x, sizeof(uint64_t));
            return sizeof(uint64_t);
        }
    };

    template <>
    class ProtoDecoder<std::string, void> final : public IProtoDecoder<std::string> {
    public:
        ProtoDecoder(std::string *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<std::string>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(std::string &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            output = std::string(input.substr(start));
            return (input.length()-start);
        }
    };
    template <std::size_t N>
    class ProtoDecoder<std::array<char,N>, void> final : public IProtoDecoder<std::array<char,N>> {
    public:
        ProtoDecoder(std::array<char,N> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::array<char,N>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(std::array<char,N> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::memset(output.data(), 0, N);
            std::memcpy(output.data(), input.data()+start, std::min(input.length()-start, N));
            return (input.length()-start);
        }
    };
    template <>
    class ProtoDecoder<ByteData, void> final : public IProtoDecoder<ByteData> {
    public:
        ProtoDecoder(ByteData *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<ByteData>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(ByteData &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            output.content = std::string(input.substr(start));
            return (input.length()-start);
        }
    };
    template <std::size_t N>
    class ProtoDecoder<std::array<unsigned char,N>, void> final : public IProtoDecoder<std::array<unsigned char,N>> {
    public:
        ProtoDecoder(std::array<unsigned char,N> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::array<unsigned char,N>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(std::array<unsigned char,N> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::memset(output.data(), 0, N);
            std::memcpy(output.data(), input.data()+start, std::min(input.length()-start, N));
            return (input.length()-start);
        }
    };

    template <class T>
    class ProtoDecoder<std::vector<T>, std::enable_if_t<
        (
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> : public IProtoDecoder<std::vector<T>> {
    public:
        ProtoDecoder(std::vector<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::vector<T>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::vector<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType == internal::ProtoWireType::LengthDelimited) {
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                std::size_t remaining = input.length()-start;
                std::size_t idx = start;
                do {
                    auto res = subDec.handle({internal::ProtoWireTypeForSubField<T>::TheType, fh.fieldNumber}, input, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    output.push_back(x);
                    idx += *res;
                    remaining -= *res;
                } while (remaining > 0);
                return input.length()-start;
            } else if (fh.wireType == internal::ProtoWireType::VarInt || fh.wireType == internal::ProtoWireType::Fixed32 || fh.wireType == internal::ProtoWireType::Fixed64) {
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                auto res = subDec.handle(fh, input, start);
                if (res) {
                    output.push_back(x);
                }
                return res;
            } else {
                return std::nullopt;
            }
        }
    };
    template <class T>
    class ProtoDecoder<std::vector<T>, std::enable_if_t<
        !(
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> : public IProtoDecoder<std::vector<T>> {
    public:
        ProtoDecoder(std::vector<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::vector<T>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::vector<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;
            T x;
            ProtoDecoder<T> subDec(&x, baseFieldNumber_);
            do {
                auto res = subDec.handle({internal::ProtoWireTypeForSubField<T>::TheType, fh.fieldNumber}, input, idx);
                if (!res) {
                    return std::nullopt;
                }
                output.push_back(std::move(x));
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            return input.length()-start;
        }
    };
    template <class T>
    class ProtoDecoder<std::list<T>, std::enable_if_t<
        (
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> : public IProtoDecoder<std::list<T>> {
    public:
        ProtoDecoder(std::list<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::list<T>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::list<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType == internal::ProtoWireType::LengthDelimited) {
                uint64_t len;
                auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
                if (!lenRes) {
                    return std::nullopt;
                }
                if (start+*lenRes+len > input.length()) {
                    return std::nullopt;
                }
                std::string_view body = input.substr(start+*lenRes, len);
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                std::size_t remaining = input.length()-start;
                std::size_t idx = start;
                do {
                    auto res = subDec.handle({internal::ProtoWireTypeForSubField<T>::TheType, fh.fieldNumber}, input, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    output.push_back(std::move(x));
                    idx += *res;
                    remaining -= *res;
                } while (remaining > 0);
                return input.length()-start;
            } else if (fh.wireType == internal::ProtoWireType::VarInt || fh.wireType == internal::ProtoWireType::Fixed32 || fh.wireType == internal::ProtoWireType::Fixed64) {
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                auto res = subDec.handle(fh, input, start);
                if (res) {
                    output.push_back(x);
                }
                return res;
            } else {
                return std::nullopt;
            }
        }
    };
    template <class T>
    class ProtoDecoder<std::list<T>, std::enable_if_t<
        !(
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> : public IProtoDecoder<std::list<T>> {
    public:
        ProtoDecoder(std::list<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::list<T>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::list<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;
            T x;
            ProtoDecoder<T> subDec(&x, baseFieldNumber_);
            do {
                auto res = subDec.handle({internal::ProtoWireTypeForSubField<T>::TheType, fh.fieldNumber}, input, idx);
                if (!res) {
                    return std::nullopt;
                }
                output.push_back(std::move(x));
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            return (input.length()-start);
        }
    };
    template <class T, std::size_t N>
    class ProtoDecoder<std::array<T, N>, std::enable_if_t<
        (
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> : public IProtoDecoder<std::array<T,N>> {
    private:
        std::size_t curIdx_;
        void addItem(std::array<T, N> &output, T t) {
            if constexpr (N > 0) {
                if (curIdx_ < N) {
                    output[curIdx_++] = t;
                } else {
                    if constexpr (N > 1) {
                        std::memmove(&output[0], &output[1], sizeof(T)*(N-1));
                    }
                    output[N-1] = t;
                }
            }
        }
    public:
        ProtoDecoder(std::array<T, N> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::array<T, N>>(output), curIdx_(0), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::array<T, N> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType == internal::ProtoWireType::LengthDelimited) {
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                std::size_t remaining = input.length()-start;
                std::size_t idx = start;
                do {
                    auto res = subDec.handle({internal::ProtoWireTypeForSubField<T>::TheType, fh.fieldNumber}, input, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    addItem(output, x);
                    idx += *res;
                    remaining -= *res;
                } while (remaining > 0);
                return (input.length()-start);
            } else if (fh.wireType == internal::ProtoWireType::VarInt || fh.wireType == internal::ProtoWireType::Fixed32 || fh.wireType == internal::ProtoWireType::Fixed64) {
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                auto res = subDec.handle(fh, input, start);
                if (res) {
                    addItem(output, x);
                }
                return res;
            } else {
                return std::nullopt;
            }
        }
    };
    template <class T, std::size_t N>
    class ProtoDecoder<std::array<T,N>, std::enable_if_t<
        !(
            std::is_arithmetic_v<T> || std::is_enum_v<T> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag,int32_t>> 
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<ZigZag, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, int64_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint32_t>>
        || std::is_same_v<T,SingleLayerWrapperWithTypeMark<Fixed, uint64_t>>
        )
    , void>> : public IProtoDecoder<std::array<T,N>> {
    private:
        std::size_t curIdx_;
        void addItem(std::array<T, N> &output, T &&t) {
            if constexpr (N > 0) {
                if (curIdx_ < N) {
                    output[curIdx_++] = std::move(t);
                } else {
                    if constexpr (N > 1) {
                        for (std::size_t ii=0; ii<N-1; ++ii) {
                            output[ii] = output[ii+1];
                        }
                    }
                    output[N-1] = std::move(t);
                }
            }
        }
    public:
        ProtoDecoder(std::array<T,N> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::array<T,N>>(output), curIdx_(0), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::array<T,N> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;
            T x;
            ProtoDecoder<T> subDec(&x, baseFieldNumber_);
            do {
                auto res = subDec.handle({internal::ProtoWireTypeForSubField<T>::TheType, fh.fieldNumber}, input, idx);
                if (!res) {
                    return std::nullopt;
                }
                addItem(output, std::move(x));
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            return (input.length()-start);
        }
    };
    template <class T>
    class ProtoDecoder<std::valarray<T>, void> final : public IProtoDecoder<std::valarray<T>> {
    public:
        ProtoDecoder(std::valarray<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::valarray<T>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::valarray<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType == internal::ProtoWireType::LengthDelimited) {
                std::vector<T> vec;
                ProtoDecoder<std::vector<T>> vecDec(&vec);
                auto res = vecDec.handle(fh, input, start);
                if (!res) {
                    return std::nullopt;
                }
                output = std::valarray<T>(vec.data(), vec.size());
                return res;
            } else if (fh.wireType == internal::ProtoWireType::VarInt || fh.wireType == internal::ProtoWireType::Fixed32 || fh.wireType == internal::ProtoWireType::Fixed64) {
                T x;
                ProtoDecoder<T> subDec(&x, baseFieldNumber_);
                auto res = subDec.handle(fh, input, start);
                if (res) {
                    std::valarray<T> newArr(output.size()+1);
                    newArr[std::slice(0,output.size(),1)] = output;
                    newArr[output.size()] = x;
                    output.resize(newArr.size());
                    output = newArr;
                }
                return res;
            } else {
                return std::nullopt;
            }
        }
    };
    template <class K, class D, class Cmp>
    class ProtoDecoder<std::map<K,D,Cmp>,void> : public IProtoDecoder<std::map<K,D,Cmp>> {
    public:
        ProtoDecoder(std::map<K,D,Cmp> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::map<K,D,Cmp>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::map<K,D,Cmp> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;

            K k;
            D d;
            struct_field_info_utils::StructFieldInfoBasedInitializer<K>::initialize(k);
            struct_field_info_utils::StructFieldInfoBasedInitializer<D>::initialize(d);

            if (remaining == 0) {
                //since we are reading until here, but got an empty input
                //that can only mean that the key and data are both optimized out
                output[k] = std::move(d);
                return 0;
            }
            
            ProtoDecoder<K> subDecK(&k, 1);
            ProtoDecoder<D> subDecD(&d, 2);
            do {
                internal::FieldHeader innerFh;
                std::size_t fieldLen;
                auto res = internal::FieldHeaderSupport::readHeader(innerFh, input, idx, &fieldLen);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
                if (innerFh.fieldNumber == 1) {
                    if (fieldLen > 0) {
                        res = subDecK.handle(innerFh, input.substr(idx, fieldLen), 0);
                    } else {
                        if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                            res = subDecK.handle(innerFh, std::string_view {}, 0);
                        } else {
                            res = subDecK.handle(innerFh, input, idx);
                        }
                    }
                    if (!res) {
                        return std::nullopt;
                    }
                } else if (innerFh.fieldNumber == 2) {
                    if (fieldLen > 0) {
                        res = subDecD.handle(innerFh, input.substr(idx, fieldLen), 0);
                    } else {
                        if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                            res = subDecD.handle(innerFh, std::string_view {}, 0);
                        } else {
                            res = subDecD.handle(innerFh, input, idx);
                        }
                    }
                    if (!res) {
                        return std::nullopt;
                    }
                }
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            output[k] = std::move(d);
            return (input.length()-start);
        }
    };
    template <class K, class D, class Hash>
    class ProtoDecoder<std::unordered_map<K,D,Hash>,void> : public IProtoDecoder<std::unordered_map<K,D,Hash>> {
    public:
        ProtoDecoder(std::unordered_map<K,D,Hash> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::unordered_map<K,D,Hash>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::unordered_map<K,D,Hash> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;

            K k;
            D d;
            struct_field_info_utils::StructFieldInfoBasedInitializer<K>::initialize(k);
            struct_field_info_utils::StructFieldInfoBasedInitializer<D>::initialize(d);

            if (remaining == 0) {
                //since we are reading until here, but got an empty input
                //that can only mean that the key and data are both optimized out
                output[k] = std::move(d);
                return 0;
            }
            
            ProtoDecoder<K> subDecK(&k, 1);
            ProtoDecoder<D> subDecD(&d, 2);
            do {
                internal::FieldHeader innerFh;
                std::size_t fieldLen;
                auto res = internal::FieldHeaderSupport::readHeader(innerFh, input, idx, &fieldLen);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
                if (innerFh.fieldNumber == 1) {
                    if (fieldLen > 0) {
                        res = subDecK.handle(innerFh, input.substr(idx, fieldLen), 0);
                    } else {
                        if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                            res = subDecK.handle(innerFh, std::string_view {}, 0);
                        } else {
                            res = subDecK.handle(innerFh, input, idx);
                        }
                    }
                    if (!res) {
                        return std::nullopt;
                    }
                } else if (innerFh.fieldNumber == 2) {
                    if (fieldLen > 0) {
                        res = subDecD.handle(innerFh, input.substr(idx, fieldLen), 0);
                    } else {
                        if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                            res = subDecD.handle(innerFh, std::string_view {}, 0);
                        } else {
                            res = subDecD.handle(innerFh, input, idx);
                        }
                    }
                    if (!res) {
                        return std::nullopt;
                    }
                }
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            output[k] = std::move(d);
            return (input.length()-start);
        }
    };
    template <class T>
    class ProtoDecoder<std::optional<T>, void> final : public IProtoDecoder<std::optional<T>> {
    public:
        ProtoDecoder(std::optional<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::optional<T>>(output), baseFieldNumber_(baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        uint64_t baseFieldNumber_;
        std::optional<std::size_t> read(std::optional<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            T x;
            ProtoDecoder<T> subDec(&x, baseFieldNumber_);
            auto res = subDec.handle(fh, input, start);
            if (res) {
                output = std::move(x);
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<VoidStruct, void> final : public IProtoDecoder<VoidStruct> {
    public:
        ProtoDecoder(VoidStruct *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<VoidStruct>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(VoidStruct &/*output*/, internal::FieldHeader const &/*fh*/, std::string_view const &/*input*/, std::size_t /*start*/) override final {
            return 0;
        }
    };
    template <>
    class ProtoDecoder<std::monostate, void> final : public IProtoDecoder<std::monostate> {
    public:
        ProtoDecoder(std::monostate *output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<std::monostate>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(std::monostate &/*output*/, internal::FieldHeader const &/*fh*/, std::string_view const &/*input*/, std::size_t /*start*/) override final {
            return 0;
        }
    };
    template <int32_t N>
    class ProtoDecoder<ConstType<N>, void> final : public IProtoDecoder<ConstType<N>> {
    public:
        ProtoDecoder(ConstType<N> *output, uint64_t baseFieldNumber) : IProtoDecoder<ConstType<N>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(ConstType<N> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            int32_t v = 0;
            ProtoDecoder<int32_t> subDec(&v, fh.fieldNumber);
            auto res = subDec.handle(fh, input, start);
            if (v != N) {
                return std::nullopt;
            }
            return res;
        }
    };
    template <typename T>
    class ProtoDecoder<ConstStringType<T>, void> final : public IProtoDecoder<ConstStringType<T>> {
    public:
        ProtoDecoder(ConstStringType<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<ConstStringType<T>>(output) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(ConstStringType<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            std::string v;
            ProtoDecoder<std::string> subDec(&v, fh.fieldNumber);
            auto res = subDec.handle(fh, input, start);
            if (std::string_view {v} != ConstStringType<T>::VALUE) {
                return std::nullopt;
            }
            return res;
        }
    };
    template <class T>
    class ProtoDecoder<SingleLayerWrapper<T>, std::enable_if_t<
        internal::is_variant_check<T>::value, void
    >> final : public IProtoDecoder<SingleLayerWrapper<T>> {
    private:
        ProtoDecoder<T> subDec_;
    public:
        ProtoDecoder(SingleLayerWrapper<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<SingleLayerWrapper<T>>(output), subDec_(&(output->value), 1) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapper<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(output.value);
            if (input.length() == start) {
                return 0;
            }
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;
            do {
                internal::FieldHeader innerFh;
                std::size_t fieldLen;
                auto res = internal::FieldHeaderSupport::readHeader(innerFh, input, idx, &fieldLen);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
                if (fieldLen > 0) {
                    res = subDec_.handle(innerFh, input.substr(idx, fieldLen), 0);
                } else {
                    if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                        res = subDec_.handle(innerFh, std::string_view {}, 0);
                    } else {
                        res = subDec_.handle(innerFh, input, idx);
                    }
                }
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            return input.length()-start;
        }
    };
    template <class T>
    class ProtoDecoder<SingleLayerWrapper<T>, std::enable_if_t<
        !internal::is_variant_check<T>::value, void
    >> final : public IProtoDecoder<SingleLayerWrapper<T>> {
    private:
        ProtoDecoder<T> subDec_;
    public:
        ProtoDecoder(SingleLayerWrapper<T> *output, uint64_t baseFieldNumber) : IProtoDecoder<SingleLayerWrapper<T>>(output), subDec_(&(output->value), baseFieldNumber) {}
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return ProtoDecoder<T>::responsibleForFieldNumbers(baseFieldNumber);
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapper<T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            return subDec_.handle(fh, input, start);
        }
    };
    template <int32_t N, class T>
    class ProtoDecoder<SingleLayerWrapperWithID<N,T>, void> final : public IProtoDecoder<SingleLayerWrapperWithID<N,T>> {
    private:
        ProtoDecoder<T> subDec_;
    public:
        ProtoDecoder(SingleLayerWrapperWithID<N,T> *output, uint64_t baseFieldNumber) : IProtoDecoder<SingleLayerWrapperWithID<N,T>>(output), subDec_(&(output->value), baseFieldNumber) {}
        ProtoDecoder(ProtoDecoder const &) = delete;
        ProtoDecoder(ProtoDecoder &&) = delete;
        ProtoDecoder &operator=(ProtoDecoder const &) = delete;
        ProtoDecoder &operator=(ProtoDecoder &&) = delete;
        virtual ~ProtoDecoder() = default;
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {(uint64_t) N};
        }
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithID<N,T> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.fieldNumber != N) {
                return std::nullopt;
            } else {
                return subDec_.handle(fh, input, start);
            }
        }
    };

    template <class... MoreVariants>
    class ProtoDecoder<std::variant<std::monostate,MoreVariants...>, void> final : public IProtoDecoder<std::variant<std::monostate,MoreVariants...>> {
    private:
        using TheVariant = std::variant<std::monostate,MoreVariants...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreVariants)+1;

        std::tuple<std::monostate,MoreVariants...> storage_;
        std::unordered_map<uint64_t, std::tuple<IProtoDecoderBase *, std::function<void(TheVariant &)>>> decoders_;

        template <std::size_t FieldIndex>
        static void addResponsibleFields_internal(uint64_t current, std::vector<uint64_t> &output) {
            if constexpr (FieldIndex < FieldCount) {
                auto v = ProtoDecoder<
                    std::variant_alternative_t<FieldIndex, TheVariant>
                >::responsibleForFieldNumbers(
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::thisFieldNumber(current)
                );
                std::copy(v.begin(), v.end(), std::back_inserter(output));
                addResponsibleFields_internal<FieldIndex+1>(
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::nextFieldNumber(current)
                    , output
                );
            }
        }
        template <std::size_t FieldIndex>
        void buildDecoders_internal(uint64_t current) {
            if constexpr (FieldIndex < FieldCount) {
                using F = std::variant_alternative_t<FieldIndex, TheVariant>;
                for (auto n : ProtoDecoder<F>::responsibleForFieldNumbers(
                    ProtoEncoder<F>::thisFieldNumber(current)
                )) {
                    decoders_[n] = {
                        new ProtoDecoder<F>(
                            &(std::get<FieldIndex>(storage_))
                            , ProtoEncoder<F>::thisFieldNumber(current)
                        )
                        , [this](TheVariant &output) {
                            output.template emplace<FieldIndex>(
                                std::move(std::get<FieldIndex>(storage_))
                            );
                        }
                    };
                }
                buildDecoders_internal<FieldIndex+1>(
                    ProtoEncoder<F>::nextFieldNumber(current)
                );
            }
        }
    public:
        ProtoDecoder(std::variant<std::monostate,MoreVariants...> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::variant<std::monostate,MoreVariants...>>(output), storage_(), decoders_() {
            buildDecoders_internal<1>(baseFieldNumber);
        }
        virtual ~ProtoDecoder() {
            for (auto &item : decoders_) {
                delete std::get<0>(item.second);
                std::get<0>(item.second) = nullptr;
            }
            decoders_.clear();
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            std::vector<uint64_t> ret;
            addResponsibleFields_internal<1>(baseFieldNumber, ret);
            return ret;
        }
    protected:
        std::optional<std::size_t> read(std::variant<std::monostate,MoreVariants...> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            auto iter = decoders_.find(fh.fieldNumber);
            if (iter == decoders_.end()) {
                return internal::FieldHeaderSupport::fieldActualLength(fh, input.length()-start, input, start);
            }
            auto *p = std::get<0>(iter->second);
            if (!p) {
                return internal::FieldHeaderSupport::fieldActualLength(fh, input.length()-start, input, start);
            }
            auto res = p->handle(fh, input, start);
            if (!res) {
                return std::nullopt;
            }
            (std::get<1>(iter->second))(output);
            return res;
        }
    };
    template <class FirstVariant, class... MoreVariants>
    class ProtoDecoder<std::variant<FirstVariant,MoreVariants...>, std::enable_if_t<!std::is_same_v<FirstVariant,std::monostate>,void>> final : public IProtoDecoder<std::variant<FirstVariant,MoreVariants...>> {
    private:
        using TheVariant = std::variant<FirstVariant,MoreVariants...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreVariants)+1;

        std::tuple<FirstVariant,MoreVariants...> storage_;
        std::unordered_map<uint64_t, std::tuple<IProtoDecoderBase *, std::function<void(TheVariant &)>>> decoders_;

        template <std::size_t FieldIndex>
        static void addResponsibleFields_internal(uint64_t current, std::vector<uint64_t> &output) {
            if constexpr (FieldIndex < FieldCount) {
                auto v = ProtoDecoder<
                    std::variant_alternative_t<FieldIndex, TheVariant>
                >::responsibleForFieldNumbers(
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::thisFieldNumber(current)
                );
                std::copy(v.begin(), v.end(), std::back_inserter(output));
                addResponsibleFields_internal<FieldIndex+1>(
                    ProtoEncoder<
                        std::variant_alternative_t<FieldIndex, TheVariant>
                    >::nextFieldNumber(current)
                    , output
                );
            }
        }
        template <std::size_t FieldIndex>
        void buildDecoders_internal(uint64_t current) {
            if constexpr (FieldIndex < FieldCount) {
                using F = std::variant_alternative_t<FieldIndex, TheVariant>;
                for (auto n : ProtoDecoder<F>::responsibleForFieldNumbers(
                    ProtoEncoder<F>::thisFieldNumber(current)
                )) {
                    decoders_[n] = {
                        new ProtoDecoder<F>(
                            &(std::get<FieldIndex>(storage_))
                            , ProtoEncoder<F>::thisFieldNumber(current)
                        )
                        , [this](TheVariant &output) {
                            output.template emplace<FieldIndex>(
                                std::move(std::get<FieldIndex>(storage_))
                            );
                        }
                    };
                }
                buildDecoders_internal<FieldIndex+1>(
                    ProtoEncoder<F>::nextFieldNumber(current)
                );
            }
        }
    public:
        ProtoDecoder(std::variant<FirstVariant,MoreVariants...> *output, uint64_t baseFieldNumber) : IProtoDecoder<std::variant<FirstVariant,MoreVariants...>>(output), storage_(), decoders_() {
            buildDecoders_internal<0>(baseFieldNumber);
        }
        virtual ~ProtoDecoder() {
            for (auto &item : decoders_) {
                delete std::get<0>(item.second);
                std::get<0>(item.second) = nullptr;
            }
            decoders_.clear();
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            std::vector<uint64_t> ret;
            addResponsibleFields_internal<0>(baseFieldNumber, ret);
            return ret;
        }
    protected:
        std::optional<std::size_t> read(std::variant<FirstVariant,MoreVariants...> &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            auto iter = decoders_.find(fh.fieldNumber);
            if (iter == decoders_.end()) {
                return internal::FieldHeaderSupport::fieldActualLength(fh, input.length()-start, input, start);
            }
            auto *p = std::get<0>(iter->second);
            if (!p) {
                return internal::FieldHeaderSupport::fieldActualLength(fh, input.length()-start, input, start);
            }
            auto res = p->handle(fh, input, start);
            if (!res) {
                return std::nullopt;
            }
            (std::get<1>(iter->second))(output);
            return res;
        }
    };
    template <class FirstItem, class... MoreItems>
    class ProtoDecoder<std::tuple<FirstItem,MoreItems...>, void> : public IProtoDecoder<std::tuple<FirstItem,MoreItems...>> {
    private:
        using TheTuple = std::tuple<FirstItem,MoreItems...>;
        static constexpr std::size_t FieldCount = sizeof...(MoreItems)+1;

        std::array<IProtoDecoderBase *, FieldCount> decoders_;

        template <std::size_t FieldIndex>
        static constexpr void buildDecoders_impl(TheTuple *t, std::array<IProtoDecoderBase *, FieldCount> &ret) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = std::tuple_element_t<FieldIndex,TheTuple>;
                ret[FieldIndex] = new ProtoDecoder<F>(
                    &(std::get<FieldIndex>(*t))
                    , FieldIndex+1
                );
                buildDecoders_impl<FieldIndex+1>(t, ret);
            }
        }
        template <std::size_t FieldIndex>
        static void clearData(TheTuple &output) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = std::tuple_element_t<FieldIndex,TheTuple>;
                struct_field_info_utils::StructFieldInfoBasedInitializer<F>::initialize(std::get<FieldIndex>(output));
                clearData<FieldIndex+1>(output);
            }
        }
    public:
        ProtoDecoder(TheTuple* output, uint64_t baseFieldNumber) : IProtoDecoder<TheTuple>(output), decoders_() {
            buildDecoders_impl<0>(output, decoders_);
        }
        virtual ~ProtoDecoder() {
            for (auto &item : decoders_) {
                delete item;
            }
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(TheTuple &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            clearData<0>(output);
            //generated proto C++ code may actually send empty message for a struct, 
            //so this corner case must be considered
            if (input.length() == start) {
                return 0;
            }
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;
            do {
                internal::FieldHeader innerFh;
                std::size_t fieldLen;
                auto res = internal::FieldHeaderSupport::readHeader(innerFh, input, idx, &fieldLen);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
                if (innerFh.fieldNumber < 1 || innerFh.fieldNumber > FieldCount) {
                    auto jumpLen = internal::FieldHeaderSupport::fieldActualLength(innerFh, fieldLen, input, idx);
                    if (!jumpLen) {
                        return std::nullopt;
                    }
                    idx += *jumpLen;
                    remaining -= *jumpLen;
                    continue;
                }
                auto decoder = decoders_[innerFh.fieldNumber-1];
                if (fieldLen > 0) {
                    res = decoder->handle(innerFh, input.substr(idx, fieldLen), 0);
                } else {
                    if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                        res = decoder->handle(innerFh, std::string_view {}, 0);
                    } else {
                        res = decoder->handle(innerFh, input, idx);
                    }
                }
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            return input.length()-start;
        }
    };

    template <class T>
    class ProtoDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> final : public IProtoDecoder<T> {
    private:
        std::unordered_map<uint64_t, IProtoDecoderBase *> decoders_;

        template <std::size_t FieldCount, std::size_t FieldIndex>
        static constexpr void buildDecoders_impl(T *t, std::unordered_map<uint64_t, IProtoDecoderBase *> &ret, uint64_t current) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                for (auto n : ProtoDecoder<F>::responsibleForFieldNumbers(
                    ProtoEncoder<F>::thisFieldNumber(current)
                )) {
                    ret[n] = new ProtoDecoder<F>(
                        &(StructFieldTypeInfo<T,FieldIndex>::access(*t))
                        , ProtoEncoder<F>::thisFieldNumber(current)
                    );
                }
                buildDecoders_impl<FieldCount,FieldIndex+1>(
                    t 
                    , ret 
                    , ProtoEncoder<F>::nextFieldNumber(current)
                );
            }
        }
    public:
        ProtoDecoder(T* output, uint64_t /*baseFieldNumber*/) : IProtoDecoder<T>(output), decoders_() {
            buildDecoders_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>(output, decoders_, 1);
        }
        virtual ~ProtoDecoder() {
            for (auto &item : decoders_) {
                delete item.second;
                item.second = nullptr;
            }
            decoders_.clear();
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(output);
            //generated proto C++ code may actually send empty message for a struct, 
            //so this corner case must be considered
            if (input.length() == start) {
                return 0;
            }
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::size_t remaining = input.length()-start;
            std::size_t idx = start;
            do {
                internal::FieldHeader innerFh;
                std::size_t fieldLen;
                auto res = internal::FieldHeaderSupport::readHeader(innerFh, input, idx, &fieldLen);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
                auto iter = decoders_.find(innerFh.fieldNumber);
                if (iter == decoders_.end()) {
                    auto jumpLen = internal::FieldHeaderSupport::fieldActualLength(innerFh, fieldLen, input, idx);
                    if (!jumpLen) {
                        return std::nullopt;
                    }
                    idx += *jumpLen;
                    remaining -= *jumpLen;
                    continue;
                }
                if (iter->second == nullptr) {
                    auto jumpLen = internal::FieldHeaderSupport::fieldActualLength(innerFh, fieldLen, input, idx);
                    if (!jumpLen) {
                        return std::nullopt;
                    }
                    idx += *jumpLen;
                    remaining -= *jumpLen;
                    continue;
                }
                if (fieldLen > 0) {
                    res = iter->second->handle(innerFh, input.substr(idx, fieldLen), 0);
                } else {
                    if (innerFh.wireType == internal::ProtoWireType::LengthDelimited) {
                        res = iter->second->handle(innerFh, std::string_view {}, 0);
                    } else {
                        res = iter->second->handle(innerFh, input, idx);
                    }
                }
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
            } while (remaining > 0);
            return input.length()-start;
        }
    };
    template <class T>
    class ProtoDecoder<T, std::enable_if_t<
        bytedata_utils::ProtobufStyleSerializableChecker<T>::IsProtobufStyleSerializable()
        , void
    >> final : public IProtoDecoder<T> {
    public:
        ProtoDecoder(T* output, uint64_t baseFieldNumber) : IProtoDecoder<T>(output) {
        }
        virtual ~ProtoDecoder() {
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return {baseFieldNumber};
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            if (fh.wireType != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            std::string s(input.substr(start));
            if (output.ParseFromString(s)) {
                return s.length();
            } else {
                return std::nullopt;
            }
        }
    };

    template <class T, typename=void>
    class Proto {};

    template <class T>
    class Proto<T, std::enable_if_t<
        (
            ProtoWrappable<T>::value
        ), void
    >> {
    private:
        T t_;
        ProtoDecoder<T> dec_;
    public:
        Proto() : t_(), dec_(&t_, 1) {}
        Proto(T const &t) : t_(t), dec_(&t_, 1) {}
        Proto(T &&t) : t_(std::move(t)), dec_(&t_, 1) {}
        Proto(Proto const &p) : t_(p.t_), dec_(&t_, 1) {}
        Proto(Proto &&p) : t_(std::move(p.t_)), dec_(&t_, 1) {}
        Proto &operator=(Proto const &p) {
            if (this != &p) {
                t_ = p.t_;
            }
            return *this;
        }
        Proto &operator=(Proto &&p) {
            if (this != &p) {
                t_ = std::move(p.t_);
            }
            return *this;
        }
        ~Proto() = default;

        Proto &operator=(T const &t) {
            t_ = t;
            return *this;
        }
        Proto &operator=(T &&t) {
            t_ = std::move(t);
            return *this;
        }

        void SerializeToStream(std::ostream &os) const {
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                ProtoEncoder<T>::write(std::nullopt, t_, os, false);
            } else {
                ProtoEncoder<T>::write(1, t_, os, false);
            }
        }
        void SerializeToString(std::string *s) const {
            std::ostringstream oss;
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                ProtoEncoder<T>::write(std::nullopt, t_, oss, false);
            } else {
                ProtoEncoder<T>::write(1, t_, oss, false);
            }
            *s = oss.str();
        }
        bool ParseFromStringView(std::string_view const &s) {
            std::optional<std::size_t> res;
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                res = dec_.handle({internal::ProtoWireType::LengthDelimited, 0}, s, 0);
            } else {
                res = dec_.handleWithOuterHeader(s, 0);
            }
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
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                ProtoEncoder<T>::write(std::nullopt, t, os, false);
            } else {
                ProtoEncoder<T>::write(1, t, os, false);
            }
        }
        static bool runDeserialize(T &t, std::string_view const &input) {
            ProtoDecoder<T> dec(&t, 1);
            std::optional<std::size_t> res;
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                res = dec.handle({internal::ProtoWireType::LengthDelimited, 0}, input, 0);
            } else {
                res = dec.handleWithOuterHeader(input, 0);
            }
            if (res) {
                return true;
            } else {
                return false;
            }
        }
        static std::string runSerializeIntoValue(T const &t) {
            std::ostringstream oss;
            runSerialize(t, oss);
            return oss.str();
        }
    };
    template <class T>
    class Proto<T *, std::enable_if_t<
        (
            ProtoWrappable<T>::value
        ), void
    >> {
    private:
        T *t_;
        ProtoDecoder<T> dec_;
    public:
        Proto() : t_(nullptr), dec_(nullptr, 1) {}
        Proto(T *t) : t_(t), dec_(t_, 1) {}
        Proto(Proto const &p) : t_(p.t_), dec_(t_, 1) {}
        Proto(Proto &&p) : t_(p.t_), dec_(t_, 1) {}
        Proto &operator=(Proto const &p) = delete;
        Proto &operator=(Proto &&p) = delete;
        ~Proto() = default;

        void SerializeToStream(std::ostream &os) const {
            if (t_) {
                if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                    ProtoEncoder<T>::write(std::nullopt, *t_, os, false);
                } else {
                    ProtoEncoder<T>::write(1, *t_, os, false);
                }
            }
        }
        void SerializeToString(std::string *s) const {
            if (t_) {
                std::ostringstream oss;
                if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                    ProtoEncoder<T>::write(std::nullopt, *t_, oss, false);
                } else {
                    ProtoEncoder<T>::write(1, *t_, oss, false);
                }
                *s = oss.str();
            } else {
                *s = "";
            }
        }
        bool ParseFromStringView(std::string_view const &s) {
            if (t_) {
                std::optional<std::size_t> res;
                if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                    res = dec_.handle({internal::ProtoWireType::LengthDelimited, 0}, s, 0);
                } else {
                    res = dec_.handleWithOuterHeader(s, 0);
                }
                if (res) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        bool ParseFromString(std::string const &s) {
            return ParseFromStringView(std::string_view(s));
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
    class Proto<T const *, std::enable_if_t<
        (
            ProtoWrappable<T>::value
        ), void
    >> {
    private:
        T const *t_;
    public:
        Proto() : t_(nullptr) {}
        Proto(T const *t) : t_(t) {}
        Proto(Proto const &p) : t_(p.t_) {}
        Proto(Proto &&p) : t_(p.t_) {}
        Proto &operator=(Proto const &p) = delete;
        Proto &operator=(Proto &&p) = delete;
        ~Proto() = default;

        void SerializeToStream(std::ostream &os) const {
            if (t_) {
                if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                    ProtoEncoder<T>::write(std::nullopt, *t_, os, false);
                } else {
                    ProtoEncoder<T>::write(1, *t_, os, false);
                }
            }
        }
        void SerializeToString(std::string *s) const {
            if (t_) {
                std::ostringstream oss;
                if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                    ProtoEncoder<T>::write(std::nullopt, *t_, oss, false);
                } else {
                    ProtoEncoder<T>::write(1, *t_, oss, false);
                }
                *s = oss.str();
            } else {
                *s = "";
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
    template <class T>
    class Proto<std::shared_ptr<const T>, std::enable_if_t<
        (
            ProtoWrappable<T>::value
        ), void
    >> {
    private:
        std::shared_ptr<const T> t_;
        T val_;
        ProtoDecoder<T> dec_;
    public:
        Proto() : t_(), val_(), dec_(&val_, 1) {}
        Proto(std::shared_ptr<const T> const &t) : t_(t), val_(), dec_(&val_, 1) {}
        Proto(std::shared_ptr<const T> &&t) : t_(std::move(t)), val_(), dec_(&val_, 1) {}
        Proto(Proto const &p) : t_(p.t_), val_(p.val_), dec_(&val_, 1) {}
        Proto(Proto &&p) : t_(std::move(p.t_)), val_(std::move(p.val_)), dec_(&val_, 1) {}
        Proto &operator=(Proto const &p) {
            if (this != &p) {
                t_ = p.t_;
                val_ = p.val_;
            }
            return *this;
        }
        Proto &operator=(Proto &&p) {
            if (this != &p) {
                t_ = std::move(p.t_);
                val_ = std::move(p.val_);
            }
            return *this;
        }
        ~Proto() = default;

        Proto &operator=(std::shared_ptr<const T> const &t) {
            t_ = t;
            return *this;
        }
        Proto &operator=(std::shared_ptr<const T> &&t) {
            t_ = std::move(t);
            return *this;
        }

        void SerializeToStream(std::ostream &os) const {
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                ProtoEncoder<T>::write(std::nullopt, *t_, os, false);
            } else {
                ProtoEncoder<T>::write(1, *t_, os, false);
            }
        }
        void SerializeToString(std::string *s) const {
            std::ostringstream oss;
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                ProtoEncoder<T>::write(std::nullopt, *t_, oss, false);
            } else {
                ProtoEncoder<T>::write(1, *t_, oss, false);
            }
            *s = oss.str();
        }
        bool ParseFromStringView(std::string_view const &s) {
            t_.reset();
            std::optional<std::size_t> res;
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                res = dec_.handle({internal::ProtoWireType::LengthDelimited, 0}, s, 0);
            } else {
                res = dec_.handleWithOuterHeader(s, 0);
            }
            if (res) {
                auto x = std::make_shared<T>(std::move(val_));
                t_ = std::const_pointer_cast<const T>(x);
                return true;
            } else {
                return false;
            }
        }
        bool ParseFromString(std::string const &s) {
            return ParseFromStringView(std::string_view(s));
        }
        std::shared_ptr<const T> const &value() const {
            return t_;
        }
        std::shared_ptr<const T> const &operator*() const {
            return t_;
        }
        std::shared_ptr<const T> &&moveValue() && {
            return std::move(t_);
        }
        static void runSerialize(std::shared_ptr<const T> const &t, std::ostream &os) {
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                ProtoEncoder<T>::write(std::nullopt, *t, os, false);
            } else {
                ProtoEncoder<T>::write(1, *t, os, false);
            }
        }
        static bool runDeserialize(std::shared_ptr<const T> &t, std::string_view const &input) {
            t.reset();
            auto x = std::make_shared<T>();
            ProtoDecoder<T> dec(*x, 1);
            std::optional<std::size_t> res;
            if constexpr (StructFieldInfo<T>::HasGeneratedStructFieldInfo) {
                res = dec.handle({internal::ProtoWireType::LengthDelimited, 0}, input, 0);
            } else {
                res = dec.handleWithOuterHeader(input, 0);
            }
            if (res) {
                t = std::const_pointer_cast<const T>(x);
                return true;
            } else {
                return false;
            }
        }
        static std::string runSerializeIntoValue(std::shared_ptr<const T> const &t) {
            std::ostringstream oss;
            runSerialize(t, oss);
            return oss.str();
        }
    }; 

    template <class T>
    struct ProtoWrappable<T, std::enable_if_t<!IgnoreProxiesForProtoInterop<T>::value && (!EncodableThroughMultipleProxies<T>::value || !ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value) && !EncodableThroughProxy<T>::value && !std::is_enum_v<T> && ConvertibleWithString<T>::value, void>> {
        static constexpr bool value = true;
    };
    template <class T>
    class ProtoEncoder<T, std::enable_if_t<!IgnoreProxiesForProtoInterop<T>::value && (!EncodableThroughMultipleProxies<T>::value || !ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value) && !EncodableThroughProxy<T>::value && !std::is_enum_v<T> && ConvertibleWithString<T>::value, void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<std::string>::thisFieldNumber(inputFieldNumber);
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<std::string>::nextFieldNumber(inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, T const &data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<std::string>::write(fieldNumber, ConvertibleWithString<T>::toString(data), os, writeDefaultValue);
        }
    };
    template <class T>
    class ProtoDecoder<T, std::enable_if_t<!IgnoreProxiesForProtoInterop<T>::value && (!EncodableThroughMultipleProxies<T>::value || !ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value) && !EncodableThroughProxy<T>::value && !std::is_enum_v<T> && ConvertibleWithString<T>::value, void>> final : public IProtoDecoder<T> {
    private:
        std::string proxy_;
        ProtoDecoder<std::string> subDec_;
    public:
        ProtoDecoder(T* output, uint64_t baseFieldNumber) : IProtoDecoder<T>(output), proxy_(), subDec_(&proxy_, baseFieldNumber) {
        }
        virtual ~ProtoDecoder() {
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return ProtoDecoder<std::string>::responsibleForFieldNumbers(baseFieldNumber);
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            auto ret =  subDec_.handle(fh, input, start);
            if (!ret) {
                return std::nullopt;
            }
            output = ConvertibleWithString<T>::fromString(proxy_);
            return ret;
        }
    };

    template <class T>
    struct ProtoWrappable<T, std::enable_if_t<!IgnoreProxiesForProtoInterop<T>::value && (!EncodableThroughMultipleProxies<T>::value || !ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value) && EncodableThroughProxy<T>::value && ProtoWrappable<typename EncodableThroughProxy<T>::EncodeProxyType>::value && ProtoWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value, void>> {
        static constexpr bool value = true;
    };
    template <class T>
    class ProtoEncoder<T, std::enable_if_t<!IgnoreProxiesForProtoInterop<T>::value && (!EncodableThroughMultipleProxies<T>::value || !ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value) && EncodableThroughProxy<T>::value && ProtoWrappable<typename EncodableThroughProxy<T>::EncodeProxyType>::value, void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<typename EncodableThroughProxy<T>::EncodeProxyType>::thisFieldNumber(inputFieldNumber);
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<typename EncodableThroughProxy<T>::EncodeProxyType>::nextFieldNumber(inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, T const &data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<typename EncodableThroughProxy<T>::EncodeProxyType>::write(fieldNumber, EncodableThroughProxy<T>::toProxy(data), os, writeDefaultValue);
        }
    };
    template <class T>
    class ProtoDecoder<T, std::enable_if_t<!IgnoreProxiesForProtoInterop<T>::value && (!EncodableThroughMultipleProxies<T>::value || !ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value) && EncodableThroughProxy<T>::value && ProtoWrappable<typename EncodableThroughProxy<T>::DecodeProxyType>::value, void>> final : public IProtoDecoder<T> {
    private:
        typename EncodableThroughProxy<T>::DecodeProxyType proxy_;
        ProtoDecoder<typename EncodableThroughProxy<T>::DecodeProxyType> subDec_;
        std::string proxy1_;
        ProtoDecoder<std::string> subDec1_;
    public:
        ProtoDecoder(T* output, uint64_t baseFieldNumber) : IProtoDecoder<T>(output), proxy_(), subDec_(&proxy_, baseFieldNumber), subDec1_(&proxy1_, baseFieldNumber) {
        }
        virtual ~ProtoDecoder() {
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            return ProtoDecoder<typename EncodableThroughProxy<T>::DecodeProxyType>::responsibleForFieldNumbers(baseFieldNumber);
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            auto ret =  subDec_.handle(fh, input, start);
            if (!ret) {                
                if constexpr (ConvertibleWithString<T>::value) {
                    auto ret =  subDec1_.handle(fh, input, start);
                    if (!ret) {
                        return std::nullopt;
                    }
                    output = ConvertibleWithString<T>::fromString(proxy1_);
                    return ret;
                } else {
                    return std::nullopt;
                }
            }
            output = EncodableThroughProxy<T>::fromProxy(std::move(proxy_));
            return ret;
        }
    };

    template <class T>
    struct ProtoWrappable<T, std::enable_if_t<!std::is_enum_v<T> && !IgnoreProxiesForProtoInterop<T>::value && EncodableThroughMultipleProxies<T>::value && ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value, void>> {
        static constexpr bool value = true;
    };
    template <class T>
    class ProtoEncoder<T, std::enable_if_t<!std::is_enum_v<T> && !IgnoreProxiesForProtoInterop<T>::value && EncodableThroughMultipleProxies<T>::value && ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value, void>> {
    public:
        static constexpr uint64_t thisFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::thisFieldNumber(inputFieldNumber);
        }
        static constexpr uint64_t nextFieldNumber(uint64_t inputFieldNumber) {
            return ProtoEncoder<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::nextFieldNumber(inputFieldNumber);
        }
        static void write(std::optional<uint64_t> fieldNumber, T const &data, std::ostream &os, bool writeDefaultValue) {
            ProtoEncoder<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::write(fieldNumber, EncodableThroughMultipleProxies<T>::toProtoEncodeProxy(data), os, writeDefaultValue);
        }
    };
    template <class T>
    class ProtoDecoder<T, std::enable_if_t<!std::is_enum_v<T> && !IgnoreProxiesForProtoInterop<T>::value && EncodableThroughMultipleProxies<T>::value && ProtoWrappable<typename EncodableThroughMultipleProxies<T>::ProtoEncodeProxyType>::value, void>> final : public IProtoDecoder<T> {
    private:
        using P = typename EncodableThroughMultipleProxies<T>::DecodeProxyTypes;
        using PT = dev::cd606::tm::infra::TupleVariantConversion<P>;
        PT storage_;
        std::array<IProtoDecoderBase *, std::variant_size_v<P>> decoders_;
        std::string proxy1_;
        ProtoDecoder<std::string> subDec1_;

        template <std::size_t FieldIndex>
        static void addResponsibleFields_internal(uint64_t current, std::set<uint64_t> &output) {
            if constexpr (FieldIndex >= 0 && FieldIndex < std::variant_size_v<P>) {
                if constexpr (ProtoWrappable<std::variant_alternative_t<FieldIndex, P>>::value) {
                    auto v = ProtoDecoder<
                        std::variant_alternative_t<FieldIndex, P>
                    >::responsibleForFieldNumbers(
                        ProtoEncoder<
                            std::variant_alternative_t<FieldIndex, P>
                        >::thisFieldNumber(current)
                    );
                    std::copy(v.begin(), v.end(), std::inserter(output, output.begin()));
                }
                addResponsibleFields_internal<FieldIndex+1>(
                    current
                    , output
                );
            }
        }
        template <std::size_t FieldIndex>
        void buildDecoders_internal(uint64_t fieldNumber) {
            if constexpr (FieldIndex >= 0 && FieldIndex < std::variant_size_v<P>) {
                using F = std::variant_alternative_t<FieldIndex, P>;
                if constexpr (ProtoWrappable<F>::value) {
                    decoders_[FieldIndex] = new ProtoDecoder<F>(
                        &(std::get<FieldIndex>(storage_))
                        , fieldNumber
                    );
                } else {
                    decoders_[FieldIndex] = nullptr;
                }
                buildDecoders_internal<FieldIndex+1>(
                    fieldNumber
                );
            }
        }
        template <std::size_t FieldIndex>
        std::optional<std::size_t> readHelper(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) {
            if constexpr (FieldIndex >= 0 && FieldIndex < std::variant_size_v<P>) {
                using F = std::variant_alternative_t<FieldIndex, P>;
                if constexpr (ProtoWrappable<F>::value) {
                    auto ret = decoders_[FieldIndex]->handle(
                        fh, input, start
                    );
                    if (ret) {
                        output = EncodableThroughMultipleProxies<T>::fromProxy(P {std::move(
                            std::get<FieldIndex>(storage_)
                        )});
                        return ret;
                    } else {
                        return readHelper<FieldIndex+1>(output, fh, input, start);
                    }
                } else {
                    return readHelper<FieldIndex+1>(output, fh, input, start);
                }
            } else {
                return std::nullopt;
            }
        }
    public:
        ProtoDecoder(T* output, uint64_t baseFieldNumber) : IProtoDecoder<T>(output), storage_(), decoders_(), proxy1_(), subDec1_(&proxy1_, baseFieldNumber) {
            buildDecoders_internal<0>(baseFieldNumber);
        }
        virtual ~ProtoDecoder() {
            for (auto *d : decoders_) {
                if (d) {
                    delete d;
                }
            }
        }
        static std::vector<uint64_t> responsibleForFieldNumbers(uint64_t baseFieldNumber) {
            std::set<uint64_t> v;
            addResponsibleFields_internal<0>(baseFieldNumber, v);
            return std::vector<uint64_t> {v.begin(), v.end()};
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::FieldHeader const &fh, std::string_view const &input, std::size_t start) override final {
            auto ret = readHelper<0>(output, fh, input, start);
            if (ret) {
                return ret;
            }          
            if constexpr (ConvertibleWithString<T>::value) {
                auto ret =  subDec1_.handle(fh, input, start);
                if (!ret) {
                    return std::nullopt;
                }
                output = ConvertibleWithString<T>::fromString(proxy1_);
                return ret;
            } else {
                return std::nullopt;
            }
        }
    };

} } } } }

#endif
