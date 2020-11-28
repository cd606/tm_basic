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
#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 

    struct ByteData {
        std::string content;
    };
    struct ByteDataView {
        std::string_view content;
    }; //this is not intended to be a "line" data type therefore the serialization code is not provided
       //For "line" purposes, always use ByteData instead. This type is mostly used for printing
       //since it does not require any copying
    inline ByteDataView byteDataView(ByteData const &d) {
        return ByteDataView {std::string_view {d.content}};
    }

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
        using value_type = T;
        T value;
    };
    template <class T>
    struct CBORWithMaxSizeHint {
        using value_type = T;
        T value;
        std::size_t maxSizeHint; //this is used in encoding, if this is exceeded, the program may have undefined behavior
    };

    namespace bytedata_utils {
        inline std::ostream &printByteDataDetails(std::ostream &os, ByteData const &d) {
            os << "[";
            for (size_t ii=0; ii<d.content.length(); ++ii) {
                if (ii > 0) {
                    os << ", ";
                }
                os << "0x" << std::hex << std::setw(2)
                    << std::setfill('0') << static_cast<uint16_t>(static_cast<uint8_t>(d.content[ii]))
                    << std::dec;
            }
            os << "] (" << d.content.length() << " bytes)";
            return os; 
        }
        inline std::ostream &printByteDataDetails(std::ostream &os, ByteDataView const &d) {
            os << "[";
            for (size_t ii=0; ii<d.content.length(); ++ii) {
                if (ii > 0) {
                    os << ", ";
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
            static std::string apply(T const &data);
            static std::size_t apply(T const &data, char *output); //this is the "unsafe" encoder, it assumes that output memory is already allocated and is enough, the return value is the actual used memory size
            static std::size_t calculateSize(T const &data);
        };

        template <class T>
        struct RunCBORSerializer<CBOR<T>, void> {
            static std::string apply(CBOR<T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
            static std::size_t apply(CBOR<T> const &data, char *output) {
                return RunCBORSerializer<T>::apply(data.value, output);
            }
            static std::size_t calculateSize(CBOR<T> const &data) {
                return RunCBORSerializer<T>::calculateSize(data.value);
            }
        };
        template <class T>
        struct RunCBORSerializer<CBORWithMaxSizeHint<T>, void> {
            static std::string apply(CBORWithMaxSizeHint<T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
            static std::size_t apply(CBORWithMaxSizeHint<T> const &data, char *output) {
                return RunCBORSerializer<T>::apply(data.value, output);
            }
            static std::size_t calculateSize(CBORWithMaxSizeHint<T> const &data) {
                return RunCBORSerializer<T>::calculateSize(data.value);
            }
        };
        template <class A>
        struct RunCBORSerializer<A const *, void> {
             static std::string apply(A const *data) {
                if (data) {
                    return RunCBORSerializer<A>::apply(*data);
                } else {
                    return "";
                }
            }
            static std::size_t apply(A const *data, char *output) {
                if (data) {
                    return RunCBORSerializer<A>::apply(*data, output);
                } else {
                    return 0;
                }
            }
            static std::size_t calculateSize(A const *data) {
                if (data) {
                    return RunCBORSerializer<A>::calculateSize(*data);
                } else {
                    return 0;
                }
            }
        };
        template <>
        struct RunCBORSerializer<bool, void> {
            static std::string apply(bool const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(bool const &data, char *output) {
                *output = static_cast<char>(data?0xf5:0xf4);
                return 1;
            }
            static constexpr std::size_t calculateSize(bool const &data) {
                return 1;
            }
        };
        template <>
        struct RunCBORSerializer<uint8_t, void> {
            static std::string apply(uint8_t const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(uint8_t const &data, char *output) {
                if (data <= 23) {
                    *output = static_cast<char>(data);
                    return 1;
                } else {
                    output[0] = static_cast<char>(24);
                    output[1] = static_cast<char>(data);
                    return 2;
                }
            }
            static constexpr std::size_t calculateSize(uint8_t const &data) {
                if (data <= 23) {
                    return 1;
                } else {
                    return 2;
                }
            }
        };
        template <>
        struct RunCBORSerializer<uint16_t, void> {
            static std::string apply(uint16_t const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(uint16_t const &data, char *output) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::apply(static_cast<uint8_t>(data), output);
                } else {
                    output[0] = static_cast<char>(25);
                    uint16_t bigEndianVersion = boost::endian::native_to_big<uint16_t>(data);
                    std::memcpy(&output[1], &bigEndianVersion, 2);
                    return 3;
                }
            }
            static constexpr std::size_t calculateSize(uint16_t const &data) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::calculateSize(static_cast<uint8_t>(data));
                } else {
                    return 3;
                }
            }
        };
        template <>
        struct RunCBORSerializer<uint32_t, void> {
            static std::string apply(uint32_t const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(uint32_t const &data, char *output) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::apply(static_cast<uint8_t>(data), output);
                } else if (data <= std::numeric_limits<uint16_t>::max()) {
                    return RunCBORSerializer<uint16_t>::apply(static_cast<uint16_t>(data), output);
                } else {
                    output[0] = static_cast<char>(26);
                    uint32_t bigEndianVersion = boost::endian::native_to_big<uint32_t>(data);
                    std::memcpy(&output[1], &bigEndianVersion, 4);
                    return 5;
                }
            }
            static constexpr std::size_t calculateSize(uint32_t const &data) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::calculateSize(static_cast<uint8_t>(data));
                } else if (data <= std::numeric_limits<uint16_t>::max()) {
                    return RunCBORSerializer<uint16_t>::calculateSize(static_cast<uint16_t>(data));
                } else {
                    return 5;
                }
            }
        };
        template <>
        struct RunCBORSerializer<uint64_t, void> {
            [[deprecated("use apply(T const &data, char *output) instead")]]
            static std::string apply(uint64_t const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(uint64_t const &data, char *output) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::apply(static_cast<uint8_t>(data), output);
                } else if (data <= std::numeric_limits<uint16_t>::max()) {
                    return RunCBORSerializer<uint16_t>::apply(static_cast<uint16_t>(data), output);
                } else if (data <= std::numeric_limits<uint32_t>::max()) {
                    return RunCBORSerializer<uint32_t>::apply(static_cast<uint32_t>(data), output);
                } else {
                    output[0] = static_cast<char>(27);
                    uint64_t bigEndianVersion = boost::endian::native_to_big<uint64_t>(data);
                    std::memcpy(&output[1], &bigEndianVersion, 8);
                    return 9;
                }
            }
            static constexpr std::size_t calculateSize(uint64_t const &data) {
                if (data <= std::numeric_limits<uint8_t>::max()) {
                    return RunCBORSerializer<uint8_t>::calculateSize(static_cast<uint8_t>(data));
                } else if (data <= std::numeric_limits<uint16_t>::max()) {
                    return RunCBORSerializer<uint16_t>::calculateSize(static_cast<uint16_t>(data));
                } else if (data <= std::numeric_limits<uint32_t>::max()) {
                    return RunCBORSerializer<uint32_t>::calculateSize(static_cast<uint32_t>(data));
                } else {
                    return 9;
                }
            }
        };
        template <class T>
        struct RunCBORSerializer<T, std::enable_if_t<
            std::is_integral_v<T> && std::is_signed_v<T>
            ,void
        >> {
            static std::string apply(T const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(T const &data, char *output) {
                if (data >= 0) {
                    return RunCBORSerializer<std::make_unsigned_t<T>>::apply(
                        static_cast<std::make_unsigned_t<T>>(data)
                        , output
                    );
                } else {
                    std::make_unsigned_t<T> v = static_cast<std::make_unsigned_t<T>>(-(data+1));
                    auto s = RunCBORSerializer<std::make_unsigned_t<T>>::apply(v, output);
                    output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | (uint8_t) 0x20);
                    return s;
                }
            }
            static constexpr std::size_t calculateSize(T const &data) {
                if (data >= 0) {
                    return RunCBORSerializer<std::make_unsigned_t<T>>::calculateSize(
                        static_cast<std::make_unsigned_t<T>>(data)
                    );
                } else {
                    std::make_unsigned_t<T> v = static_cast<std::make_unsigned_t<T>>(-(data+1));
                    return RunCBORSerializer<std::make_unsigned_t<T>>::calculateSize(v);
                }
            }
        };
        template <class T>
        struct RunCBORSerializer<T, std::enable_if_t<
            std::is_enum_v<T>
            ,void
        >> {
            static std::string apply(T const &data) {
                return RunCBORSerializer<std::underlying_type_t<T>>::apply(
                    static_cast<std::underlying_type_t<T>>(data)
                );
            }
            static std::size_t apply(T const &data, char *output) {
                return RunCBORSerializer<std::underlying_type_t<T>>::apply(
                    static_cast<std::underlying_type_t<T>>(data), output
                );
            }
            static constexpr std::size_t calculateSize(T const &data) {
                return RunCBORSerializer<std::underlying_type_t<T>>::calculateSize(
                    static_cast<std::underlying_type_t<T>>(data)
                );
            }
        };
        template <>
        struct RunCBORSerializer<char, void> {
            static std::string apply(char const &data) {
                return RunCBORSerializer<int8_t>::apply(static_cast<int8_t>(data));
            }
            static std::size_t apply(char const &data, char *output) {
                return RunCBORSerializer<int8_t>::apply(static_cast<int8_t>(data), output);
            }
            static constexpr std::size_t calculateSize(char const &data) {
                return RunCBORSerializer<int8_t>::calculateSize(static_cast<int8_t>(data));
            }
        };
        template <>
        struct RunCBORSerializer<float, void> {
            static std::string apply(float const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(float const &data, char *output) {
                output[0] = static_cast<char>((uint8_t) 0xE0 | (uint8_t) 26);
                uint32_t dBuf;
                std::memcpy(&dBuf, &data, 4);
                boost::endian::native_to_big_inplace<uint32_t>(dBuf);
                std::memcpy(&output[1], &dBuf, 4);
                return 5;
            }
            static constexpr std::size_t calculateSize(float const &data) {
                return 5;
            }
        };
        template <>
        struct RunCBORSerializer<double, void> {
            static std::string apply(double const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(double const &data, char *output) {
                output[0] = static_cast<char>((uint8_t) 0xE0 | (uint8_t) 27);
                uint64_t dBuf;
                std::memcpy(&dBuf, &data, 8);
                boost::endian::native_to_big_inplace<uint64_t>(dBuf);
                std::memcpy(&output[1], &dBuf, 8);
                return 9;
            }
            static constexpr std::size_t calculateSize(double const &data) {
                return 9;
            }
        };
        template <>
        struct RunCBORSerializer<std::string, void> {
            static std::string apply(std::string const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::string const &data, char *output) {
                auto prefixLen = RunCBORSerializer<size_t>::apply(data.length(), output);
                std::memcpy(output+prefixLen, data.c_str(), data.length());
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x60);
                return prefixLen+data.length();
            }
            static std::size_t calculateSize(std::string const &data) {
                auto prefixLen = RunCBORSerializer<size_t>::calculateSize(data.length());
                return prefixLen+data.length();
            }
        };
        template <>
        struct RunCBORSerializer<std::string_view, void> {
            static std::string apply(std::string_view const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::string_view const &data, char *output) {
                auto prefixLen = RunCBORSerializer<size_t>::apply(data.length(), output);
                std::memcpy(output+prefixLen, data.data(), data.length());
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x60);
                return prefixLen+data.length();
            }
            static std::size_t calculateSize(std::string_view const &data) {
                auto prefixLen = RunCBORSerializer<size_t>::calculateSize(data.length());
                return prefixLen+data.length();
            }
        };
        template <>
        struct RunCBORSerializer<ByteData, void> {
            static std::string apply(ByteData const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(ByteData const &data, char *output) {
                auto prefixLen = RunCBORSerializer<size_t>::apply(data.content.length(), output);
                std::memcpy(output+prefixLen, data.content.c_str(), data.content.length());
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x40);
                return prefixLen+data.content.length();
            }
            static std::size_t calculateSize(ByteData const &data) {
                auto prefixLen = RunCBORSerializer<size_t>::calculateSize(data.content.length());
                return prefixLen+data.content.length();
            }
        };
        template <>
        struct RunCBORSerializer<ByteDataView, void> {
            static std::string apply(ByteDataView const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(ByteDataView const &data, char *output) {
                auto prefixLen = RunCBORSerializer<size_t>::apply(data.content.length(), output);
                std::memcpy(output+prefixLen, data.content.data(), data.content.length());
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x40);
                return prefixLen+data.content.length();
            }
            static std::size_t calculateSize(ByteDataView const &data) {
                auto prefixLen = RunCBORSerializer<size_t>::calculateSize(data.content.length());
                return prefixLen+data.content.length();
            }
        };
        template <>
        struct RunCBORSerializer<ByteDataWithTopic, void> {
            static std::string apply(ByteDataWithTopic const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(ByteDataWithTopic const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(2, output);
                *output = static_cast<char>(static_cast<uint8_t>(*output) | 0x80);
                char *v1Start = output+s;
                auto s1 = RunCBORSerializer<std::string>::apply(data.topic, v1Start);
                char *v2Start = v1Start+s1;
                auto s2 = RunCBORSerializer<size_t>::apply(data.content.length(), v2Start);
                std::memcpy(v2Start+s2, data.content.c_str(), data.content.length());
                *v2Start = static_cast<char>(static_cast<uint8_t>(*v2Start) | 0x40);
                return s+s1+s2+data.content.length();
            }
            static std::size_t calculateSize(ByteDataWithTopic const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(2);
                auto s1 = RunCBORSerializer<std::string>::calculateSize(data.topic);
                auto s2 = RunCBORSerializer<size_t>::calculateSize(data.content.length());
                return s+s1+s2+data.content.length();
            }
        };
        template <>
        struct RunCBORSerializer<ByteDataWithID, void> {
            static std::string apply(ByteDataWithID const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(ByteDataWithID const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(2, output);
                *output = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                char *v1Start = output+s;
                auto s1 = RunCBORSerializer<std::string>::apply(data.id, v1Start);
                char *v2Start = v1Start+s1;
                auto s2 = RunCBORSerializer<size_t>::apply(data.content.length(), v2Start);
                std::memcpy(v2Start+s2, data.content.c_str(), data.content.length());
                *v2Start = static_cast<char>(static_cast<uint8_t>(*v2Start) | 0x40);
                return s+s1+s2+data.content.length();
            }
            static std::size_t calculateSize(ByteDataWithID const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(2);
                auto s1 = RunCBORSerializer<std::string>::calculateSize(data.id);
                auto s2 = RunCBORSerializer<size_t>::calculateSize(data.content.length());
                return s+s1+s2+data.content.length();
            }
        };
        template <class A>
        struct RunCBORSerializer<std::vector<A>, void> {
            static std::string apply(std::vector<A> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::vector<A> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item, p);
                    p += s1;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::vector<A> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item);
                }
                return s;
            }
        };
        template <class A, size_t N>
        struct RunCBORSerializer<std::array<A, N>, void> {
            static std::string apply(std::array<A, N> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::array<A, N> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item, p);
                    p += s1;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::array<A, N> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item);
                }
                return s;
            }
        };
        template <size_t N>
        struct RunCBORSerializer<std::array<char, N>, void> {
            static std::string apply(std::array<char, N> const &data) {
                return RunCBORSerializer<ByteDataView>::apply(
                    ByteDataView {std::string_view(data.data(), N)}
                );
            }
            static std::size_t apply(std::array<char, N> const &data, char *output) {
                return RunCBORSerializer<ByteDataView>::apply(
                    ByteDataView {std::string_view(data.data(), N)}
                    , output
                );
            }
            static std::size_t calculateSize(std::array<char, N> const &data) {
                return RunCBORSerializer<ByteDataView>::calculateSize(
                    ByteDataView {std::string_view(data.data(), N)}
                );
            }
        };
        template <class A>
        struct RunCBORSerializer<std::list<A>, void> {
            static std::string apply(std::list<A> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::list<A> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item, p);
                    p += s1;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::list<A> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item);
                }
                return s;
            }
        };
        template <class A, class Cmp>
        struct RunCBORSerializer<std::set<A, Cmp>, void> {
            static std::string apply(std::set<A, Cmp> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::set<A, Cmp> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item, p);
                    p += s1;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::set<A, Cmp> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item);
                }
                return s;
            }
        };
        template <class A, class Hash>
        struct RunCBORSerializer<std::unordered_set<A, Hash>, void> {
            static std::string apply(std::unordered_set<A, Hash> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::unordered_set<A, Hash> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item, p);
                    p += s1;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::unordered_set<A, Hash> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item);
                }
                return s;
            }
        };
        template <class A, class B, class Cmp>
        struct RunCBORSerializer<std::map<A, B, Cmp>, void> {
            static std::string apply(std::map<A, B, Cmp> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::map<A, B, Cmp> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0xA0);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item.first, p);
                    p += s1;
                    auto s2 = RunCBORSerializer<B>::apply(item.second, p);
                    p += s2;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::map<A, B, Cmp> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item.first);
                    s += RunCBORSerializer<B>::calculateSize(item.second);
                }
                return s;
            }
        };
        template <class A, class B, class Hash>
        struct RunCBORSerializer<std::unordered_map<A, B, Hash>, void> {
            static std::string apply(std::unordered_map<A, B, Hash> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(std::unordered_map<A, B, Hash> const &data, char *output) {
                auto s = RunCBORSerializer<size_t>::apply(data.size(), output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0xA0);
                char *p = output+s;
                for (auto const &item : data) {
                    auto s1 = RunCBORSerializer<A>::apply(item.first, p);
                    p += s1;
                    auto s2 = RunCBORSerializer<B>::apply(item.second, p);
                    p += s2;
                }
                return (p-output);
            }
            static std::size_t calculateSize(std::unordered_map<A, B, Hash> const &data) {
                auto s = RunCBORSerializer<size_t>::calculateSize(data.size());
                for (auto const &item : data) {
                    s += RunCBORSerializer<A>::calculateSize(item.first);
                    s += RunCBORSerializer<B>::calculateSize(item.second);
                }
                return s;
            }
        };
        template <class A>
        struct RunCBORSerializer<std::unique_ptr<A>, void> {
            static std::string apply(std::unique_ptr<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {data.get()});
                }
            }
            static std::size_t apply(std::unique_ptr<A> const &data, char *output) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {}, output);
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {data.get()}, output);
                }
            }
            static std::size_t calculateSize(std::unique_ptr<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::calculateSize(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::calculateSize(std::vector<A const *> {data.get()});
                }
            }
        };
        template <class A>
        struct RunCBORSerializer<std::shared_ptr<A>, void> {
            static std::string apply(std::shared_ptr<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {data.get()});
                }
            }
            static std::size_t apply(std::shared_ptr<A> const &data, char *output) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {}, output);
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {data.get()}, output);
                }
            }
            static std::size_t calculateSize(std::shared_ptr<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::calculateSize(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::calculateSize(std::vector<A const *> {data.get()});
                }
            }
        };
        template <class A>
        struct RunCBORSerializer<std::optional<A>, void> {
            static std::string apply(std::optional<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {&(*data)});
                }
            }
            static std::size_t apply(std::optional<A> const &data, char *output) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::apply(std::vector<A> {}, output);
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::apply(std::vector<A const *> {&(*data)}, output);
                }
            }
            static std::size_t calculateSize(std::optional<A> const &data) {
                if (!data) {
                    return RunCBORSerializer<std::vector<A>>::calculateSize(std::vector<A> {});
                } else {
                    return RunCBORSerializer<std::vector<A const *>>::calculateSize(std::vector<A const *> {&(*data)});
                }
            }
        };
        template <>
        struct RunCBORSerializer<VoidStruct, void> {
            static std::string apply(VoidStruct const &data) {
                return RunCBORSerializer<int32_t>::apply(0);
            }
            static std::size_t apply(VoidStruct const &data, char *output) {
                return RunCBORSerializer<int32_t>::apply(0, output);
            }
            static constexpr std::size_t calculateSize(VoidStruct const &data) {
                return RunCBORSerializer<int32_t>::calculateSize(0);
            }
        };
        template <class T>
        struct RunCBORSerializer<SingleLayerWrapper<T>, void> {
            static std::string apply(SingleLayerWrapper<T> const &data) {
                return RunCBORSerializer<T>::apply(data.value);
            }
            static std::size_t apply(SingleLayerWrapper<T> const &data, char *output) {
                return RunCBORSerializer<T>::apply(data.value, output);
            }
            static std::size_t calculateSize(SingleLayerWrapper<T> const &data) {
                return RunCBORSerializer<T>::calculateSize(data.value);
            }
        };
        template <int32_t N>
        struct RunCBORSerializer<ConstType<N>, void> {
            static std::string apply(ConstType<N> const &data) {
                return RunCBORSerializer<int32_t>::apply(N);
            }
            static std::size_t apply(ConstType<N> const &data, char *output) {
                return RunCBORSerializer<int32_t>::apply(N, output);
            }
            static constexpr std::size_t calculateSize(ConstType<N> const &data) {
                return RunCBORSerializer<int32_t>::calculateSize(N);
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
        struct RunCBORDeserializer<CBORWithMaxSizeHint<T>, void> {
            static std::optional<std::tuple<CBORWithMaxSizeHint<T>,size_t>> apply(std::string_view const &data, size_t start) {
                auto t = RunCBORDeserializer<T>::apply(data, start);
                if (t) {
                    return std::tuple<CBORWithMaxSizeHint<T>,size_t> { CBORWithMaxSizeHint<T> {std::move(std::get<0>(*t)), std::get<1>(*t)}, std::get<1>(*t) };
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
                switch (data[start])
                {
                    case (char) 0xf4:
                        return std::tuple<bool, size_t> {false, 1};
                    case (char) 0xf5:
                        return std::tuple<bool, size_t> {true, 1};
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
        template <class T>
        struct RunCBORDeserializer<T, std::enable_if_t<
            std::is_enum_v<T>
            ,void
        >> {
            static std::optional<std::tuple<T, size_t>> apply(std::string_view const &data, size_t start) {
                auto ret = RunCBORDeserializer<std::underlying_type_t<T>>::apply(data, start);
                if (ret) {
                    return std::tuple<T, size_t> {
                        static_cast<T>(std::get<0>(*ret)), std::get<1>(*ret)
                    };
                } else {
                    return std::nullopt;
                }
            }
        };
        template <>
        struct RunCBORDeserializer<char, void> {
            static std::optional<std::tuple<char, size_t>> apply(std::string_view const &data, size_t start) {
                auto ret = RunCBORDeserializer<int8_t>::apply(data, start);
                if (ret) {
                    return std::tuple<char, size_t> {
                        static_cast<char>(std::get<0>(*ret)), std::get<1>(*ret)
                    };
                } else {
                    return std::nullopt;
                }
            }
        };
        template <>
        struct RunCBORDeserializer<float, void> {
            static std::optional<std::tuple<float, size_t>> apply(std::string_view const &data, size_t start) {
                auto tryAsInt = RunCBORDeserializer<int64_t>::apply(data, start);
                if (tryAsInt) {
                    return std::tuple<float, size_t> {static_cast<float>(std::get<0>(*tryAsInt)), std::get<1>(*tryAsInt)};
                }
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
                auto tryAsFloat = RunCBORDeserializer<float>::apply(data, start);
                if (tryAsFloat) {
                    return std::tuple<double, size_t> {static_cast<double>(std::get<0>(*tryAsFloat)), std::get<1>(*tryAsFloat)};
                }
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
        template <>
        struct RunCBORDeserializer<ByteDataWithID, void> {
            static std::optional<std::tuple<ByteDataWithID, size_t>> apply(std::string_view const &data, size_t start) {
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
                auto id = RunCBORDeserializer<std::string>::apply(data, start+accumLen);
                if (!id) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*id);
                auto content = RunCBORDeserializer<ByteData>::apply(data, start+accumLen);
                if (!content) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*content);
                return std::tuple<ByteDataWithID, size_t> {
                    ByteDataWithID {std::move(std::get<0>(*id)), std::move(std::get<0>(*content).content)}
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
        template <size_t N>
        struct RunCBORDeserializer<std::array<char,N>, void> {
            static std::optional<std::tuple<std::array<char,N>, size_t>> apply(std::string_view const &data, size_t start) {
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
                if (std::get<0>(*v) != N) {
                    return std::nullopt;
                }
                if (data.length() < start+N+std::get<1>(*v)) {
                    return std::nullopt;
                }
                std::optional<std::tuple<std::array<char, N>, size_t>> ret { std::tuple<std::array<char, N>, size_t> {} };
                std::memcpy(std::get<0>(*ret).data(), data.data()+start+std::get<1>(*v), N);
                std::get<1>(*ret) = N+std::get<1>(*v);
                return ret;
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

        template <class T>
        struct DirectlySerializableChecker {
            static constexpr bool IsDirectlySerializable() {
                const auto checker1 = boost::hana::is_valid(
                    [](auto *t) -> decltype((void) (t->SerializeToString((std::string *) nullptr))) {}
                );
                const auto checker2 = boost::hana::is_valid(
                    [](auto *t) -> decltype((void) (t->ParseFromString(*((std::string const *) nullptr)))) {}
                );
                if constexpr (checker1((T *) nullptr)) {
                    if constexpr (checker2((T *) nullptr)) {
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
        struct DirectlySerializableChecker<CBOR<T>> {
            static constexpr bool IsDirectlySerializable() {
                return true;
            }
        };
        template <class T>
        struct DirectlySerializableChecker<CBORWithMaxSizeHint<T>> {
            static constexpr bool IsDirectlySerializable() {
                return true;
            }
        };
        template <>
        struct DirectlySerializableChecker<std::string> {
            static constexpr bool IsDirectlySerializable() {
                return true;
            }
        };
        template <>
        struct DirectlySerializableChecker<ByteData> {
            static constexpr bool IsDirectlySerializable() {
                return true;
            }
        };
        template <size_t N>
        struct DirectlySerializableChecker<std::array<char, N>> {
            static constexpr bool IsDirectlySerializable() {
                return true;
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
                return RunCBORSerializer<T>::apply(data.value);
            }
        };
        template <class T>
        struct RunSerializer<CBORWithMaxSizeHint<T>, void> {
            static std::string apply(CBORWithMaxSizeHint<T> const &data) {
                std::string s;
                if (data.maxSizeHint == 0) {
                    auto actualSize = RunCBORSerializer<T>::calculateSize(data.value);
                    s.resize(actualSize);
                    RunCBORSerializer<T>::apply(data.value, const_cast<char *>(s.c_str()));
                } else {
                    s.resize(data.maxSizeHint);
                    auto res = RunCBORSerializer<T>::apply(data.value, const_cast<char *>(s.c_str()));
                    s.resize(res);
                }
                return s;
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
        struct RunSerializer<std::string_view, void> {
            static std::string apply(std::string_view const &data) {
                return std::string {data};
            }
        };
        template <>
        struct RunSerializer<ByteDataView, void> {
            static std::string apply(ByteDataView const &data) {
                return std::string {data.content};
            }
        };
        template <size_t N>
        struct RunSerializer<std::array<char, N>, void> {
            static std::string apply(std::array<char, N> const &data) {
                return std::string {data.data(), N};
            }
        };
        template <class T, typename Enable=void>
        struct RunDeserializer {
            static std::optional<T> apply(std::string_view const &data) {
                return apply(std::string {data});
            }
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
            static std::optional<CBOR<T>> apply(std::string_view const &data) {
                auto r = RunCBORDeserializer<T>::apply(data, 0);
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
            static std::optional<CBOR<T>> apply(std::string const &data) {
                return apply(std::string_view {data});
            }
        };
        template <class T>
        struct RunDeserializer<CBORWithMaxSizeHint<T>, void> {
            static std::optional<CBORWithMaxSizeHint<T>> apply(std::string_view const &data) {
                auto r = RunCBORDeserializer<T>::apply(data, 0);
                if (!r) {
                    return std::nullopt;
                }
                if (std::get<1>(*r) != data.length()) {
                    return std::nullopt;
                }
                return std::optional<CBORWithMaxSizeHint<T>> {
                    CBORWithMaxSizeHint<T> {std::move(std::get<0>(*r)), data.length()}
                };
            }
            static std::optional<CBORWithMaxSizeHint<T>> apply(std::string const &data) {
                return apply(std::string_view {data});
            }
        };
        template <>
        struct RunDeserializer<std::string, void> {
            static std::optional<std::string> apply(std::string_view const &data) {
                return {std::string {data}};
            }
            static std::optional<std::string> apply(std::string const &data) {
                return {data};
            }
        };
        template <>
        struct RunDeserializer<ByteData, void> {
            static std::optional<ByteData> apply(std::string_view const &data) {
                return {ByteData {std::string {data}}};
            }
            static std::optional<ByteData> apply(std::string const &data) {
                return {ByteData {data}};
            }
        };
        template <size_t N>
        struct RunDeserializer<std::array<char, N>, void> {
            static std::optional<std::array<char, N>> apply(std::string_view const &data) {
                if (data.length() != N) {
                    return std::nullopt;
                }
                std::optional<std::array<char, N>> ret {std::array<char, N> {}};
                std::memcpy(ret->data(), data.data(), N);
                return ret;
            }
            static std::optional<std::array<char, N>> apply(std::string const &data) {
                return apply(std::string_view {data});
            }
        };

        template <class T, size_t N>
        struct RunCBORSerializerWithNameList {};
        template <class T, size_t N>
        struct RunCBORDeserializerWithNameList {};

        #include <tm_kit/basic/ByteData_Tuple_Variant_Piece.hpp>

        template <class VersionType, class DataType, class Cmp>
        struct RunCBORSerializer<infra::VersionedData<VersionType,DataType,Cmp>> {
            static std::string apply(infra::VersionedData<VersionType,DataType,Cmp> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(infra::VersionedData<VersionType,DataType,Cmp> const &data, char *output) {
                std::tuple<VersionType const *, DataType const *> t {
                    &(data.version), &(data.data)
                };
                return RunCBORSerializerWithNameList<std::tuple<VersionType const *, DataType const *>, 2>::apply(
                    t
                    , {"version", "data"}
                    , output
                );
            }
            static std::size_t calculateSize(infra::VersionedData<VersionType,DataType,Cmp> const &data) {
                std::tuple<VersionType const *, DataType const *> t {
                    &(data.version), &(data.data)
                };
                return RunCBORSerializerWithNameList<std::tuple<VersionType const *, DataType const *>, 2>::calculateSize(
                    t
                    , {"version", "data"}
                );
            }
        };
        template <class GroupIDType, class VersionType, class DataType, class Cmp>
        struct RunCBORSerializer<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>> {
            static std::string apply(infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> const &data) {
                std::string s;
                s.resize(calculateSize(data)); 
                apply(data, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> const &data, char *output) {
                std::tuple<GroupIDType const *, VersionType const *, DataType const *> t {
                    &(data.groupID), &(data.version), &(data.data)
                };
                return RunCBORSerializerWithNameList<std::tuple<GroupIDType const *, VersionType const *, DataType const *>, 3>::apply(
                    t
                    , {"groupID", "version", "data"}
                    , output
                );
            }
            static std::size_t calculateSize(infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> const &data) {
                std::tuple<GroupIDType const *, VersionType const *, DataType const *> t {
                    &(data.groupID), &(data.version), &(data.data)
                };
                return RunCBORSerializerWithNameList<std::tuple<GroupIDType const *, VersionType const *, DataType const *>, 3>::calculateSize(
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
            static std::optional<infra::VersionedData<VersionType,DataType,Cmp>> apply(std::string_view const &data) {
                auto x = RunDeserializer<std::tuple<VersionType,DataType>>::apply(data);
                if (!x) {
                    return std::nullopt;
                }
                return infra::VersionedData<VersionType,DataType,Cmp> {
                    std::move(std::get<0>(*x)), std::move(std::get<1>(*x))
                };
            }
            static std::optional<infra::VersionedData<VersionType,DataType,Cmp>> apply(std::string const &data) {
                return apply(std::string_view {data});
            }
        };
        template <class GroupIDType, class VersionType, class DataType, class Cmp>
        struct RunDeserializer<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>, void> {
            static std::optional<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>> apply(std::string_view const &data) {
                auto x = RunDeserializer<std::tuple<GroupIDType,VersionType,DataType>>::apply(data);
                if (!x) {
                    return std::nullopt;
                }
                return infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp> {
                    std::move(std::get<0>(*x)), std::move(std::get<1>(*x)), std::move(std::get<2>(*x))
                };
            }
            static std::optional<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,Cmp>> apply(std::string const &data) {
                return apply(std::string_view {data});
            }
        };

        template <class T, typename Enable>
        std::string RunCBORSerializer<T,Enable>::apply(T const &data) {
            std::string s = RunSerializer<T>::apply(data);
            return RunCBORSerializer<ByteData>::apply(ByteData {s});
        }
        template <class T, typename Enable>
        std::size_t RunCBORSerializer<T,Enable>::apply(T const &data, char *output) {
            std::string s = RunSerializer<T>::apply(data);
            return RunCBORSerializer<ByteData>::apply(ByteData {s}, output);
        }
        //This is the only case where we might have to encode the data twice
        //during the CBOR serializing, but that is unavoidable since we don't
        //know how much space the data will take without encoding it. Anyway,
        //size calculation is optional in CBOR serializing
        template <class T, typename Enable>
        std::size_t RunCBORSerializer<T,Enable>::calculateSize(T const &data) {
            std::string s = RunSerializer<T>::apply(data);
            return RunCBORSerializer<ByteData>::calculateSize(ByteData {s});
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

        template <class T>
        inline std::optional<T> extractCBORDeserializerResult(std::optional<std::tuple<T, size_t>> &&v) {
            if (!v) {
                return std::nullopt;
            }
            return std::optional<T> {std::move(std::get<0>(*v))};
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
        inline static std::optional<T> deserializeFunc(std::string_view const &data) {
            return bytedata_utils::RunDeserializer<T>::apply(data);
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
        template <class T>
        static std::shared_ptr<typename M::template Action<TypedDataWithTopic<T>,T>> removeTopic() {
            return M::template liftPure<TypedDataWithTopic<T>>(
                [](TypedDataWithTopic<T> &&data) -> T {
                    return std::move(data.content);
                }
            );
        }
    };

    template <class T>
    inline bool operator==(CBOR<T> const &a, CBOR<T> const &b) {
        return (a.value == b.value);
    }
    template <class T>
    inline bool operator==(CBORWithMaxSizeHint<T> const &a, CBORWithMaxSizeHint<T> const &b) {
        return (a.value == b.value && a.maxSizeHint == b.maxSizeHint);
    }
    inline bool operator==(ByteData const &a, ByteData const &b) {
        return (a.content == b.content);
    }
    inline bool operator==(ByteDataView const &a, ByteDataView const &b) {
        return (a.content == b.content);
    }
    inline bool operator==(ByteDataWithTopic const &a, ByteDataWithTopic const &b) {
        return (a.topic == b.topic && a.content == b.content);
    }
    inline bool operator==(ByteDataWithID const &a, ByteDataWithID const &b) {
        return (a.id == b.id && a.content == b.content);
    }
    template <class T>
    inline bool operator==(TypedDataWithTopic<T> const &a, TypedDataWithTopic<T> const &b) {
        return (a.topic == b.topic && a.content == b.content);
    }
} } } } 

namespace std {
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteData const &d) {
        os << "ByteData{length=" << d.content.length() << "}";
        return os; 
    }
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteDataView const &d) {
        os << "ByteDataView{length=" << d.content.length() << "}";
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
