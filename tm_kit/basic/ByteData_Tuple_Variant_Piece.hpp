#include <tm_kit/basic/ByteData_Tuple_Piece.hpp>

template <class A0>
struct RunSerializer<std::tuple<A0>> {
    static std::string apply(std::tuple<A0> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        
        std::ostringstream oss;
        oss
            
            << s0;
        return oss.str();
    }
};
template <class A0>
struct RunCBORSerializer<std::tuple<A0>> {
    static std::vector<uint8_t> apply(std::tuple<A0> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(1);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        return v;
    }
};
template <class A0>
struct RunCBORSerializerWithNameList<std::tuple<A0>, 1> {
    static std::vector<uint8_t> apply(std::tuple<A0> const &data, std::array<std::string, 1> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(1);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        return v;
    }
};
template <class A0>
struct RunDeserializer<std::tuple<A0>> {
    static std::optional<std::tuple<A0>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+len));
        if (!a0) {
            return std::nullopt;
        }
        return std::tuple<A0> {
            std::move(*a0)
        };
    }
};
template <class A0>
struct RunCBORDeserializer<std::tuple<A0>> {
    static std::optional<std::tuple<std::tuple<A0>,size_t>> apply(std::string_view const &data, size_t start) {
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
        if (std::get<0>(*n) != 1) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        return std::tuple<std::tuple<A0>,size_t> { std::tuple<A0> {
            std::move(std::get<0>(*a0))
        }, accumLen};
    }
};
template <class A0>
struct RunCBORDeserializerWithNameList<std::tuple<A0>, 1> {
    static std::optional<std::tuple<std::tuple<A0>,size_t>> apply(std::string_view const &data, size_t start, std::array<std::string,1> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
        };
        if (data.length() < start+1) {
            return std::nullopt;
        }
        if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0xa0) {
            return std::nullopt;
        }
        auto n = parseCBORUnsignedInt<size_t>(data, start);
        if (!n) {
            return std::nullopt;
        }
        if (std::get<0>(*n) != 1) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        for (size_t ii=0; ii<1; ++ii) {
            auto nm = RunCBORDeserializer<std::string>::apply(data, start+accumLen);
            if (!nm) {
                return std::nullopt;
            }
            accumLen += std::get<1>(*nm);
            auto nameIter = nameMap.find(std::get<0>(*nm));
            if (nameIter == nameMap.end()) {
                return std::nullopt;
            }
            switch (nameIter->second) {
            case 0:
                if (a0) {
                    return std::nullopt;
                }
                a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
                if (!a0) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a0);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0>,size_t> { std::tuple<A0> {
            std::move(std::get<0>(*a0))
        }, accumLen};
    }
};
template <class A0, class A1>
struct RunSerializer<std::variant<A0,A1>> {
    static std::string apply(std::variant<A0,A1> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1>
struct RunCBORSerializer<std::variant<A0,A1>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1>
struct RunDeserializer<std::variant<A0,A1>> {
    static std::optional<std::variant<A0,A1>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2>
struct RunSerializer<std::variant<A0,A1,A2>> {
    static std::string apply(std::variant<A0,A1,A2> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2>
struct RunCBORSerializer<std::variant<A0,A1,A2>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2>
struct RunDeserializer<std::variant<A0,A1,A2>> {
    static std::optional<std::variant<A0,A1,A2>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3>
struct RunSerializer<std::variant<A0,A1,A2,A3>> {
    static std::string apply(std::variant<A0,A1,A2,A3> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3>
struct RunDeserializer<std::variant<A0,A1,A2,A3>> {
    static std::optional<std::variant<A0,A1,A2,A3>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunSerializer<std::variant<A0,A1,A2,A3,A4>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        case 4:
            oss << RunSerializer<A4>::apply(std::get<4>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3,A4> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        case 4:
            {
                auto a4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
                v.insert(v.end(), a4.begin(), a4.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunDeserializer<std::variant<A0,A1,A2,A3,A4>> {
    static std::optional<std::variant<A0,A1,A2,A3,A4>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4> { std::move(*res) };
            }
        case 4:
            {
                auto res = RunDeserializer<A4>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunSerializer<std::variant<A0,A1,A2,A3,A4,A5>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        case 4:
            oss << RunSerializer<A4>::apply(std::get<4>(data));
            break;
        case 5:
            oss << RunSerializer<A5>::apply(std::get<5>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3,A4,A5> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        case 4:
            {
                auto a4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
                v.insert(v.end(), a4.begin(), a4.end());
                break;
            }
            break;
        case 5:
            {
                auto a5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
                v.insert(v.end(), a5.begin(), a5.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunDeserializer<std::variant<A0,A1,A2,A3,A4,A5>> {
    static std::optional<std::variant<A0,A1,A2,A3,A4,A5>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5> { std::move(*res) };
            }
        case 4:
            {
                auto res = RunDeserializer<A4>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5> { std::move(*res) };
            }
        case 5:
            {
                auto res = RunDeserializer<A5>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        case 4:
            oss << RunSerializer<A4>::apply(std::get<4>(data));
            break;
        case 5:
            oss << RunSerializer<A5>::apply(std::get<5>(data));
            break;
        case 6:
            oss << RunSerializer<A6>::apply(std::get<6>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3,A4,A5,A6> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        case 4:
            {
                auto a4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
                v.insert(v.end(), a4.begin(), a4.end());
                break;
            }
            break;
        case 5:
            {
                auto a5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
                v.insert(v.end(), a5.begin(), a5.end());
                break;
            }
            break;
        case 6:
            {
                auto a6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
                v.insert(v.end(), a6.begin(), a6.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6>> {
    static std::optional<std::variant<A0,A1,A2,A3,A4,A5,A6>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        case 4:
            {
                auto res = RunDeserializer<A4>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        case 5:
            {
                auto res = RunDeserializer<A5>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        case 6:
            {
                auto res = RunDeserializer<A6>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        case 4:
            oss << RunSerializer<A4>::apply(std::get<4>(data));
            break;
        case 5:
            oss << RunSerializer<A5>::apply(std::get<5>(data));
            break;
        case 6:
            oss << RunSerializer<A6>::apply(std::get<6>(data));
            break;
        case 7:
            oss << RunSerializer<A7>::apply(std::get<7>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        case 4:
            {
                auto a4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
                v.insert(v.end(), a4.begin(), a4.end());
                break;
            }
            break;
        case 5:
            {
                auto a5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
                v.insert(v.end(), a5.begin(), a5.end());
                break;
            }
            break;
        case 6:
            {
                auto a6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
                v.insert(v.end(), a6.begin(), a6.end());
                break;
            }
            break;
        case 7:
            {
                auto a7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
                v.insert(v.end(), a7.begin(), a7.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::optional<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 4:
            {
                auto res = RunDeserializer<A4>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 5:
            {
                auto res = RunDeserializer<A5>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 6:
            {
                auto res = RunDeserializer<A6>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        case 7:
            {
                auto res = RunDeserializer<A7>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        case 4:
            oss << RunSerializer<A4>::apply(std::get<4>(data));
            break;
        case 5:
            oss << RunSerializer<A5>::apply(std::get<5>(data));
            break;
        case 6:
            oss << RunSerializer<A6>::apply(std::get<6>(data));
            break;
        case 7:
            oss << RunSerializer<A7>::apply(std::get<7>(data));
            break;
        case 8:
            oss << RunSerializer<A8>::apply(std::get<8>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        case 4:
            {
                auto a4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
                v.insert(v.end(), a4.begin(), a4.end());
                break;
            }
            break;
        case 5:
            {
                auto a5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
                v.insert(v.end(), a5.begin(), a5.end());
                break;
            }
            break;
        case 6:
            {
                auto a6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
                v.insert(v.end(), a6.begin(), a6.end());
                break;
            }
            break;
        case 7:
            {
                auto a7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
                v.insert(v.end(), a7.begin(), a7.end());
                break;
            }
            break;
        case 8:
            {
                auto a8 = RunCBORSerializer<A8>::apply(std::get<8>(data));
                v.insert(v.end(), a8.begin(), a8.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::optional<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 4:
            {
                auto res = RunDeserializer<A4>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 5:
            {
                auto res = RunDeserializer<A5>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 6:
            {
                auto res = RunDeserializer<A6>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 7:
            {
                auto res = RunDeserializer<A7>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        case 8:
            {
                auto res = RunDeserializer<A8>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::string apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data) {
        std::ostringstream oss;
        oss << RunSerializer<int8_t>::apply((int8_t) data.index());
        switch (data.index()) {
        case 0:
            oss << RunSerializer<A0>::apply(std::get<0>(data));
            break;
        case 1:
            oss << RunSerializer<A1>::apply(std::get<1>(data));
            break;
        case 2:
            oss << RunSerializer<A2>::apply(std::get<2>(data));
            break;
        case 3:
            oss << RunSerializer<A3>::apply(std::get<3>(data));
            break;
        case 4:
            oss << RunSerializer<A4>::apply(std::get<4>(data));
            break;
        case 5:
            oss << RunSerializer<A5>::apply(std::get<5>(data));
            break;
        case 6:
            oss << RunSerializer<A6>::apply(std::get<6>(data));
            break;
        case 7:
            oss << RunSerializer<A7>::apply(std::get<7>(data));
            break;
        case 8:
            oss << RunSerializer<A8>::apply(std::get<8>(data));
            break;
        case 9:
            oss << RunSerializer<A9>::apply(std::get<9>(data));
            break;
        default:
            break;
        }
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORSerializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::vector<uint8_t> apply(std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto idx = RunCBORSerializer<uint8_t>::apply((uint8_t) data.index());
        v.insert(v.end(), idx.begin(), idx.end());
        switch (data.index()) {
        case 0:
            {
                auto a0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
                v.insert(v.end(), a0.begin(), a0.end());
                break;
            }
            break;
        case 1:
            {
                auto a1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
                v.insert(v.end(), a1.begin(), a1.end());
                break;
            }
            break;
        case 2:
            {
                auto a2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
                v.insert(v.end(), a2.begin(), a2.end());
                break;
            }
            break;
        case 3:
            {
                auto a3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
                v.insert(v.end(), a3.begin(), a3.end());
                break;
            }
            break;
        case 4:
            {
                auto a4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
                v.insert(v.end(), a4.begin(), a4.end());
                break;
            }
            break;
        case 5:
            {
                auto a5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
                v.insert(v.end(), a5.begin(), a5.end());
                break;
            }
            break;
        case 6:
            {
                auto a6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
                v.insert(v.end(), a6.begin(), a6.end());
                break;
            }
            break;
        case 7:
            {
                auto a7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
                v.insert(v.end(), a7.begin(), a7.end());
                break;
            }
            break;
        case 8:
            {
                auto a8 = RunCBORSerializer<A8>::apply(std::get<8>(data));
                v.insert(v.end(), a8.begin(), a8.end());
                break;
            }
            break;
        case 9:
            {
                auto a9 = RunCBORSerializer<A9>::apply(std::get<9>(data));
                v.insert(v.end(), a9.begin(), a9.end());
                break;
            }
            break;
        default:
            break;
        }
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunDeserializer<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::optional<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int8_t)) {
            return std::nullopt;
        }
        auto which = RunDeserializer<int8_t>::apply(std::string(p, p+sizeof(int8_t)));
        if (!which) {
            return std::nullopt;
        }
        p += sizeof(int8_t);
        len -= sizeof(int8_t);
        switch (*which) {
        case 0:
            {
                auto res = RunDeserializer<A0>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 1:
            {
                auto res = RunDeserializer<A1>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 2:
            {
                auto res = RunDeserializer<A2>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 3:
            {
                auto res = RunDeserializer<A3>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 4:
            {
                auto res = RunDeserializer<A4>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 5:
            {
                auto res = RunDeserializer<A5>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 6:
            {
                auto res = RunDeserializer<A6>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 7:
            {
                auto res = RunDeserializer<A7>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 8:
            {
                auto res = RunDeserializer<A8>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        case 9:
            {
                auto res = RunDeserializer<A9>::apply(std::string(p, p+len));
                if (!res) {
                    return std::nullopt;
                }
                return std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> { std::move(*res) };
            }
        default:
            return std::nullopt;
        }
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
};