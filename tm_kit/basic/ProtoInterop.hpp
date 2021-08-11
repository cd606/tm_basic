#ifndef TM_KIT_BASIC_PROTO_INTEROP_HPP_
#define TM_KIT_BASIC_PROTO_INTEROP_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>

#include <iostream>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace proto_interop {

    namespace internal {
        class VarIntSupport {
        public:
            template <class IntType, typename=std::enable_if_t<std::is_integral_v<IntType> && std::is_unsigned_v<IntType>>>
            static void write(IntType data, std::ostream &os) {
                IntType remaining = data;
                while (remaining >= (uint8_t) 0x80) {
                    os << (((uint8_t) (remaining & (IntType) 0x7f)) | (uint8_t) 0x80);
                    remaining = remaining >> 8;
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
                    shift += 8;
                    if (shift >= sizeof(IntType)*8) {
                        return std::nullopt;
                    };
                }
                return std::nullopt;
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
                if (input & 0x1 == 0) {
                    return (int32_t) (input >> 1);
                } else {
                    return (int32_t) (((~input) >> 1) | (uint32_t) 0x8FFF);
                }
            }
        };
        template <>
        class ZigZagIntSupport<int64_t> {
        public:
            static uint64_t encode(int64_t input) {
                return (uint64_t) ((input << 1) ^ (input >> 63));
            }
            static int64_t decode(uint64_t input) {
                if (input & 0x1 == 0) {
                    return (int64_t) (input >> 1);
                } else {
                    return (int32_t) (((~input) >> 1) | (uint32_t) 0x8FFFFFFF);
                }
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
            static std::optional<std::size_t> readHeader(FieldHeader &fh, std::string_view const &input, std::size_t start) {
                uint64_t x;
                auto res = VarIntSupport::read<uint64_t>(x, input, start);
                if (!res) {
                    std::cerr << "Bad int\n";
                    return std::nullopt;
                }
                uint8_t wt = (uint8_t) (x & (uint64_t) 0x7);
                if (wt > (uint8_t) ProtoWireType::Fixed32) {
                    std::cerr << "Bad type " << (int) wt << "\n";
                    return std::nullopt;                    
                }
                fh.wireType = (ProtoWireType) wt;
                fh.fieldNumber = (x >> 3);
                return res;
            }
        };
    }

    template <class T, typename Enable=void>
    class ProtoEncoder {};

    template <class IntType>
    class ProtoEncoder<IntType, std::enable_if_t<
        (std::is_integral_v<IntType> && std::is_unsigned_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType>)
        , void
    >> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, IntType data, std::ostream &os) {
            if (data == 0) {
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
    template <>
    class ProtoEncoder<bool, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, bool data, std::ostream &os) {
            if (!data) {
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
    class ProtoEncoder<int8_t, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, int8_t data, std::ostream &os) {
            if (data == 0) {
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
    class ProtoEncoder<int16_t, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, int16_t data, std::ostream &os) {
            if (data == 0) {
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
    class ProtoEncoder<int32_t, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, int32_t data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint32_t>(internal::ZigZagIntSupport<int32_t>::encode(data), os);
        }
    };
    template <>
    class ProtoEncoder<int64_t, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, int64_t data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::VarInt, *fieldNumber}
                    , os
                );
            }
            internal::VarIntSupport::write<uint64_t>(internal::ZigZagIntSupport<int64_t>::encode(data), os);
        }
    };

    template <class T>
    class ProtoEncoder<T, std::enable_if_t<std::is_enum_v<T>, void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, T data, std::ostream &os) {
            ProtoEncoder<std::underlying_type_t<T>>::write(
                fieldNumber, static_cast<std::underlying_type_t<T>>(data), os
            );
        }
    };

    template <>
    class ProtoEncoder<float, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, float data, std::ostream &os) {
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
    class ProtoEncoder<double, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, double data, std::ostream &os) {
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
    class ProtoEncoder<std::string, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::string const &data, std::ostream &os) {
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
    class ProtoEncoder<std::string_view, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::string_view const &data, std::ostream &os) {
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
    class ProtoEncoder<ByteData, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, ByteData const &data, std::ostream &os) {
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
    class ProtoEncoder<ByteDataView, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, ByteDataView const &data, std::ostream &os) {
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

    template <class T>
    class ProtoEncoder<std::vector<T>, std::enable_if_t<(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::vector<T> const &data, std::ostream &os) {
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
                ProtoEncoder<T>::write(std::nullopt, item, ss);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T>
    class ProtoEncoder<std::vector<T>, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::vector<T> const &data, std::ostream &os) {
            for (auto const &item : data) {
                ProtoEncoder<T>::write(fieldNumber, item, os);
            }
        }
    };
    template <class T>
    class ProtoEncoder<std::list<T>, std::enable_if_t<(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::list<T> const &data, std::ostream &os) {
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
                ProtoEncoder<T>::write(std::nullopt, item, ss);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T>
    class ProtoEncoder<std::list<T>, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::list<T> const &data, std::ostream &os) {
            for (auto const &item : data) {
                ProtoEncoder<T>::write(fieldNumber, item, os);
            }
        }
    };
    template <class T, std::size_t N>
    class ProtoEncoder<std::array<T,N>, std::enable_if_t<(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::array<T,N> const &data, std::ostream &os) {
            if (fieldNumber) {
                internal::FieldHeaderSupport::writeHeader(
                    internal::FieldHeader {internal::ProtoWireType::LengthDelimited, *fieldNumber}
                    , os
                );
            }
            std::ostringstream ss;
            for (auto const &item : data) {
                ProtoEncoder<T>::write(std::nullopt, item, ss);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T, std::size_t N>
    class ProtoEncoder<std::array<T,N>, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::array<T,N> const &data, std::ostream &os) {
            for (auto const &item : data) {
                ProtoEncoder<T>::write(fieldNumber, item, os);
            }
        }
    };
    template <class T>
    class ProtoEncoder<std::valarray<T>, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::valarray<T> const &data, std::ostream &os) {
            if (data.size() == 0) {
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
                ProtoEncoder<T>::write(std::nullopt, item, ss);
            }
            auto cont = ss.str();
            internal::VarIntSupport::write<uint64_t>(cont.length(), os);
            os.write(cont.data(), cont.length());
        }
    };
    template <class T>
    class ProtoEncoder<std::optional<T>, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, std::optional<T> const &data, std::ostream &os) {
            if (data) {
                ProtoEncoder<T>::write(fieldNumber, *data, os);
            }
        }
    };
    template <>
    class ProtoEncoder<VoidStruct, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, VoidStruct const &data, std::ostream &os) {
        }
    };
    template <int32_t N>
    class ProtoEncoder<ConstType<N>, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, ConstType<N> const &data, std::ostream &os) {
        }
    };
    template <int32_t N, class T>
    class ProtoEncoder<SingleLayerWrapperWithID<N,T>, void> {
    public:
        static void write(std::optional<uint64_t> fieldNumber, SingleLayerWrapperWithID<N,T> const &data, std::ostream &os) {
            std::optional<uint64_t> f = std::nullopt;
            if (fieldNumber) {
                f = (uint64_t) N;
            }
            ProtoEncoder<T>::write(f, data.value, os);
        }
    };

    template <class T>
    class ProtoEncoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
    private:
        template <std::size_t FieldCount, std::size_t FieldIndex>
        static void buildIndices_impl(std::array<uint64_t,FieldCount> &ret, std::size_t current) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (IsSingleLayerWrapperWithID<T>::Value) {
                    ret[FieldIndex] = (std::size_t) IsSingleLayerWrapperWithID<T>::ID;
                    buildIndices_impl<FieldCount,FieldIndex+1>(ret, (std::size_t) IsSingleLayerWrapperWithID<T>::ID+1);
                } else {
                    ret[FieldIndex] = current;
                    buildIndices_impl<FieldCount,FieldIndex+1>(ret, current+1);
                }
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
                ProtoEncoder<F>::write(indices[FieldIndex], data.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()), os);
                write_impl<FieldCount,FieldIndex+1>(data, os, indices);
            }
        }
    public:
        static void write(std::optional<uint64_t> fieldNumber, T const &data, std::ostream &os) {
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
    
    class IProtoDecoderBase {
    public:
        virtual ~IProtoDecoderBase() = default;
        virtual std::optional<std::size_t> handle(internal::ProtoWireType wt, std::string_view const &input, std::size_t start) = 0;
    };
    template <class T>
    class IProtoDecoder : public IProtoDecoderBase {
    private:
        T *output_;
    public:
        IProtoDecoder(T *output) : output_(output) {}
        virtual ~IProtoDecoder() = default;
    protected:
        virtual std::optional<std::size_t> read(T &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) = 0;
    public:
        std::optional<std::size_t> handle(internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            return read(*output_, wt, input, start);
        }
    };

    template <class T, typename=void>
    class ProtoDecoder {};

    template <class IntType>
    class ProtoDecoder<IntType, std::enable_if_t<(std::is_integral_v<IntType> && std::is_unsigned_v<IntType> && !std::is_same_v<IntType,bool> && !std::is_enum_v<IntType>), void>> final : public IProtoDecoder<IntType> {
    public:
        ProtoDecoder(IntType *output) : IProtoDecoder<IntType>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(IntType &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            switch (wt) {
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
    class ProtoDecoder<bool, void> final : public IProtoDecoder<bool> {
    public:
        ProtoDecoder(bool *output) : IProtoDecoder<bool>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(bool &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::VarInt) {
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
    class ProtoDecoder<int8_t, void> final : public IProtoDecoder<int8_t> {
    public:
        ProtoDecoder(int8_t *output) : IProtoDecoder<int8_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int8_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::VarInt) {
                return std::nullopt;
            }
            uint8_t x;
            auto res = internal::VarIntSupport::read<uint8_t>(x, input, start);
            if (res) {
                output = (int8_t) x;
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<int16_t, void> final : public IProtoDecoder<int16_t> {
    public:
        ProtoDecoder(int16_t *output) : IProtoDecoder<int16_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int16_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::VarInt) {
                return std::nullopt;
            }
            uint16_t x;
            auto res = internal::VarIntSupport::read<uint16_t>(x, input, start);
            if (res) {
                output = (int16_t) x;
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<int32_t, void> final : public IProtoDecoder<int32_t> {
    public:
        ProtoDecoder(int32_t *output) : IProtoDecoder<int32_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int32_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            switch (wt) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = internal::VarIntSupport::read<uint32_t>(x, input, start);
                    if (res) {
                        output = internal::ZigZagIntSupport<int32_t>::decode(x);
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
        ProtoDecoder(int64_t *output) : IProtoDecoder<int64_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int64_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            switch (wt) {
            case internal::ProtoWireType::VarInt:
                {
                    uint64_t x;
                    auto res = internal::VarIntSupport::read<uint64_t>(x, input, start);
                    if (res) {
                        output = internal::ZigZagIntSupport<int64_t>::decode(x);
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

    template <class T>
    class ProtoDecoder<T, std::enable_if_t<std::is_enum_v<T>, void>> final : public IProtoDecoder<T> {
    public:
        ProtoDecoder(T *output) : IProtoDecoder<T>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(T &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            std::underlying_type_t<T> x;
            ProtoDecoder<std::underlying_type_t<T>> dec(&x);
            auto res = dec.handle(wt, input, start);
            if (res) {
                output = (T) x;
            }
            return res;
        }
    };

    template <>
    class ProtoDecoder<float, void> final : public IProtoDecoder<float> {
    public:
        ProtoDecoder(float *output) : IProtoDecoder<float>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(float &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::Fixed32) {
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
        ProtoDecoder(double *output) : IProtoDecoder<double>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(double &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::Fixed64) {
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
        ProtoDecoder(std::string *output) : IProtoDecoder<std::string>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::string &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            uint64_t len;
            auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
            if (!lenRes) {
                return std::nullopt;
            }
            if (start+*lenRes+len > input.length()) {
                return std::nullopt;
            }
            output = std::string(input.substr(start+*lenRes, len));
            return (*lenRes+len);
        }
    };
    template <>
    class ProtoDecoder<ByteData, void> final : public IProtoDecoder<ByteData> {
    public:
        ProtoDecoder(ByteData *output) : IProtoDecoder<ByteData>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(ByteData &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            uint64_t len;
            auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
            if (!lenRes) {
                return std::nullopt;
            }
            if (start+*lenRes+len > input.length()) {
                return std::nullopt;
            }
            output = { std::string(input.substr(start+*lenRes, len)) };
            return (*lenRes+len);
        }
    };

    template <class T>
    class ProtoDecoder<std::vector<T>, std::enable_if_t<(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> final : public IProtoDecoder<std::vector<T>> {
    public:
        ProtoDecoder(std::vector<T> *output) : IProtoDecoder<std::vector<T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::vector<T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt == internal::ProtoWireType::LengthDelimited) {
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
                ProtoDecoder<T> subDec(&x);
                std::size_t remaining = len;
                std::size_t idx = 0;
                while (remaining > 0) {
                    auto res = subDec.handle(wt, body, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    output.push_back(x);
                    idx += *res;
                    remaining -= *res;
                }
                return *lenRes+len;
            } else if (wt == internal::ProtoWireType::VarInt) {
                T x;
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, input, start);
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
    class ProtoDecoder<std::vector<T>, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> final : public IProtoDecoder<std::vector<T>> {
    public:
        ProtoDecoder(std::vector<T> *output) : IProtoDecoder<std::vector<T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::vector<T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            uint64_t len;
            auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
            if (!lenRes) {
                return std::nullopt;
            }
            if (start+*lenRes+len > input.length()) {
                return std::nullopt;
            }
            std::string_view body = input.substr(start+*lenRes, len);
            std::size_t remaining = len;
            std::size_t idx = 0;
            T x;
            while (remaining > 0) {
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, body, idx);
                if (!res) {
                    return std::nullopt;
                }
                output.push_back(std::move(x));
                idx += *res;
                remaining -= *res;
            }
            return *lenRes+len;
        }
    };
    template <class T>
    class ProtoDecoder<std::list<T>, std::enable_if_t<(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> final : public IProtoDecoder<std::list<T>> {
    public:
        ProtoDecoder(std::list<T> *output) : IProtoDecoder<std::list<T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::list<T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt == internal::ProtoWireType::LengthDelimited) {
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
                ProtoDecoder<T> subDec(&x);
                std::size_t remaining = len;
                std::size_t idx = 0;
                while (remaining > 0) {
                    auto res = subDec.handle(wt, body, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    output.push_back(x);
                    idx += *res;
                    remaining -= *res;
                }
                return *lenRes+len;
            } else if (wt == internal::ProtoWireType::VarInt) {
                T x;
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, input, start);
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
    class ProtoDecoder<std::list<T>, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> final : public IProtoDecoder<std::list<T>> {
    public:
        ProtoDecoder(std::list<T> *output) : IProtoDecoder<std::list<T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::list<T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            uint64_t len;
            auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
            if (!lenRes) {
                return std::nullopt;
            }
            if (start+*lenRes+len > input.length()) {
                return std::nullopt;
            }
            std::string_view body = input.substr(start+*lenRes, len);
            std::size_t remaining = len;
            std::size_t idx = 0;
            T x;
            while (remaining > 0) {
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, body, idx);
                if (!res) {
                    return std::nullopt;
                }
                output.push_back(std::move(x));
                idx += *res;
                remaining -= *res;
            }
            return *lenRes+len;
        }
    };
    template <class T, std::size_t N>
    class ProtoDecoder<std::array<T, N>, std::enable_if_t<(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> final : public IProtoDecoder<std::array<T,N>> {
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
        ProtoDecoder(std::array<T, N> *output) : IProtoDecoder<std::array<T, N>>(output), curIdx_(0) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::array<T, N> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt == internal::ProtoWireType::LengthDelimited) {
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
                ProtoDecoder<T> subDec(&x);
                std::size_t remaining = len;
                std::size_t idx = 0;
                while (remaining > 0) {
                    auto res = subDec.handle(wt, body, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    addItem(output, x);
                    idx += *res;
                    remaining -= *res;
                }
                return *lenRes+len;
            } else if (wt == internal::ProtoWireType::VarInt) {
                T x;
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, input, start);
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
    class ProtoDecoder<std::array<T,N>, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>), void>> final : public IProtoDecoder<std::vector<T>> {
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
        ProtoDecoder(std::array<T,N> *output) : IProtoDecoder<std::array<T,N>>(output), curIdx_(0) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::array<T,N> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            uint64_t len;
            auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
            if (!lenRes) {
                return std::nullopt;
            }
            if (start+*lenRes+len > input.length()) {
                return std::nullopt;
            }
            std::string_view body = input.substr(start+*lenRes, len);
            std::size_t remaining = len;
            std::size_t idx = 0;
            T x;
            while (remaining > 0) {
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, body, idx);
                if (!res) {
                    return std::nullopt;
                }
                addItem(output, std::move(x));
                idx += *res;
                remaining -= *res;
            }
            return *lenRes+len;
        }
    };
    template <class T>
    class ProtoDecoder<std::valarray<T>, void> final : public IProtoDecoder<std::valarray<T>> {
    public:
        ProtoDecoder(std::valarray<T> *output) : IProtoDecoder<std::valarray<T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::valarray<T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt == internal::ProtoWireType::LengthDelimited) {
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
                ProtoDecoder<T> subDec(&x);
                std::size_t remaining = len;
                std::size_t idx = 0;
                while (remaining > 0) {
                    auto res = subDec.handle(wt, body, idx);
                    if (!res) {
                        return std::nullopt;
                    }
                    output.resize(output.size()+1);
                    output[output.size()-1] = x;
                    idx += *res;
                    remaining -= *res;
                }
                return *lenRes+len;
            } else if (wt == internal::ProtoWireType::VarInt) {
                T x;
                ProtoDecoder<T> subDec(&x);
                auto res = subDec.handle(wt, input, start);
                if (res) {
                    output.resize(output.size()+1);
                    output[output.size()-1] = x;
                }
                return res;
            } else {
                return std::nullopt;
            }
        }
    };
    template <class T>
    class ProtoDecoder<std::optional<T>, void> final : public IProtoDecoder<std::optional<T>> {
    public:
        ProtoDecoder(std::optional<T> *output) : IProtoDecoder<std::optional<T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(std::optional<T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            T x;
            ProtoDecoder<T> subDec(&x);
            auto res = subDec.handle(wt, input, start);
            if (res) {
                output = std::move(x);
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<VoidStruct, void> final : public IProtoDecoder<VoidStruct> {
    public:
        ProtoDecoder(VoidStruct *output) : IProtoDecoder<VoidStruct>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(VoidStruct &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            return 0;
        }
    };
    template <int32_t N>
    class ProtoDecoder<ConstType<N>, void> final : public IProtoDecoder<ConstType<N>> {
    public:
        ProtoDecoder(ConstType<N> *output) : IProtoDecoder<ConstType<N>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(ConstType<N> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            return 0;
        }
    };
    template <int32_t N, class T>
    class ProtoDecoder<SingleLayerWrapperWithID<N,T>, void> final : public IProtoDecoder<SingleLayerWrapperWithID<N,T>> {
    public:
        ProtoDecoder(SingleLayerWrapperWithID<N,T> *output) : IProtoDecoder<SingleLayerWrapperWithID<N,T>>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(SingleLayerWrapperWithID<N,T> &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            ProtoDecoder<T> subDec(&output.value);
            return subDec.handle(wt, input, start);
        }
    };

    template <class T>
    class ProtoDecoder<T, std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> final : public IProtoDecoder<T> {
    private:
        std::unordered_map<uint64_t, IProtoDecoderBase *> decoders_;

        template <std::size_t FieldCount, std::size_t FieldIndex>
        static constexpr void buildDecoders_impl(T *t, std::unordered_map<uint64_t, IProtoDecoderBase *> &ret, std::size_t current) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                using F = typename StructFieldTypeInfo<T,FieldIndex>::TheType;
                if constexpr (IsSingleLayerWrapperWithID<T>::Value) {
                    ret[(uint64_t) IsSingleLayerWrapperWithID<T>::ID] = new ProtoDecoder<F>(
                        &(t->*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()))
                    );
                    buildDecoders_impl<FieldCount,FieldIndex+1>(t, ret, (std::size_t) IsSingleLayerWrapperWithID<T>::ID+1);
                } else {
                    ret[(uint64_t) current] = new ProtoDecoder<F>(
                        &(t->*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()))
                    );
                    buildDecoders_impl<FieldCount,FieldIndex+1>(t, ret, current+1);
                }
            }
        }
    public:
        ProtoDecoder(T* output) : IProtoDecoder<T>(output), decoders_() {
            buildDecoders_impl<StructFieldInfo<T>::FIELD_NAMES.size(),0>(output, decoders_, 1);
        }
        virtual ~ProtoDecoder() {
            for (auto &item : decoders_) {
                delete item.second;
                item.second = nullptr;
            }
            decoders_.clear();
        }
    protected:
        std::optional<std::size_t> read(T &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(output);
            if (wt != internal::ProtoWireType::LengthDelimited) {
                return std::nullopt;
            }
            uint64_t len;
            auto lenRes = internal::VarIntSupport::read<uint64_t>(len, input, start);
            if (!lenRes) {
                return std::nullopt;
            }
            if (start+*lenRes+len > input.length()) {
                return std::nullopt;
            }
            std::string_view body = input.substr(start+*lenRes, len);
            std::size_t remaining = len;
            std::size_t idx = 0;
            while (remaining > 0) {
                internal::FieldHeader fh;
                auto res = internal::FieldHeaderSupport::readHeader(fh, body, idx);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
                auto iter = decoders_.find(fh.fieldNumber);
                if (iter == decoders_.end()) {
                    continue;
                }
                if (iter->second == nullptr) {
                    continue;
                }
                res = iter->second->handle(fh.wireType, body, idx);
                if (!res) {
                    return std::nullopt;
                }
                idx += *res;
                remaining -= *res;
            }
            return *lenRes+len;
        }
    };
    

} } } } }

#endif
