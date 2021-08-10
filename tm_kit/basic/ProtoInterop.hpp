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
                OS << (uint8_t) remaining;
            }
            template <class IntType, typename=std::enable_if_t<std::is_integral_v<IntType> && std::is_unsigned_v<IntType>>>
            static std::optional<std::size_t> read(IntType &data, std::string_view const &input, std::size_t startPos) {
                std::size_t currentPos = startPos;
                data = 0;
                std::size_t shift = 0;
                while (currentPos < input.length()) {
                    uint8_t b = (uint8_t) (input[currentPos]);
                    bool hasMore = ((b & (uint8_t) 0x80) != 0);
                    data = data | (((IntType) (b & (uint8_t) 0x7f)) << shift);
                    if (!hasMore) {
                        return (currentPos-startPos+1);
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
                    return -((int32_t) (input >> 1));
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
                    return -((int64_t) (input >> 1));
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
        }

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
            static std::optional<std::size_t> readHeader(FieldHeader &fh, std::string_view const &input, std::size_t startPos) {
                uint64_t x;
                auto res = VarIntSupport::read<uint64_t>(x, input, startPos);
                if (!res) {
                    return std::nullopt;
                }
                uint8_t wt = (uint8_t) (x & (uint64_t) 0x7);
                if (wt > (uint8_t) ProtoWireType::Fixed32) {
                    return std::nullopt;                    
                }
                fh.wireType = (ProtoWireType) wt;
                fh.fieldNumber = (x >> 3);
                return res;
            }
        };
    }

    template <class T>
    class ProtoEncoder {};

    template <class IntType, typename=std::enable_if_t<std::is_integral_v<IntType> && std::is_unsigned_v<IntType>>>
    class ProtoEncoder<IntType> {
    public:
        static void write(uint64_t fieldNumber, IntType data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::VarInt, fieldNumber}
                , os;
            );
            internal::VarIntSupport<IntType>::write(data, os);
        }
    };
    template <>
    class ProtoEncoder<bool> {
    public:
        static void write(uint64_t fieldNumber, bool data, std::ostream &os) {
            if (!data) {
                return;
            }
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::VarInt, fieldNumber}
                , os;
            );
            internal::VarIntSupport<uint8_t>::write((uint8_t) data, os);
        }
    };
    template <>
    class ProtoEncoder<int8_t> {
    public:
        static void write(uint64_t fieldNumber, int8_t data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::VarInt, fieldNumber}
                , os;
            );
            internal::VarIntSupport<uint8_t>::write((uint8_t) data, os);
        }
    };
    template <>
    class ProtoEncoder<int16_t> {
    public:
        static void write(uint64_t fieldNumber, int16_t data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::VarInt, fieldNumber}
                , os;
            )
            internal::VarIntSupport<uint16_t>::write((uint16_t) data, os);
        }
    };
    template <>
    class ProtoEncoder<int32_t> {
    public:
        static void write(uint64_t fieldNumber, int32_t data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::VarInt, fieldNumber}
                , os;
            );
            internal::VarIntSupport<uint32_t>::write(ZigZagIntSupport<int32_t>::encode(data), os);
        }
    };
    template <>
    class ProtoEncoder<int64_t> {
    public:
        static void write(uint64_t fieldNumber, int64_t data, std::ostream &os) {
            if (data == 0) {
                return;
            }
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::VarInt, fieldNumber}
                , os;
            );
            internal::VarIntSupport<uint64_t>::write(ZigZagIntSupport<int64_t>::encode(data), os);
        }
    };

    template <class T, typename=std::enable_if_t<std::is_enum_v<T>>>
    class ProtoEncoder<T> {
    public:
        static void write(uint64_t fieldNumber, T data, std::ostream &os) {
            ProtoEncoder<std::underlying_type_t<T>>::write(
                fieldNumber, static_cast<std::underlying_type_t<T>>(data), os
            );
        }
    };

    template <>
    class ProtoEncoder<float> {
    public:
        static void write(uint64_t fieldNumber, float data, std::ostream &os) {
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::Fixed32, fieldNumber}
                , os;
            );
            uint32_t dBuf;
            std::memcpy(&dBuf, &data, 4);
            boost::endian::native_to_small_inplace<uint32_t>(dBuf);
            os.write(reinterpret_cast<char *>(&dBuf), 4);
        }
    };
    template <>
    class ProtoEncoder<double> {
    public:
        static void write(uint64_t fieldNumber, double data, std::ostream &os) {
            internal::FieldHeaderSupport::writeHeader(
                internal::FieldHeader {internal::ProtoWireType::Fixed64, fieldNumber}
                , os;
            );
            uint64_t dBuf;
            std::memcpy(&dBuf, &data, 8);
            boost::endian::native_to_small_inplace<uint64_t>(dBuf);
            os.write(reinterpret_cast<char *>(&dBuf), 8);
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
            return read(*output_, wt, input, startPos);
        }
    };

    template <class T>
    class ProtoDecoder {};

    template <class IntType, typename=std::enable_if_t<std::is_integral_v<IntType> && std::is_unsigned_v<IntType>>>
    class ProtoDecoder<IntType> final : public IProtoDecoder<IntType> {
    public:
        ProtoDecoder(IntType *output) : IProtoDecoder<IntType>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(T &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            switch (wt) {
            case internal::ProtoWireType::VarInt:
                return VarIntSupport<IntType>::read(output, input, startPos);
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (startPos+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x. input.data()+startPos, sizeof(uint32_t));
                    boost::endian::small_to_native_inplace<uint32_t>(x);
                    output = (IntType) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (startPos+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x. input.data()+startPos, sizeof(uint64_t));
                    boost::endian::small_to_native_inplace<uint64_t>(x);
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
    class ProtoDecoder<bool> final : public IProtoDecoder<bool> {
    public:
        ProtoDecoder(bool *output) : IProtoDecoder<bool>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(bool &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::VarInt) {
                return std::nullopt;
            }
            uint8_t x;
            auto res = VarIntSupport<uint8_t>::read(x, input, startPos);
            if (res) {
                output = (bool) x;
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<int8_t> final : public IProtoDecoder<int8_t> {
    public:
        ProtoDecoder(int8_t *output) : IProtoDecoder<int8_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int8_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::VarInt) {
                return std::nullopt;
            }
            uint8_t x;
            auto res = VarIntSupport<uint8_t>::read(x, input, startPos);
            if (res) {
                output = (int8_t) x;
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<int16_t> final : public IProtoDecoder<int16_t> {
    public:
        ProtoDecoder(int16_t *output) : IProtoDecoder<int16_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int16_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::VarInt) {
                return std::nullopt;
            }
            uint16_t x;
            auto res = VarIntSupport<uint16_t>::read(x, input, startPos);
            if (res) {
                output = (int16_t) x;
            }
            return res;
        }
    };
    template <>
    class ProtoDecoder<int32_t> final : public IProtoDecoder<int32_t> {
    public:
        ProtoDecoder(int32_t *output) : IProtoDecoder<int32_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int32_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            switch (wt) {
            case internal::ProtoWireType::VarInt:
                {
                    uint32_t x;
                    auto res = VarIntSupport<uint32_t>::read(x, input, startPos);
                    if (res) {
                        output = ZigZagIntSupport<int32_t>::decode(x);
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (startPos+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x. input.data()+startPos, sizeof(uint32_t));
                    boost::endian::small_to_native_inplace<uint32_t>(x);
                    output = (int32_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (startPos+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x. input.data()+startPos, sizeof(uint64_t));
                    boost::endian::small_to_native_inplace<uint64_t>(x);
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
    class ProtoDecoder<int64_t> final : public IProtoDecoder<int64_t> {
    public:
        ProtoDecoder(int64_t *output) : IProtoDecoder<int64_t>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(int64_t &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            switch (wt) {
            case internal::ProtoWireType::VarInt:
                {
                    uint64_t x;
                    auto res = VarIntSupport<uint64_t>::read(x, input, startPos);
                    if (res) {
                        output = ZigZagIntSupport<int64_t>::decode(x);
                    }
                    return res;
                }
                break;
            case internal::ProtoWireType::Fixed32:
                {
                    if (startPos+sizeof(uint32_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x. input.data()+startPos, sizeof(uint32_t));
                    boost::endian::small_to_native_inplace<uint32_t>(x);
                    output = (int64_t) x;            
                    return sizeof(uint32_t);
                }
                break;
            case internal::ProtoWireType::Fixed64:
                {
                    if (startPos+sizeof(uint64_t) > input.length()) {
                        return std::nullopt;
                    }
                    uint32_t x;
                    std::memcpy(&x. input.data()+startPos, sizeof(uint64_t));
                    boost::endian::small_to_native_inplace<uint64_t>(x);
                    output = (int64_t) x;            
                    return sizeof(uint64_t);
                }
                break;
            default:
                return std::nullopt;
                break;
            }
        }
        }
    };

    template <class T, typename=std::enable_if_t<std::is_enum_v<T>>>
    class ProtoDecoder<T> final : public IProtoDecoder<T> {
    public:
        ProtoDecoder(T *output) : IProtoDecoder<T>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(T &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            std::underlying_type_t<T> x;
            ProtoDecoder<std::underlying_type_t<T>> dec(&x);
            auto res = dec.handle(wt, input, startPos);
            if (res) {
                output = (T) x;
            }
            return res;
        }
    };

    template <>
    class ProtoDecoder<float> final : public IProtoDecoder<float> {
    public:
        ProtoDecoder(float *output) : IProtoDecoder<float>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(float &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::Fixed32) {
                return std::nullopt;
            }
            if (startPos+sizeof(uint32_t) > input.length()) {
                return std::nullopt;
            }
            uint32_t x;
            std::memcpy(&x, input.data()+startPos, sizeof(uint32_t));
            boost::endian::small_to_native_inplace<uint32_t>(x);
            std::memcpy(&output, &x, sizeof(uint32_t));
            return sizeof(uint32_t);
        }
    };
    template <>
    class ProtoDecoder<double> final : public IProtoDecoder<double> {
    public:
        ProtoDecoder(double *output) : IProtoDecoder<double>(output) {}
        virtual ~ProtoDecoder() = default;
    protected:
        std::optional<std::size_t> read(double &output, internal::ProtoWireType wt, std::string_view const &input, std::size_t start) override final {
            if (wt != internal::ProtoWireType::Fixed64) {
                return std::nullopt;
            }
            if (startPos+sizeof(uint32_t) > input.length()) {
                return std::nullopt;
            }
            uint64_t x;
            std::memcpy(&x, input.data()+startPos, sizeof(uint64_t));
            boost::endian::small_to_native_inplace<uint64_t>(x);
            std::memcpy(&output, &x, sizeof(uint64_t));
            return sizeof(uint64_t);
        }
    };

} } } } }

#endif