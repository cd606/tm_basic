#ifndef TM_KIT_BASIC_BYTE_DATA_HPP_
#define TM_KIT_BASIC_BYTE_DATA_HPP_

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <type_traits>
#include <memory>
#include <iostream>
#include <sstream>
#include <optional>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/SingleLayerWrapper.hpp>
#include <tm_kit/basic/ConstType.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <boost/endian/conversion.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 

    struct ByteData {
        std::string content;
    };

    struct ByteDataWithTopic {
        std::string topic;
        std::string content;
    };

    struct ByteDataWithID {
        std::string id; //When we reach the transport level, all IDs are already converted to string representations
        std::string content;       
    };

    template <class T>
    struct TypedDataWithTopic {
        using ContentType = T;
        std::string topic;
        T content;
    };

    template <class T>
    struct CBOR {
        T value;
    };

    namespace bytedata_utils {
        inline std::ostream &printByteDataDetails(std::ostream &os, ByteData const &d) {
            os << "[";
            for (size_t ii=0; ii<d.content.length(); ++ii) {
                if (ii > 0) {
                    std::cout << ", ";
                }
                os << "0x" << std::hex << std::setw(2)
                    << std::setfill('0') << static_cast<uint16_t>(static_cast<uint8_t>(d.content[ii]))
                    << std::dec;
            }
            os << "] (" << d.content.length() << " bytes)";
            return os; 
        }

        template <class T, typename Enable=void>
        struct RunCBORSerializer {
            static std::vector<uint8_t> apply(T const &data);
        };

        template <class T>
        struct RunCBORSerializer<CBOR<T>, void> {
            static std::vector<uint8_t> apply(CBOR<T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
        };
        template <class A>
        struct RunCBORSerializer<A const *, void> {
            static std::vector<uint8_t> apply(A const *data) {
                if (data) {
                    return RunCBORSerializer<A>::apply(*data);
                } else {
                    return std::vector<uint8_t> {};
                }
            }
        };
        template <>
        struct RunCBORSerializer<bool, void> {
            static std::vector<uint8_t> apply(bool const &data) {
                uint8_t c = data?1:0;
                return std::vector<uint8_t> {&c, &c+1};
            }
        };
        template <>
        struct RunCBORSerializer<uint8_t, void> {
            static std::vector<uint8_t> apply(uint8_t const &data) {
                if (data <= 23) {
                    uint8_t c = data;
                    return std::vector<uint8_t> {&c, &c+1};
                } else {
                    uint8_t buf[2];
                    buf[0] = 24;
                    buf[1] = data;
                    return std::vector<uint8_t> {buf, buf+2};
                }
            }
        };
        template <>
        struct RunCBORSerializer<uint16_t, void> {
            static std::vector<uint8_t> apply(uint16_t const &data) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::apply(static_cast<uint8_t>(data));
                } else {
                    uint8_t buf[3];
                    buf[0] = 25;
                    uint16_t bigEndianVersion = boost::endian::native_to_big<uint16_t>(data);
                    std::memcpy(&buf[1], &bigEndianVersion, 2);
                    return std::vector<uint8_t> {buf, buf+3};
                }
            }
        };
        template <>
        struct RunCBORSerializer<uint32_t, void> {
            static std::vector<uint8_t> apply(uint32_t const &data) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::apply(static_cast<uint8_t>(data));
                } else if (data <= std::numeric_limits<uint16_t>::max()) {
                    return RunCBORSerializer<uint16_t>::apply(static_cast<uint16_t>(data));
                } else {
                    uint8_t buf[5];
                    buf[0] = 26;
                    uint32_t bigEndianVersion = boost::endian::native_to_big<uint32_t>(data);
                    std::memcpy(&buf[1], &bigEndianVersion, 4);
                    return std::vector<uint8_t> {buf, buf+5};
                }
            }
        };
        template <>
        struct RunCBORSerializer<uint64_t, void> {
            static std::vector<uint8_t> apply(uint64_t const &data) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::apply(static_cast<uint8_t>(data));
                } else if (data <= std::numeric_limits<uint16_t>::max()) {
                    return RunCBORSerializer<uint16_t>::apply(static_cast<uint16_t>(data));
                } else if (data <= std::numeric_limits<uint32_t>::max()) {
                    return RunCBORSerializer<uint32_t>::apply(static_cast<uint32_t>(data));
                } else {
                    uint8_t buf[9];
                    buf[0] = 27;
                    uint64_t bigEndianVersion = boost::endian::native_to_big<uint64_t>(data);
                    std::memcpy(&buf[1], &bigEndianVersion, 8);
                    return std::vector<uint8_t> {buf, buf+9};
                }
            }
        };
        template <class T>
        struct RunCBORSerializer<T, std::enable_if_t<
            std::is_integral_v<T> && std::is_signed_v<T>
            ,void
        >> {
            static std::vector<uint8_t> apply(T const &data) {
                if (data >= 0) {
                    return RunCBORSerializer<std::make_unsigned_t<T>>::apply(
                        static_cast<std::make_unsigned_t<T>>(data)
                    );
                } else {
                    std::make_unsigned_t<T> v = static_cast<std::make_unsigned_t<T>>(-(data+1));
                    auto s = RunCBORSerializer<std::make_unsigned_t<T>>::apply(v);
                    s[0] = s[0] | (uint8_t) 0x20;
                    return s;
                }
            }
        };
        template <>
        struct RunCBORSerializer<float, void> {
            static std::vector<uint8_t> apply(float const &data) {
                uint8_t buf[5];
                buf[0] = (uint8_t) 0xE0 | (uint8_t) 26;
                uint32_t dBuf;
                std::memcpy(&dBuf, &data, 4);
                boost::endian::native_to_big_inplace<uint32_t>(dBuf);
                std::memcpy(&buf[1], &dBuf, 4);
                return std::vector<uint8_t> {buf, buf+5};
            }
        };
        template <>
        struct RunCBORSerializer<double, void> {
            static std::vector<uint8_t> apply(double const &data) {
                uint8_t buf[9];
                buf[0] = (uint8_t) 0xE0 | (uint8_t) 27;
                uint64_t dBuf;
                std::memcpy(&dBuf, &data, 8);
                boost::endian::native_to_big_inplace<uint64_t>(dBuf);
                std::memcpy(&buf[1], &dBuf, 8);
                return std::vector<uint8_t> {buf, buf+9};
            }
        };
        template <>
        struct RunCBORSerializer<std::string, void> {
            static std::vector<uint8_t> apply(std::string const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.length());
                size_t prefixLen = r.size();
                r.resize(prefixLen+data.length());
                std::memcpy(r.data()+prefixLen, data.c_str(), data.length());
                r[0] = r[0] | 0x60;
                return r;
            }
        };
        template <>
        struct RunCBORSerializer<ByteData, void> {
            static std::vector<uint8_t> apply(ByteData const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.content.length());
                size_t prefixLen = r.size();
                r.resize(prefixLen+data.content.length());
                std::memcpy(r.data()+prefixLen, data.content.c_str(), data.content.length());
                r[0] = r[0] | 0x40;
                return r;
            }
        };
        template <>
        struct RunCBORSerializer<ByteDataWithTopic, void> {
            static std::vector<uint8_t> apply(ByteDataWithTopic const &data) {
                auto v = RunCBORSerializer<size_t>::apply(2);
                v[0] = v[0] | 0x80;
                auto v1 = RunCBORSerializer<std::string>::apply(data.topic);
                v.insert(v.end(), v1.begin(), v1.end());
                auto v2 = RunCBORSerializer<size_t>::apply(data.content.length());
                size_t prefixLen = v2.size();
                v2.resize(prefixLen+data.content.length());
                std::memcpy(v2.data()+prefixLen, data.content.c_str(), data.content.length());
                v2[0] = v2[0] | 0x40;
                v.insert(v.end(), v2.begin(), v2.end());
                return v;
            }
        };
        template <class A>
        struct RunCBORSerializer<std::vector<A>, void> {
            static std::vector<uint8_t> apply(std::vector<A> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.size());
                r[0] = r[0] | 0x80;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item);
                    r.insert(r.end(), r1.begin(), r1.end());
                }
                return r;
            }
        };
        template <class A, size_t N>
        struct RunCBORSerializer<std::array<A, N>, void> {
            static std::vector<uint8_t> apply(std::array<A, N> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(N);
                r[0] = r[0] | 0x80;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item);
                    r.insert(r.end(), r1.begin(), r1.end());
                }
                return r;
            }
        };
        template <class A>
        struct RunCBORSerializer<std::list<A>, void> {
            static std::vector<uint8_t> apply(std::list<A> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.size());
                r[0] = r[0] | 0x80;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item);
                    r.insert(r.end(), r1.begin(), r1.end());
                }
                return r;
            }
        };
        template <class A, class Cmp>
        struct RunCBORSerializer<std::set<A, Cmp>, void> {
            static std::vector<uint8_t> apply(std::set<A, Cmp> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.size());
                r[0] = r[0] | 0x80;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item);
                    r.insert(r.end(), r1.begin(), r1.end());
                }
                return r;
            }
        };
        template <class A, class Hash>
        struct RunCBORSerializer<std::unordered_set<A, Hash>, void> {
            static std::vector<uint8_t> apply(std::unordered_set<A, Hash> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.size());
                r[0] = r[0] | 0x80;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item);
                    r.insert(r.end(), r1.begin(), r1.end());
                }
                return r;
            }
        };
        template <class A, class B, class Cmp>
        struct RunCBORSerializer<std::map<A, B, Cmp>, void> {
            static std::vector<uint8_t> apply(std::map<A, B, Cmp> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.size());
                r[0] = r[0] | 0xA0;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item.first);
                    r.insert(r.end(), r1.begin(), r1.end());
                    auto r2 = RunCBORSerializer<B>::apply(item.second);
                    r.insert(r.end(), r2.begin(), r2.end());
                }
                return r;
            }
        };
        template <class A, class B, class Hash>
        struct RunCBORSerializer<std::unordered_map<A, B, Hash>, void> {
            static std::vector<uint8_t> apply(std::unordered_map<A, B, Hash> const &data) {
                auto r = RunCBORSerializer<size_t>::apply(data.size());
                r[0] = r[0] | 0xA0;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<A>::apply(item.first);
                    r.insert(r.end(), r1.begin(), r1.end());
                    auto r2 = RunCBORSerializer<B>::apply(item.second);
                    r.insert(r.end(), r2.begin(), r2.end());
                }
                return r;
            }
        };
        template <class A>
        struct RunCBORSerializer<std::unique_ptr<A>, void> {
            static std::vector<uint8_t> apply(std::unique_ptr<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {data.get()});
                }
            }
        };
        template <class A>
        struct RunCBORSerializer<std::shared_ptr<A>, void> {
            static std::vector<uint8_t> apply(std::shared_ptr<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {data.get()});
                }
            }
        };
        template <class A>
        struct RunCBORSerializer<std::optional<A>, void> {
            static std::vector<uint8_t> apply(std::optional<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {&(*data)});
                }
            }
        };
        template <>
        struct RunCBORSerializer<VoidStruct, void> {
            static std::vector<uint8_t> apply(VoidStruct const &data) {
                return RunCBORSerializer<int32_t>::apply(0);
            }
        };
        template <class T>
        struct RunCBORSerializer<SingleLayerWrapper<T>, void> {
            static std::vector<uint8_t> apply(SingleLayerWrapper<T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
        };
        template <int32_t N>
        struct RunCBORSerializer<ConstType<N>, void> {
            static std::vector<uint8_t> apply(ConstType<N> const &data) {
                return RunCBORSerializer<int32_t>::apply(N);
            }
        };

        template <class T, typename Enable=void>
        struct RunCBORDeserializer {
            static std::optional<std::tuple<T,size_t>> apply(std::string_view const &data, size_t start);
        };

        template <class T>
        struct RunCBORDeserializer<CBOR<T>, void> {
            static std::optional<std::tuple<CBOR<T>,size_t>> apply(std::string_view const &data, size_t start) {
                auto t = RunCBORDeserializer<T>::apply(data, start);
                if (t) {
                    return std::tuple<CBOR<T>,size_t> { CBOR<T> {std::move(std::get<0>(*t))}, std::get<1>(*t) };
                } else {
                    return std::nullopt;
                }
            }
        };


        template <class T>
        inline std::optional<std::tuple<T, size_t>> parseCBORUnsignedInt(std::string_view const &data, size_t start) {
            if (data.length() < start+1) {
                return std::nullopt;
            }
            uint8_t c = (static_cast<uint8_t>(data[start]) & (uint8_t) 0x1f);
            if (c <= 23) {
                return std::tuple<T, size_t> { static_cast<T>(c), 1 };
            }
            switch (c) {
            case 24:
                if (data.length() < start+2) {
                    return std::nullopt;
                }
                return std::tuple<T, size_t> { static_cast<T>(static_cast<uint8_t>(data[start+1])), 2 };
            case 25:
                {
                    if (data.length() < start+3) {
                        return std::nullopt;
                    }
                    uint16_t bigEndianVersion;
                    data.copy(reinterpret_cast<char *>(&bigEndianVersion), 2, start+1);
                    return std::tuple<T, size_t> { static_cast<T>(boost::endian::big_to_native<uint16_t>(bigEndianVersion)), 3 };
                }
                break;
            case 26:
                {
                    if (data.length() < start+5) {
                        return std::nullopt;
                    }
                    uint32_t bigEndianVersion;
                    data.copy(reinterpret_cast<char *>(&bigEndianVersion), 4, start+1);
                    return std::tuple<T, size_t> { static_cast<T>(boost::endian::big_to_native<uint32_t>(bigEndianVersion)), 5 };
                }
                break;
            case 27:
                {
                    if (data.length() < start+9) {
                        return std::nullopt;
                    }
                    uint64_t bigEndianVersion;
                    data.copy(reinterpret_cast<char *>(&bigEndianVersion), 8, start+1);
                    return std::tuple<T, size_t> { static_cast<T>(boost::endian::big_to_native<uint64_t>(bigEndianVersion)), 9 };
                }
                break;
            default:
                return std::nullopt;
            }
        }

        template <class T>
        inline std::optional<std::tuple<T, size_t>> parseCBORInt(std::string_view const &data, size_t start) {
            if (data.length() < start+1) {
                return std::nullopt;
            }
            bool isNegative = ((static_cast<uint8_t>(data[start]) & (uint8_t) 0x20) == (uint8_t) 0x20);
            if (isNegative) {
                auto v = parseCBORUnsignedInt<std::make_unsigned_t<T>>(data, start);
                if (v) {
                    return std::tuple<T, size_t> {-(static_cast<T>(std::get<0>(*v)))-1, std::get<1>(*v)};
                } else {
                    return std::nullopt;
                }
            } else {
                return parseCBORUnsignedInt<T>(data, start);
            }
        }

        template <>
        struct RunCBORDeserializer<bool, void> {
            static std::optional<std::tuple<bool, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != (uint8_t) 0) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<uint8_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                switch (std::get<0>(*v)) {
                case 0:
                    return std::tuple<bool, size_t> {false, std::get<1>(*v)};
                case 1:
                    return std::tuple<bool, size_t> {true, std::get<1>(*v)};
                default:
                    return std::nullopt;
                }
            }
        };
        template <>
        struct RunCBORDeserializer<uint8_t, void> {
            static std::optional<std::tuple<uint8_t, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != (uint8_t) 0) {
                    return std::nullopt;
                }
                return parseCBORUnsignedInt<uint8_t>(data, start);
            }
        };
        template <>
        struct RunCBORDeserializer<uint16_t, void> {
            static std::optional<std::tuple<uint16_t, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != (uint8_t) 0) {
                    return std::nullopt;
                }
                return parseCBORUnsignedInt<uint16_t>(data, start);
            }
        };
        template <>
        struct RunCBORDeserializer<uint32_t, void> {
            static std::optional<std::tuple<uint32_t, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != (uint8_t) 0) {
                    return std::nullopt;
                }
                return parseCBORUnsignedInt<uint32_t>(data, start);
            }
        };
        template <>
        struct RunCBORDeserializer<uint64_t, void> {
            static std::optional<std::tuple<uint64_t, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != (uint8_t) 0) {
                    return std::nullopt;
                }
                return parseCBORUnsignedInt<uint64_t>(data, start);
            }
        };
        template <class T>
        struct RunCBORDeserializer<T, std::enable_if_t<
            std::is_integral_v<T> && std::is_signed_v<T>
            ,void
        >> {
            static std::optional<std::tuple<T, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                uint8_t c = (static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0);
                if (c != (uint8_t) 0 && c != (uint8_t) 0x20) {
                    return std::nullopt;
                }
                return parseCBORInt<T>(data, start);
            }
        };
        template <>
        struct RunCBORDeserializer<float, void> {
            static std::optional<std::tuple<float, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+5) {
                    return std::nullopt;
                }
                if (static_cast<uint8_t>(data[start]) != ((uint8_t) 0xE0 | (uint8_t) 26)) {
                    return std::nullopt;
                }
                uint32_t dBuf;
                data.copy(reinterpret_cast<char *>(&dBuf), 4, start+1);
                boost::endian::big_to_native_inplace<uint32_t>(dBuf);
                float f;
                std::memcpy(&f, &dBuf, 4);
                return std::tuple<float, size_t> {f, 5};
            }
        };
        template <>
        struct RunCBORDeserializer<double, void> {
            static std::optional<std::tuple<double, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+9) {
                    return std::nullopt;
                }
                if (static_cast<uint8_t>(data[start]) != ((uint8_t) 0xE0 | (uint8_t) 27)) {
                    return std::nullopt;
                }
                uint64_t dBuf;
                data.copy(reinterpret_cast<char *>(&dBuf), 8, start+1);
                boost::endian::big_to_native_inplace<uint64_t>(dBuf);
                double f;
                std::memcpy(&f, &dBuf, 8);
                return std::tuple<double, size_t> {f, 9};
            }
        };
        template <>
        struct RunCBORDeserializer<std::string, void> {
            static std::optional<std::tuple<std::string, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x60) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                if (data.length() < start+std::get<0>(*v)+std::get<1>(*v)) {
                    return std::nullopt;
                }
                return std::tuple<std::string, size_t> {data.substr(start+std::get<1>(*v), std::get<0>(*v)), std::get<0>(*v)+std::get<1>(*v)};
            }
        };
        template <>
        struct RunCBORDeserializer<ByteData, void> {
            static std::optional<std::tuple<ByteData, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x40) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                if (data.length() < start+std::get<0>(*v)+std::get<1>(*v)) {
                    return std::nullopt;
                }
                return std::tuple<ByteData, size_t> {ByteData { std::string {data.substr(start+std::get<1>(*v), std::get<0>(*v))} }, std::get<0>(*v)+std::get<1>(*v)};
            }
        };
        template <>
        struct RunCBORDeserializer<ByteDataWithTopic, void> {
            static std::optional<std::tuple<ByteDataWithTopic, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
                    return std::nullopt;
                }
                auto n = parseCBORUnsignedInt<size_t>(data, start);
                if (!n) {
                    return std::nullopt;
                }
                if (std::get<0>(*n) != 2) {
                    return std::nullopt;
                }
                size_t accumLen = std::get<1>(*n);
                auto topic = RunCBORDeserializer<std::string>::apply(data, start+accumLen);
                if (!topic) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*topic);
                auto content = RunCBORDeserializer<ByteData>::apply(data, start+accumLen);
                if (!content) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*content);
                return std::tuple<ByteDataWithTopic, size_t> {
                    ByteDataWithTopic {std::move(std::get<0>(*topic)), std::move(std::get<0>(*content).content)}
                    , accumLen
                };
            }
        };
        template <class A>
        struct RunCBORDeserializer<std::vector<A>, void> {
            static std::optional<std::tuple<std::vector<A>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::vector<A> ret;
                ret.resize(std::get<0>(*v));
                for (size_t ii=0; ii<std::get<0>(*v); ++ii) {
                    auto r = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r);
                    newStart += std::get<1>(*r);
                    ret[ii] = std::move(std::get<0>(*r));
                }
                return std::tuple<std::vector<A>, size_t> {std::move(ret), len};
            }
        };
        template <class A, size_t N>
        struct RunCBORDeserializer<std::array<A,N>, void> {
            static std::optional<std::tuple<std::array<A,N>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                if (std::get<0>(*v) != N) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::array<A,N> ret;
                for (size_t ii=0; ii<N; ++ii) {
                    auto r = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r);
                    newStart += std::get<1>(*r);
                    ret[ii] = std::move(std::get<0>(*r));
                }
                return std::tuple<std::array<A,N>, size_t> {std::move(ret), len};
            }
        };
        template <class A>
        struct RunCBORDeserializer<std::list<A>, void> {
            static std::optional<std::tuple<std::list<A>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::list<A> ret;
                for (size_t ii=0; ii<std::get<0>(*v); ++ii) {
                    auto r = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r);
                    newStart += std::get<1>(*r);
                    ret.push_back(std::move(std::get<0>(*r)));
                }
                return std::tuple<std::list<A>, size_t> {std::move(ret), len};
            }
        };
        template <class A, class Cmp>
        struct RunCBORDeserializer<std::set<A, Cmp>, void> {
            static std::optional<std::tuple<std::set<A, Cmp>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::set<A, Cmp> ret;
                for (size_t ii=0; ii<std::get<0>(*v); ++ii) {
                    auto r = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r);
                    newStart += std::get<1>(*r);
                    ret.insert(std::move(std::get<0>(*r)));
                }
                return std::tuple<std::set<A, Cmp>, size_t> {std::move(ret), len};
            }
        };
        template <class A, class Hash>
        struct RunCBORDeserializer<std::unordered_set<A, Hash>, void> {
            static std::optional<std::tuple<std::unordered_set<A, Hash>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::unordered_set<A, Hash> ret;
                for (size_t ii=0; ii<std::get<0>(*v); ++ii) {
                    auto r = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r);
                    newStart += std::get<1>(*r);
                    ret.insert(std::move(std::get<0>(*r)));
                }
                return std::tuple<std::unordered_set<A, Hash>, size_t> {std::move(ret), len};
            }
        };
        template <class A, class B, class Cmp>
        struct RunCBORDeserializer<std::map<A, B, Cmp>, void> {
            static std::optional<std::tuple<std::map<A, B, Cmp>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0xa0) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::map<A, B, Cmp> ret;
                for (size_t ii=0; ii<std::get<0>(*v); ++ii) {
                    auto r1 = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r1) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r1);
                    newStart += std::get<1>(*r1);
                    auto r2 = RunCBORDeserializer<B>::apply(data, newStart);
                    if (!r2) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r2);
                    newStart += std::get<1>(*r2);
                    ret.insert({std::move(std::get<0>(*r1)), std::move(std::get<0>(*r2))});
                }
                return std::tuple<std::map<A, B, Cmp>, size_t> {std::move(ret), len};
            }
        };
        template <class A, class B, class Hash>
        struct RunCBORDeserializer<std::unordered_map<A, B, Hash>, void> {
            static std::optional<std::tuple<std::unordered_map<A, B, Hash>, size_t>> apply(std::string_view const &data, size_t start) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0xa0) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                std::unordered_map<A, B, Hash> ret;
                for (size_t ii=0; ii<std::get<0>(*v); ++ii) {
                    auto r1 = RunCBORDeserializer<A>::apply(data, newStart);
                    if (!r1) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r1);
                    newStart += std::get<1>(*r1);
                    auto r2 = RunCBORDeserializer<B>::apply(data, newStart);
                    if (!r2) {
                        return std::nullopt;
                    }
                    len += std::get<1>(*r2);
                    newStart += std::get<1>(*r2);
                    ret.insert({std::move(std::get<0>(*r1)), std::move(std::get<0>(*r2))});
                }
                return std::tuple<std::unordered_map<A, B, Hash>, size_t> {std::move(ret), len};
            }
        };
        template <class A>
        struct RunCBORDeserializer<std::unique_ptr<A>, void> {
            static std::optional<std::tuple<std::unique_ptr<A>, size_t>> apply(std::string_view const &data, size_t start) {
                auto v = RunCBORDeserializer<std::vector<A>>::apply(data, start);
                if (!v) {
                    return std::nullopt;
                }
                switch (std::get<0>(*v).size()) {
                case 0:
                    return std::tuple<std::unique_ptr<A>, size_t> {std::unique_ptr<A>(), std::get<1>(*v)};
                case 1:
                    return std::tuple<std::unique_ptr<A>, size_t> {std::make_unique<A>(std::move((std::get<0>(*v))[0])), std::get<1>(*v)};
                default:
                    return std::nullopt;
                }
            }
        };
        template <class A>
        struct RunCBORDeserializer<std::shared_ptr<A>, void> {
            static std::optional<std::tuple<std::shared_ptr<A>, size_t>> apply(std::string_view const &data, size_t start) {
                auto v = RunCBORDeserializer<std::vector<A>>::apply(data, start);
                if (!v) {
                    return std::nullopt;
                }
                switch (std::get<0>(*v).size()) {
                case 0:
                    return std::tuple<std::shared_ptr<A>, size_t> {std::shared_ptr<A>(), std::get<1>(*v)};
                case 1:
                    return std::tuple<std::shared_ptr<A>, size_t> {std::make_shared<A>(std::move((std::get<0>(*v))[0])), std::get<1>(*v)};
                default:
                    return std::nullopt;
                }
            }
        };
        template <class A>
        struct RunCBORDeserializer<std::optional<A>, void> {
            static std::optional<std::tuple<std::optional<A>, size_t>> apply(std::string_view const &data, size_t start) {
                auto v = RunCBORDeserializer<std::vector<A>>::apply(data, start);
                if (!v) {
                    return std::nullopt;
                }
                switch (std::get<0>(*v).size()) {
                case 0:
                    return std::tuple<std::optional<A>, size_t> {std::nullopt, std::get<1>(*v)};
                case 1:
                    return std::tuple<std::optional<A>, size_t> {std::optional<A> {std::move((std::get<0>(*v))[0])}, std::get<1>(*v)};
                default:
                    return std::nullopt;
                }
            }
        };
        template <>
        struct RunCBORDeserializer<VoidStruct, void> {
            static std::optional<std::tuple<VoidStruct, size_t>> apply(std::string_view const &data, size_t start) {
                auto r = RunCBORDeserializer<int32_t>::apply(data, start);
                if (!r) {
                    return std::nullopt;
                }
                if (std::get<0>(*r) != 0) {
                    return std::nullopt;
                }
                return std::tuple<VoidStruct, size_t> {VoidStruct {}, std::get<1>(*r)};
            }
        };
        template <class T>
        struct RunCBORDeserializer<SingleLayerWrapper<T>, void> {
            static std::optional<std::tuple<SingleLayerWrapper<T>, size_t>> apply(std::string_view const &data, size_t start) {
                auto r = RunCBORDeserializer<T>::apply(data, start);
                if (!r) {
                    return std::nullopt;
                }
                return std::tuple<SingleLayerWrapper<T>, size_t> {SingleLayerWrapper<T> {std::move(std::get<0>(*r))}, std::get<1>(*r)};
            }
        };
        template <int32_t N>
        struct RunCBORDeserializer<ConstType<N>, void> {
            static std::optional<std::tuple<ConstType<N>, size_t>> apply(std::string_view const &data, size_t start) {
                auto r = RunCBORDeserializer<int32_t>::apply(data, start);
                if (!r) {
                    return std::nullopt;
                }
                if (std::get<0>(*r) != N) {
                    return std::nullopt;
                }
                return std::tuple<ConstType<N>, size_t> {ConstType<N> {}, std::get<1>(*r)};
            }
        };

        template <class T, typename Enable=void>
        struct RunSerializer {
            static std::string apply(T const &data) {
                std::string s;
                data.SerializeToString(&s);
                return s;
            }
        };
        template <class T>
        struct RunSerializer<CBOR<T>, void> {
            static std::string apply(CBOR<T> const &data) {
                auto res = RunCBORSerializer<T>::apply(data.value);
                const char *p = reinterpret_cast<const char *>(res.data());
                return std::string {p, p+res.size()};
            }
        };
        template <>
        struct RunSerializer<std::string, void> {
            static std::string apply(std::string const &data) {
                return data;
            }
        };
        template <>
        struct RunSerializer<ByteData, void> {
            static std::string apply(ByteData const &data) {
                return data.content;
            }
        };
        template <>
        struct RunSerializer<bool, void> {
            static std::string apply(bool const &data) {
                char x = data?1:0;
                return std::string {&x, &x+1};
            }
        };
        template <class T>
        struct RunSerializer<T, std::enable_if_t<std::is_integral_v<T>,void>> {
            static std::string apply(T const &data) {
                T littleEndianVersion = boost::endian::native_to_little<T>(data);
                char *p = reinterpret_cast<char *>(&littleEndianVersion);
                return std::string {p, p+sizeof(T)};
            }
        };
        template <>
        struct RunSerializer<float, void> {
            static std::string apply(float const &data) {
                uint32_t dBuf;
                std::memcpy(&dBuf, &data, 4);
                boost::endian::native_to_little_inplace<uint32_t>(dBuf);
                const char *p = reinterpret_cast<const char *>(&dBuf);
                return std::string {p, p+4};
            }
        };
        template <>
        struct RunSerializer<double, void> {
            static std::string apply(double const &data) {
                uint64_t dBuf;
                std::memcpy(&dBuf, &data, 8);
                boost::endian::native_to_little_inplace<uint64_t>(dBuf);
                const char *p = reinterpret_cast<const char *>(&dBuf);
                return std::string {p, p+8};
            }
        };
        template <>
        struct RunSerializer<ByteDataWithTopic, void> {
            static std::string apply(ByteDataWithTopic const &data) {
                std::ostringstream oss;
                oss << RunSerializer<uint32_t>::apply(data.topic.length())
                    << data.topic
                    << data.content;
                return oss.str();
            }
        };
        template <class A>
        struct RunSerializer<std::unique_ptr<A>, void> {
            static std::string apply(std::unique_ptr<A> const &data) {
                if (data) {
                    auto s = RunSerializer<A>::apply(*data);
                    std::ostringstream oss;
                    oss << RunSerializer<int8_t>::apply(1) << s;
                    return oss.str();
                } else {
                    return RunSerializer<int8_t>::apply(0);
                }
            }
        };
        template <class A>
        struct RunSerializer<std::shared_ptr<A>, void> {
            static std::string apply(std::shared_ptr<A> const &data) {
                if (data) {
                    auto s = RunSerializer<A>::apply(*data);
                    std::ostringstream oss;
                    oss << RunSerializer<int8_t>::apply(1) << s;
                    return oss.str();
                } else {
                    return RunSerializer<int8_t>::apply(0);
                }
            }
        };
        //This is a helper specialization and does not have a deserializer counterpart
        template <class A>
        struct RunSerializer<A const *, void> {
            static std::string apply(A const *data) {
                if (data) {
                    return RunSerializer<A>::apply(*data);
                } else {
                    return "";
                }
            }
        };
        template <class A>
        struct RunSerializer<std::vector<A>, void> {
            static std::string apply(std::vector<A> const &data) {
                int32_t len = data.size();
                std::ostringstream oss;
                oss << RunSerializer<int32_t>::apply(len);
                for (auto const &item : data) {
                    auto s = RunSerializer<A>::apply(item);
                    int32_t sLen = s.length();
                    oss << RunSerializer<int32_t>::apply(sLen)
                        << s;
                }
                return oss.str();
            }
        };
        template <class A, size_t N>
        struct RunSerializer<std::array<A,N>, void> {
            static std::string apply(std::array<A,N> const &data) {
                std::ostringstream oss;
                oss << RunSerializer<uint32_t>::apply(static_cast<uint32_t>(N));
                for (auto const &item : data) {
                    auto s = RunSerializer<A>::apply(item);
                    int32_t sLen = s.length();
                    oss << RunSerializer<int32_t>::apply(sLen)
                        << s;
                }
                return oss.str();
            }
        };
        template <class A>
        struct RunSerializer<std::list<A>, void> {
            static std::string apply(std::list<A> const &data) {
                int32_t len = data.size();
                std::ostringstream oss;
                oss << RunSerializer<int32_t>::apply(len);
                for (auto const &item : data) {
                    auto s = RunSerializer<A>::apply(item);
                    int32_t sLen = s.length();
                    oss << RunSerializer<int32_t>::apply(sLen)
                        << s;
                }
                return oss.str();
            }
        };
        template <class A, class Cmp>
        struct RunSerializer<std::set<A, Cmp>, void> {
            static std::string apply(std::set<A, Cmp> const &data) {
                int32_t len = data.size();
                std::ostringstream oss;
                oss << RunSerializer<int32_t>::apply(len);
                for (auto const &item : data) {
                    auto s = RunSerializer<A>::apply(item);
                    int32_t sLen = s.length();
                    oss << RunSerializer<int32_t>::apply(sLen)
                        << s;
                }
                return oss.str();
            }
        };
        template <class A, class Hash>
        struct RunSerializer<std::unordered_set<A, Hash>, void> {
            static std::string apply(std::unordered_set<A, Hash> const &data) {
                int32_t len = data.size();
                std::ostringstream oss;
                oss << RunSerializer<int32_t>::apply(len);
                for (auto const &item : data) {
                    auto s = RunSerializer<A>::apply(item);
                    int32_t sLen = s.length();
                    oss << RunSerializer<int32_t>::apply(sLen)
                        << s;
                }
                return oss.str();
            }
        };
        template <class A, class B, class Cmp>
        struct RunSerializer<std::map<A, B, Cmp>, void> {
            static std::string apply(std::map<A, B, Cmp> const &data) {
                int32_t len = data.size();
                std::ostringstream oss;
                oss << RunSerializer<int32_t>::apply(len);
                for (auto const &item : data) {
                    auto s1 = RunSerializer<A>::apply(item.first);
                    auto s2 = RunSerializer<B>::apply(item.second);
                    int32_t s1Len = s1.length();
                    int32_t s2Len = s2.length();
                    oss << RunSerializer<int32_t>::apply(s1Len)
                        << s1
                        << RunSerializer<int32_t>::apply(s2Len)
                        << s2;
                }
                return oss.str();
            }
        };
        template <class A, class B, class Hash>
        struct RunSerializer<std::unordered_map<A, B, Hash>, void> {
            static std::string apply(std::unordered_map<A, B, Hash> const &data) {
                int32_t len = data.size();
                std::ostringstream oss;
                oss << RunSerializer<int32_t>::apply(len);
                for (auto const &item : data) {
                    auto s1 = RunSerializer<A>::apply(item.first);
                    auto s2 = RunSerializer<B>::apply(item.second);
                    int32_t s1Len = s1.length();
                    int32_t s2Len = s2.length();
                    oss << RunSerializer<int32_t>::apply(s1Len)
                        << s1
                        << RunSerializer<int32_t>::apply(s2Len)
                        << s2;
                }
                return oss.str();
            }
        };
        template <class A>
        struct RunSerializer<std::optional<A>, void> {
            static std::string apply(std::optional<A> const &data) {
                if (data) {
                    auto s = RunSerializer<A>::apply(*data);
                    std::ostringstream oss;
                    oss << RunSerializer<int8_t>::apply(1) << s;
                    return oss.str();
                } else {
                    return RunSerializer<int8_t>::apply(0);
                }
            }
        };
        template <>
        struct RunSerializer<VoidStruct, void> {
            static std::string apply(VoidStruct const &data) {
                return std::string();
            }
        };
        template <class T>
        struct RunSerializer<SingleLayerWrapper<T>, void> {
            static std::string apply(SingleLayerWrapper<T> const &data) {
                return RunSerializer<T>::apply(data.value);
            }
        };
        template <int32_t N>
        struct RunSerializer<ConstType<N>, void> {
            static std::string apply(ConstType<N> const &data) {
                return RunSerializer<int32_t>::apply(N);
            }
        };
       
        template <class T, typename Enable=void>
        struct RunDeserializer {
            static std::optional<T> apply(std::string const &data) {
                T t;
                if (t.ParseFromString(data)) {
                    return t;
                } else {
                    return std::nullopt;
                }
            }
        };
        template <class T>
        struct RunDeserializer<CBOR<T>, void> {
            static std::optional<CBOR<T>> apply(std::string const &data) {
                auto r = RunCBORDeserializer<T>::apply(std::string_view {data}, 0);
                if (!r) {
                    return std::nullopt;
                }
                if (std::get<1>(*r) != data.length()) {
                    return std::nullopt;
                }
                return std::optional<CBOR<T>> {
                    CBOR<T> {std::move(std::get<0>(*r))}
                };
            }
        };
        template <>
        struct RunDeserializer<std::string, void> {
            static std::optional<std::string> apply(std::string const &data) {
                return {data};
            }
        };
        template <>
        struct RunDeserializer<ByteData, void> {
            static std::optional<ByteData> apply(std::string const &data) {
                return {ByteData {data}};
            }
        };
        template <>
        struct RunDeserializer<bool, void> {
            static std::optional<bool> apply(std::string const &data) {
                if (data.length() != 1) {
                    return std::nullopt;
                }
                char c = data[0];
                switch (c) {
                case 0:
                    return false;
                case 1:
                    return true;
                default:
                    return std::nullopt;
                }
            }
        };
        template <class T>
        struct RunDeserializer<T, std::enable_if_t<std::is_integral_v<T>,void>> {
            static std::optional<T> apply(std::string const &data) {
                if (data.length() != sizeof(T)) {
                    return std::nullopt;
                }
                T littleEndianVersion;
                std::memcpy(&littleEndianVersion, data.c_str(), sizeof(T));
                return {boost::endian::little_to_native<T>(littleEndianVersion)};
            }
        };
        template <>
        struct RunDeserializer<float, void> {
            static std::optional<float> apply(std::string const &data) {
                if (data.length() != 4) {
                    return std::nullopt;
                }
                uint32_t dBuf;
                std::memcpy(&dBuf, data.c_str(), 4);
                boost::endian::little_to_native_inplace<uint32_t>(dBuf);
                float f;
                std::memcpy(&f, &dBuf, 4);
                return {f};
            }
        };
        template <>
        struct RunDeserializer<double, void> {
            static std::optional<double> apply(std::string const &data) {
                if (data.length() != 8) {
                    return std::nullopt;
                }
                uint64_t dBuf;
                std::memcpy(&dBuf, data.c_str(), 8);
                boost::endian::little_to_native_inplace<uint64_t>(dBuf);
                double f;
                std::memcpy(&f, &dBuf, 8);
                return {f};
            }
        };
        template <>
        struct RunDeserializer<ByteDataWithTopic, void> {
            static std::optional<ByteDataWithTopic> apply(std::string const &data) {
                if (data.length() < sizeof(uint32_t)) {
                    return std::nullopt;
                }
                auto topicLen = RunDeserializer<uint32_t>::apply(data.substr(0, sizeof(uint32_t)));
                if (!topicLen) {
                    return std::nullopt;
                }
                if (data.length() < *topicLen+sizeof(uint32_t)) {
                    return std::nullopt;
                }
                return ByteDataWithTopic {
                    data.substr(sizeof(uint32_t), *topicLen)
                    , data.substr(*topicLen+sizeof(uint32_t))
                };
            }
        };
        template <class A>
        struct RunDeserializer<std::unique_ptr<A>, void> {
            static std::optional<std::unique_ptr<A>> apply(std::string const &data) {
                if (data.length() == 0) {
                    return std::nullopt;
                }
                auto x = RunDeserializer<int8_t>::apply(data.substr(0,1));
                if (!x) {
                    return std::nullopt;
                }
                if (*x == 0) {
                    return {std::unique_ptr<A> {}};
                } else {
                    auto a = RunDeserializer<A>::apply(data.substr(1));
                    if (!a) {
                        return std::nullopt;
                    }
                    return {std::make_unique<A>(std::move(*a))};
                }
            }
        };
        template <class A>
        struct RunDeserializer<std::shared_ptr<A>, void> {
            static std::optional<std::shared_ptr<A>> apply(std::string const &data) {
                if (data.length() == 0) {
                    return std::nullopt;
                }
                auto x = RunDeserializer<int8_t>::apply(data.substr(0,1));
                if (!x) {
                    return std::nullopt;
                }
                if (*x == 0) {
                    return {std::shared_ptr<A> {}};
                } else {
                    auto a = RunDeserializer<A>::apply(data.substr(1));
                    if (!a) {
                        return std::nullopt;
                    }
                    return {std::make_shared<A>(std::move(*a))};
                }
            }
        };
        template <class A>
        struct RunDeserializer<std::optional<A>, void> {
            static std::optional<std::optional<A>> apply(std::string const &data) {
                if (data.length() == 0) {
                    return std::nullopt;
                }
                auto x = RunDeserializer<int8_t>::apply(data.substr(0,1));
                if (!x) {
                    return std::nullopt;
                }
                if (*x == 0) {
                    return std::optional<std::optional<A>> { std::optional<A> {std::nullopt} };
                } else {
                    auto a = RunDeserializer<A>::apply(data.substr(1));
                    if (!a) {
                        return std::nullopt;
                    }
                    return { {std::move(*a)} };
                }
            }
        };
        template <class A>
        struct RunDeserializer<std::vector<A>, void> {
            static std::optional<std::vector<A>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(int32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                p += sizeof(int32_t);
                len -= sizeof(int32_t);
                std::vector<A> res;
                res.resize(*resLen);
                for (int32_t ii=0; ii<*resLen; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto itemLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!itemLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *itemLen) {
                        return std::nullopt;
                    }
                    auto item = RunDeserializer<A>::apply(std::string(p, p+(*itemLen)));
                    if (!item) {
                        return std::nullopt;
                    }
                    res[ii] = std::move(*item);
                    p += *itemLen;
                    len -= *itemLen;
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <class A, size_t N>
        struct RunDeserializer<std::array<A,N>, void> {
            static std::optional<std::array<A,N>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(uint32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<uint32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                if (*resLen != static_cast<uint32_t>(N)) {
                    return std::nullopt;
                }
                p += sizeof(uint32_t);
                len -= sizeof(uint32_t);
                std::array<A,N> res;
                for (size_t ii=0; ii<N; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto itemLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!itemLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *itemLen) {
                        return std::nullopt;
                    }
                    auto item = RunDeserializer<A>::apply(std::string(p, p+(*itemLen)));
                    if (!item) {
                        return std::nullopt;
                    }
                    res[ii] = std::move(*item);
                    p += *itemLen;
                    len -= *itemLen;
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <class A>
        struct RunDeserializer<std::list<A>, void> {
            static std::optional<std::list<A>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(int32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                p += sizeof(int32_t);
                len -= sizeof(int32_t);
                std::list<A> res;
                for (int32_t ii=0; ii<*resLen; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto itemLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!itemLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *itemLen) {
                        return std::nullopt;
                    }
                    auto item = RunDeserializer<A>::apply(std::string(p, p+(*itemLen)));
                    if (!item) {
                        return std::nullopt;
                    }
                    res.push_back(std::move(*item));
                    p += *itemLen;
                    len -= *itemLen;
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <class A, class Cmp>
        struct RunDeserializer<std::set<A, Cmp>, void> {
            static std::optional<std::set<A, Cmp>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(int32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                p += sizeof(int32_t);
                len -= sizeof(int32_t);
                std::set<A, Cmp> res;
                for (int32_t ii=0; ii<*resLen; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto itemLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!itemLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *itemLen) {
                        return std::nullopt;
                    }
                    auto item = RunDeserializer<A>::apply(std::string(p, p+(*itemLen)));
                    if (!item) {
                        return std::nullopt;
                    }
                    res.insert(std::move(*item));
                    p += *itemLen;
                    len -= *itemLen;
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <class A, class Hash>
        struct RunDeserializer<std::unordered_set<A, Hash>, void> {
            static std::optional<std::unordered_set<A, Hash>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(int32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                p += sizeof(int32_t);
                len -= sizeof(int32_t);
                std::unordered_set<A, Hash> res;
                for (int32_t ii=0; ii<*resLen; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto itemLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!itemLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *itemLen) {
                        return std::nullopt;
                    }
                    auto item = RunDeserializer<A>::apply(std::string(p, p+(*itemLen)));
                    if (!item) {
                        return std::nullopt;
                    }
                    res.insert(std::move(*item));
                    p += *itemLen;
                    len -= *itemLen;
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <class A, class B, class Cmp>
        struct RunDeserializer<std::map<A, B, Cmp>, void> {
            static std::optional<std::map<A, B, Cmp>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(int32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                p += sizeof(int32_t);
                len -= sizeof(int32_t);
                std::map<A, B, Cmp> res;
                for (int32_t ii=0; ii<*resLen; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto keyLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!keyLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *keyLen) {
                        return std::nullopt;
                    }
                    auto key = RunDeserializer<A>::apply(std::string(p, p+(*keyLen)));
                    if (!key) {
                        return std::nullopt;
                    }
                    p += *keyLen;
                    len -= *keyLen;
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto dataLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!dataLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *dataLen) {
                        return std::nullopt;
                    }
                    auto data = RunDeserializer<B>::apply(std::string(p, p+(*dataLen)));
                    if (!data) {
                        return std::nullopt;
                    }
                    p += *dataLen;
                    len -= *dataLen;

                    res.insert({std::move(*key), std::move(*data)});
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <class A, class B, class Hash>
        struct RunDeserializer<std::unordered_map<A, B, Hash>, void> {
            static std::optional<std::unordered_map<A, B, Hash>> apply(std::string const &data) {
                const char *p = data.c_str();
                size_t len = data.length();
                if (len < sizeof(int32_t)) {
                    return std::nullopt;
                }
                auto resLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                if (!resLen) {
                    return std::nullopt;
                }
                p += sizeof(int32_t);
                len -= sizeof(int32_t);
                std::unordered_map<A, B, Hash> res;
                for (int32_t ii=0; ii<*resLen; ++ii) {
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto keyLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!keyLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *keyLen) {
                        return std::nullopt;
                    }
                    auto key = RunDeserializer<A>::apply(std::string(p, p+(*keyLen)));
                    if (!key) {
                        return std::nullopt;
                    }
                    p += *keyLen;
                    len -= *keyLen;
                    if (len < sizeof(int32_t)) {
                        return std::nullopt;
                    }
                    auto dataLen = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
                    if (!dataLen) {
                        return std::nullopt;
                    }
                    p += sizeof(int32_t);
                    len -= sizeof(int32_t);
                    if (len < *dataLen) {
                        return std::nullopt;
                    }
                    auto data = RunDeserializer<B>::apply(std::string(p, p+(*dataLen)));
                    if (!data) {
                        return std::nullopt;
                    }
                    p += *dataLen;
                    len -= *dataLen;

                    res.insert({std::move(*key), std::move(*data)});
                }
                if (len != 0) {
                    return std::nullopt;
                }
                return {std::move(res)};
            }
        };
        template <>
        struct RunDeserializer<VoidStruct, void> {
            static std::optional<VoidStruct> apply(std::string const &data) {
                if (data.length() == 0) {
                    return VoidStruct {};
                } else {
                    return std::nullopt;
                }
            }
        };
        template <class T>
        struct RunDeserializer<SingleLayerWrapper<T>, void> {
            static std::optional<SingleLayerWrapper<T>> apply(std::string const &data) {
                auto t = RunDeserializer<T>::apply(data);
                if (!t) {
                    return std::nullopt;
                }
                return SingleLayerWrapper<T> {std::move(*t)};
            }
        };
        template <int32_t N>
        struct RunDeserializer<ConstType<N>, void> {
            static std::optional<ConstType<N>> apply(std::string const &data) {
                auto t = RunDeserializer<int32_t>::apply(data);
                if (!t) {
                    return std::nullopt;
                }
                if (*t != N) {
                    return std::nullopt;
                }
                return ConstType<N> {};
            }
        };

        template <class T, size_t N>
        struct RunCBORSerializerWithNameList {};

        template <class T>
        struct RunCBORSerializerWithNameList<T, 0> {
            static std::vector<uint8_t> apply(T const &data, std::array<std::string,0> const &) {
                return RunCBORSerializer<T>::apply(data);
            }
        };

        template <class T>
        struct RunCBORSerializerWithNameList<T, 1> {
            static std::vector<uint8_t> apply(T const &data, std::array<std::string,1> const &names) {
                auto r = RunCBORSerializer<size_t>::apply(1);
                r[0] = r[0] | 0xA0;
                for (auto const &item : data) {
                    auto r1 = RunCBORSerializer<std::string>::apply(names[0]);
                    r.insert(r.end(), r1.begin(), r1.end());
                    auto r2 = RunCBORSerializer<T>::apply(data);
                    r.insert(r.end(), r2.begin(), r2.end());
                }
                return r;
            }
        };
        template <class T, size_t N>
        struct RunCBORDeserializerWithNameList {};

        template <class T>
        struct RunCBORDeserializerWithNameList<T, 0> {
            static std::optional<std::tuple<T,size_t>> apply(std::string_view const &data, size_t start, std::array<std::string,0> const &) {
                return RunCBORDeserializer<T>::apply(data, start);
            }
        };

        template <class T>
        struct RunCBORDeserializerWithNameList<T, 1> {
            static std::optional<std::tuple<T,size_t>> apply(std::string_view const &data, size_t start, std::array<std::string,1> const &names) {
                if (data.length() < start+1) {
                    return std::nullopt;
                }
                if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0xa0) {
                    return std::nullopt;
                }
                auto v = parseCBORUnsignedInt<size_t>(data, start);
                if (!v) {
                    return std::nullopt;
                }
                if (std::get<0>(*v) != 1) {
                    return std::nullopt;
                }
                size_t len = std::get<1>(*v);
                size_t newStart = start+len;
                if (data.length() < newStart) {
                    return std::nullopt;
                }
                auto r1 = RunCBORDeserializer<std::string>::apply(data, newStart);
                if (!r1) {
                    return std::nullopt;
                }
                if (std::get<0>(*r1) != names[0]) {
                    return std::nullopt;
                }
                len += std::get<1>(*r1);
                newStart += std::get<1>(*r1);
                auto r2 = RunCBORDeserializer<T>::apply(data, newStart);
                if (!r2) {
                    return std::nullopt;
                }
                len += std::get<1>(*r2);
                return std::tuple<T,size_t>(std::move(std::get<0>(*r2)), len);
            }
        };

        #include <tm_kit/basic/ByteData_Tuple_Variant_Piece.hpp>

        template <class VersionType, class DataType, class Cmp>
        struct RunCBORSerializer<infra::VersionedData<VersionType,DataType,Cmp>> {
            static std::vector<uint8_t> apply(infra::VersionedData<VersionType,DataType,Cmp> const &data) {
                std::tuple<VersionType const *, DataType const *> t {
                    &(data.version), &(data.data)
                };
                return RunCBORSerializerWithNameList<std::tuple<VersionType const *, DataType const *>, 2>::apply(
                    t
                    , {"version", "data"}
                );
            }
        };
        template <class GroupIDType, class VersionType, class DataType, class Cmp>
        struct RunCBORSerializer<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>> {
            static std::vector<uint8_t> apply(infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> const &data) {
                std::tuple<GroupIDType const *, VersionType const *, DataType const *> t {
                    &(data.groupID), &(data.version), &(data.data)
                };
                return RunCBORSerializerWithNameList<std::tuple<GroupIDType const *, VersionType const *, DataType const *>, 3>::apply(
                    t
                    , {"groupID", "version", "data"}
                );
            }
        };
        template <class VersionType, class DataType, class Cmp>
        struct RunCBORDeserializer<infra::VersionedData<VersionType,DataType,Cmp>, void> {
            static std::optional<std::tuple<infra::VersionedData<VersionType,DataType,Cmp>,size_t>> apply(std::string_view const &data, size_t start) {
                auto x = RunCBORDeserializerWithNameList<std::tuple<VersionType,DataType>, 2>::apply(
                    data, start
                    , {"version", "data"}
                );
                if (!x) {
                    return std::nullopt;
                }
                return std::tuple<infra::VersionedData<VersionType,DataType,Cmp>,size_t> {infra::VersionedData<VersionType,DataType,Cmp> {
                    std::move(std::get<0>(std::get<0>(*x))), std::move(std::get<1>(std::get<0>(*x)))
                }, std::get<1>(*x)};
            }
        };
        template <class GroupIDType, class VersionType, class DataType, class Cmp>
        struct RunCBORDeserializer<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>, void> {
            static std::optional<std::tuple<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>,size_t>> apply(std::string_view const &data, size_t start) {
                auto x = RunCBORDeserializerWithNameList<std::tuple<GroupIDType,VersionType,DataType>, 3>::apply(
                    data, start
                    , {"groupID", "version", "data"}
                );
                if (!x) {
                    return std::nullopt;
                }
                return std::tuple<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>,size_t> {infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> {
                    std::move(std::get<0>(std::get<0>(*x))), std::move(std::get<1>(std::get<0>(*x))), std::move(std::get<2>(std::get<0>(*x)))
                }, std::get<1>(*x)};
            }
        };
        template <class VersionType, class DataType, class Cmp>
        struct RunSerializer<infra::VersionedData<VersionType,DataType,Cmp>> {
            static std::string apply(infra::VersionedData<VersionType,DataType,Cmp> const &data) {
                std::tuple<VersionType const *, DataType const *> t {
                    &(data.version), &(data.data)
                };
                return RunSerializer<std::tuple<VersionType const *, DataType const *>>::apply(t);
            }
        };
        template <class GroupIDType, class VersionType, class DataType, class Cmp>
        struct RunSerializer<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>> {
            static std::string apply(infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> const &data) {
                std::tuple<GroupIDType const *, VersionType const *, DataType const *> t {
                    &(data.groupID), &(data.version), &(data.data)
                };
                return RunSerializer<std::tuple<GroupIDType const *, VersionType const *, DataType const *>>::apply(t);
            }
        };
        template <class VersionType, class DataType, class Cmp>
        struct RunDeserializer<infra::VersionedData<VersionType,DataType,Cmp>, void> {
            static std::optional<infra::VersionedData<VersionType,DataType,Cmp>> apply(std::string const &data) {
                auto x = RunDeserializer<std::tuple<VersionType,DataType>>::apply(data);
                if (!x) {
                    return std::nullopt;
                }
                return infra::VersionedData<VersionType,DataType,Cmp> {
                    std::move(std::get<0>(*x)), std::move(std::get<1>(*x))
                };
            }
        };
        template <class GroupIDType, class VersionType, class DataType, class Cmp>
        struct RunDeserializer<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>, void> {
            static std::optional<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>> apply(std::string const &data) {
                auto x = RunDeserializer<std::tuple<GroupIDType,VersionType,DataType>>::apply(data);
                if (!x) {
                    return std::nullopt;
                }
                return infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> {
                    std::move(std::get<0>(*x)), std::move(std::get<1>(*x)), std::move(std::get<2>(*x))
                };
            }
        };

        template <class T, typename Enable>
        std::vector<uint8_t> RunCBORSerializer<T,Enable>::apply(T const &data) {
            std::string s = RunSerializer<T>::apply(data);
            return RunCBORSerializer<ByteData>::apply(ByteData {s});
        }
        template <class T, class Enable>
        std::optional<std::tuple<T,size_t>> RunCBORDeserializer<T,Enable>::apply(std::string_view const &data, size_t start) {
            auto t = RunCBORDeserializer<ByteData>::apply(data, start);
            if (t) {
                auto x = RunDeserializer<T>::apply(std::get<0>(*t).content);
                if (x) {
                    return std::tuple<T,size_t> { std::move(*x), std::get<1>(*t) };
                } else {
                    return std::nullopt;
                }     
            } else {
                return std::nullopt;
            }
        }
    }

    template <class M>
    class SerializationActions {
    public:
        template <class T>
        inline static std::string serializeFunc(T const &t) {
            return bytedata_utils::RunSerializer<T>::apply(t);
        }
        template <class T>
        inline static std::optional<T> deserializeFunc(std::string const &data) {
            return bytedata_utils::RunDeserializer<T>::apply(data);
        }
        template <class T, class F>
        static std::shared_ptr<typename M::template Action<T, TypedDataWithTopic<T>>> addTopic(F &&f) {
            return M::template liftPure<T>(
                [f=std::move(f)](T &&data) -> TypedDataWithTopic<T> {
                    return {f(data), std::move(data)};
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<T, TypedDataWithTopic<T>>> addConstTopic(std::string const &s) {
            return addTopic<T>(ConstGenerator<std::string, T>(s));
        }

        template <class T>
        static std::shared_ptr<typename M::template Action<TypedDataWithTopic<T>, ByteDataWithTopic>> serialize() {
            return M::template liftPure<TypedDataWithTopic<T>>(
                [](TypedDataWithTopic<T> &&data) -> ByteDataWithTopic {
                    return {std::move(data.topic), { bytedata_utils::RunSerializer<T>::apply(data.content) }};
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<typename M::template Key<T>, ByteDataWithID>> serializeWithKey() {
            return M::template liftPure<typename M::template Key<T>>(
                [](typename M::template Key<T> &&data) -> ByteDataWithID {
                    return {M::EnvironmentType::id_to_string(data.id()), { bytedata_utils::RunSerializer<T>::apply(data.key()) }};
                }
            );
        }
        template <class A, class B>
        static std::shared_ptr<typename M::template Action<typename M::template KeyedData<A,B>, ByteDataWithID>> serializeWithKey() {
            return M::template liftPure<typename M::template KeyedData<A,B>>(
                [](typename M::template KeyedData<A,B> &&data) -> ByteDataWithID {
                    return {M::EnvironmentType::id_to_string(data.key.id()), { bytedata_utils::RunSerializer<B>::apply(data.data) }};
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<ByteDataWithTopic, TypedDataWithTopic<T>>> deserialize() {
            return M::template liftMaybe<ByteDataWithTopic>(
                [](ByteDataWithTopic &&data) -> std::optional<TypedDataWithTopic<T>> {
                    std::optional<T> t = bytedata_utils::RunDeserializer<T>::apply(data.content);
                    if (t) {
                        return TypedDataWithTopic<T> {std::move(data.topic), std::move(*t)};
                    } else {
                        return std::nullopt;
                    }
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<ByteDataWithID, typename M::template Key<T>>> deserializeWithKey() {
            return M::template liftMaybe<ByteDataWithID>(
                [](ByteDataWithID &&data) -> std::optional<typename M::template Key<T>> {
                    std::optional<T> t = bytedata_utils::RunDeserializer<T>::apply(data.content);
                    if (t) {
                        return typename M::template Key<T> {M::EnvironmentType::id_from_string(data.id), std::move(*t)};
                    } else {
                        return std::nullopt;
                    }
                }
            );
        }
    };

} } } } 

namespace std {
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteData const &d) {
        os << "ByteData{length=" << d.content.length() << "}";
        return os; 
    }
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteDataWithTopic const &d) {
        os << "ByteDataWithTopic{topic=" << d.topic << ",length=" << d.content.length() << "}";
        return os; 
    }
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteDataWithID const &d) {
        os << "ByteDataWithID{id=" << d.id << ",length=" << d.content.length() << "}";
        return os; 
    }
    template <class T>
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::TypedDataWithTopic<T> const &d) {
        os << "TypedDataWithTopic{topic=" << d.topic << ",content=" << d.content << "}";
        return os; 
    }
};

namespace dev { namespace cd606 { namespace tm { namespace infra { namespace withtime_utils {
    template <class A>
    struct ValueCopier<basic::TypedDataWithTopic<A>> {
        inline static basic::TypedDataWithTopic<A> copy(basic::TypedDataWithTopic<A> const &a) {
            return basic::TypedDataWithTopic<A> {a.topic, ValueCopier<A>::copy(a.content)};
        }
    };
} } } } }

#endif
