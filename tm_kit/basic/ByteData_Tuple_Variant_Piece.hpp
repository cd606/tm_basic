#include <tm_kit/basic/ByteData_Tuple_Piece.hpp>

template <class A0, class A1>
struct RunCBORSerializer<std::variant<A0,A1>> {
    static std::string apply(std::variant<A0,A1> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1>
struct RunCBORDeserializer<std::variant<A0,A1>> {
    static std::optional<std::tuple<std::variant<A0,A1>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1>,size_t> { std::variant<A0,A1> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1>,size_t> { std::variant<A0,A1> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2>
struct RunCBORSerializer<std::variant<A0,A1,A2>> {
    static std::string apply(std::variant<A0,A1,A2> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2>
struct RunCBORDeserializer<std::variant<A0,A1,A2>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2>,size_t> { std::variant<A0,A1,A2> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2>,size_t> { std::variant<A0,A1,A2> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2>,size_t> { std::variant<A0,A1,A2> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3>> {
    static std::string apply(std::variant<A0,A1,A2,A3> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3>,size_t> { std::variant<A0,A1,A2,A3> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3>,size_t> { std::variant<A0,A1,A2,A3> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3>,size_t> { std::variant<A0,A1,A2,A3> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3>,size_t> { std::variant<A0,A1,A2,A3> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3,A4> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::apply(std::get<4>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3,A4> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::calculateSize(std::get<4>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3,A4>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t> { std::variant<A0,A1,A2,A3,A4> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t> { std::variant<A0,A1,A2,A3,A4> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t> { std::variant<A0,A1,A2,A3,A4> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t> { std::variant<A0,A1,A2,A3,A4> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 4:
            {
                auto res = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t> { std::variant<A0,A1,A2,A3,A4> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3,A4> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3,A4> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3,A4> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3,A4> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3,A4> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 4:
            {
                if (output.index() != 4) {
                    output = std::variant<A0,A1,A2,A3,A4> {std::in_place_index<4>, A4 {}};
                }
                auto res = RunCBORDeserializer<A4>::applyInPlace(std::get<4>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3,A4,A5> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::apply(std::get<4>(data), output+s);
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::apply(std::get<5>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3,A4,A5> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::calculateSize(std::get<4>(data));
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::calculateSize(std::get<5>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3,A4,A5>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t> { std::variant<A0,A1,A2,A3,A4,A5> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t> { std::variant<A0,A1,A2,A3,A4,A5> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t> { std::variant<A0,A1,A2,A3,A4,A5> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t> { std::variant<A0,A1,A2,A3,A4,A5> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 4:
            {
                auto res = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t> { std::variant<A0,A1,A2,A3,A4,A5> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 5:
            {
                auto res = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t> { std::variant<A0,A1,A2,A3,A4,A5> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3,A4,A5> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3,A4,A5> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3,A4,A5> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3,A4,A5> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3,A4,A5> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 4:
            {
                if (output.index() != 4) {
                    output = std::variant<A0,A1,A2,A3,A4,A5> {std::in_place_index<4>, A4 {}};
                }
                auto res = RunCBORDeserializer<A4>::applyInPlace(std::get<4>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 5:
            {
                if (output.index() != 5) {
                    output = std::variant<A0,A1,A2,A3,A4,A5> {std::in_place_index<5>, A5 {}};
                }
                auto res = RunCBORDeserializer<A5>::applyInPlace(std::get<5>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3,A4,A5,A6> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::apply(std::get<4>(data), output+s);
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::apply(std::get<5>(data), output+s);
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::apply(std::get<6>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3,A4,A5,A6> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::calculateSize(std::get<4>(data));
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::calculateSize(std::get<5>(data));
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::calculateSize(std::get<6>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 4:
            {
                auto res = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 5:
            {
                auto res = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 6:
            {
                auto res = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3,A4,A5,A6> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 4:
            {
                if (output.index() != 4) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<4>, A4 {}};
                }
                auto res = RunCBORDeserializer<A4>::applyInPlace(std::get<4>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 5:
            {
                if (output.index() != 5) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<5>, A5 {}};
                }
                auto res = RunCBORDeserializer<A5>::applyInPlace(std::get<5>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 6:
            {
                if (output.index() != 6) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6> {std::in_place_index<6>, A6 {}};
                }
                auto res = RunCBORDeserializer<A6>::applyInPlace(std::get<6>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::apply(std::get<4>(data), output+s);
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::apply(std::get<5>(data), output+s);
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::apply(std::get<6>(data), output+s);
                break;
            }
            break;
        case 7:
            {
                s += RunCBORSerializer<A7>::apply(std::get<7>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3,A4,A5,A6,A7> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::calculateSize(std::get<4>(data));
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::calculateSize(std::get<5>(data));
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::calculateSize(std::get<6>(data));
                break;
            }
            break;
        case 7:
            {
                s += RunCBORSerializer<A7>::calculateSize(std::get<7>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 4:
            {
                auto res = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 5:
            {
                auto res = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 6:
            {
                auto res = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 7:
            {
                auto res = RunCBORDeserializer<A7>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3,A4,A5,A6,A7> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 4:
            {
                if (output.index() != 4) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<4>, A4 {}};
                }
                auto res = RunCBORDeserializer<A4>::applyInPlace(std::get<4>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 5:
            {
                if (output.index() != 5) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<5>, A5 {}};
                }
                auto res = RunCBORDeserializer<A5>::applyInPlace(std::get<5>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 6:
            {
                if (output.index() != 6) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<6>, A6 {}};
                }
                auto res = RunCBORDeserializer<A6>::applyInPlace(std::get<6>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 7:
            {
                if (output.index() != 7) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7> {std::in_place_index<7>, A7 {}};
                }
                auto res = RunCBORDeserializer<A7>::applyInPlace(std::get<7>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::apply(std::get<4>(data), output+s);
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::apply(std::get<5>(data), output+s);
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::apply(std::get<6>(data), output+s);
                break;
            }
            break;
        case 7:
            {
                s += RunCBORSerializer<A7>::apply(std::get<7>(data), output+s);
                break;
            }
            break;
        case 8:
            {
                s += RunCBORSerializer<A8>::apply(std::get<8>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::calculateSize(std::get<4>(data));
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::calculateSize(std::get<5>(data));
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::calculateSize(std::get<6>(data));
                break;
            }
            break;
        case 7:
            {
                s += RunCBORSerializer<A7>::calculateSize(std::get<7>(data));
                break;
            }
            break;
        case 8:
            {
                s += RunCBORSerializer<A8>::calculateSize(std::get<8>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 4:
            {
                auto res = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 5:
            {
                auto res = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 6:
            {
                auto res = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 7:
            {
                auto res = RunCBORDeserializer<A7>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 8:
            {
                auto res = RunCBORDeserializer<A8>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 4:
            {
                if (output.index() != 4) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<4>, A4 {}};
                }
                auto res = RunCBORDeserializer<A4>::applyInPlace(std::get<4>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 5:
            {
                if (output.index() != 5) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<5>, A5 {}};
                }
                auto res = RunCBORDeserializer<A5>::applyInPlace(std::get<5>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 6:
            {
                if (output.index() != 6) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<6>, A6 {}};
                }
                auto res = RunCBORDeserializer<A6>::applyInPlace(std::get<6>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 7:
            {
                if (output.index() != 7) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<7>, A7 {}};
                }
                auto res = RunCBORDeserializer<A7>::applyInPlace(std::get<7>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 8:
            {
                if (output.index() != 8) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> {std::in_place_index<8>, A8 {}};
                }
                auto res = RunCBORDeserializer<A8>::applyInPlace(std::get<8>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char *>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(2, output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        auto idxS = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index(), output+s);
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::apply(std::get<0>(data), output+s);
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::apply(std::get<1>(data), output+s);
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::apply(std::get<2>(data), output+s);
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::apply(std::get<3>(data), output+s);
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::apply(std::get<4>(data), output+s);
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::apply(std::get<5>(data), output+s);
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::apply(std::get<6>(data), output+s);
                break;
            }
            break;
        case 7:
            {
                s += RunCBORSerializer<A7>::apply(std::get<7>(data), output+s);
                break;
            }
            break;
        case 8:
            {
                s += RunCBORSerializer<A8>::apply(std::get<8>(data), output+s);
                break;
            }
            break;
        case 9:
            {
                s += RunCBORSerializer<A9>::apply(std::get<9>(data), output+s);
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
    static std::size_t calculateSize(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(2);
        auto idxS = RunCBORSerializer<uint8_t>::calculateSize((uint8_t) data.index());
        s += idxS;
        switch (data.index()) {
        case 0:
            {
                s += RunCBORSerializer<A0>::calculateSize(std::get<0>(data));
                break;
            }
            break;
        case 1:
            {
                s += RunCBORSerializer<A1>::calculateSize(std::get<1>(data));
                break;
            }
            break;
        case 2:
            {
                s += RunCBORSerializer<A2>::calculateSize(std::get<2>(data));
                break;
            }
            break;
        case 3:
            {
                s += RunCBORSerializer<A3>::calculateSize(std::get<3>(data));
                break;
            }
            break;
        case 4:
            {
                s += RunCBORSerializer<A4>::calculateSize(std::get<4>(data));
                break;
            }
            break;
        case 5:
            {
                s += RunCBORSerializer<A5>::calculateSize(std::get<5>(data));
                break;
            }
            break;
        case 6:
            {
                s += RunCBORSerializer<A6>::calculateSize(std::get<6>(data));
                break;
            }
            break;
        case 7:
            {
                s += RunCBORSerializer<A7>::calculateSize(std::get<7>(data));
                break;
            }
            break;
        case 8:
            {
                s += RunCBORSerializer<A8>::calculateSize(std::get<8>(data));
                break;
            }
            break;
        case 9:
            {
                s += RunCBORSerializer<A9>::calculateSize(std::get<9>(data));
                break;
            }
            break;
        default:
            break;
        }
        return s;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t>> apply(std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                auto res = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 1:
            {
                auto res = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 2:
            {
                auto res = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 3:
            {
                auto res = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 4:
            {
                auto res = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 5:
            {
                auto res = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 6:
            {
                auto res = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 7:
            {
                auto res = RunCBORDeserializer<A7>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 8:
            {
                auto res = RunCBORDeserializer<A8>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        case 9:
            {
                auto res = RunCBORDeserializer<A9>::apply(data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*res);
                return std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::move(std::get<0>(*res))}, accumLen };
            }
        default:
            return std::nullopt;
        }
    }
    static std::optional<size_t> applyInPlace(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> &output,std::string_view const &data, size_t start) {
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
        auto which = parseCBORUnsignedInt<uint8_t>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);
        switch (std::get<0>(*which)) {
        case 0:
            {
                if (output.index() != 0) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<0>, A0 {}};
                }
                auto res = RunCBORDeserializer<A0>::applyInPlace(std::get<0>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 1:
            {
                if (output.index() != 1) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<1>, A1 {}};
                }
                auto res = RunCBORDeserializer<A1>::applyInPlace(std::get<1>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 2:
            {
                if (output.index() != 2) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<2>, A2 {}};
                }
                auto res = RunCBORDeserializer<A2>::applyInPlace(std::get<2>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 3:
            {
                if (output.index() != 3) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<3>, A3 {}};
                }
                auto res = RunCBORDeserializer<A3>::applyInPlace(std::get<3>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 4:
            {
                if (output.index() != 4) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<4>, A4 {}};
                }
                auto res = RunCBORDeserializer<A4>::applyInPlace(std::get<4>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 5:
            {
                if (output.index() != 5) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<5>, A5 {}};
                }
                auto res = RunCBORDeserializer<A5>::applyInPlace(std::get<5>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 6:
            {
                if (output.index() != 6) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<6>, A6 {}};
                }
                auto res = RunCBORDeserializer<A6>::applyInPlace(std::get<6>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 7:
            {
                if (output.index() != 7) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<7>, A7 {}};
                }
                auto res = RunCBORDeserializer<A7>::applyInPlace(std::get<7>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 8:
            {
                if (output.index() != 8) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<8>, A8 {}};
                }
                auto res = RunCBORDeserializer<A8>::applyInPlace(std::get<8>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        case 9:
            {
                if (output.index() != 9) {
                    output = std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {std::in_place_index<9>, A9 {}};
                }
                auto res = RunCBORDeserializer<A9>::applyInPlace(std::get<9>(output), data, start+accumLen);
                if (!res) {
                    return std::nullopt;
                }
                accumLen += *res;
                return accumLen;
            }
        default:
            return std::nullopt;
        }
    }
};