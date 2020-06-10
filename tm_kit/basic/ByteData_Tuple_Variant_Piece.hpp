template <class A0, class A1>
struct RunSerializer<std::tuple<A0,A1>> {
    static std::string apply(std::tuple<A0,A1> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        int32_t sSize0 = s0.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << s1;
        return oss.str();
    }
};
template <class A0, class A1>
struct RunCBORSerializer<std::tuple<A0,A1>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        return v;
    }
};
template <class A0, class A1>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1>, 2> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1> const &data, std::array<std::string, 2> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(2);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        return v;
    }
};
template <class A0, class A1>
struct RunDeserializer<std::tuple<A0,A1>> {
    static std::optional<std::tuple<A0,A1>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+len));
        if (!a1) {
            return std::nullopt;
        }
        return std::tuple<A0,A1> {
            std::move(*a0)
            , std::move(*a1)
        };
    }
};
template <class A0, class A1>
struct RunCBORDeserializer<std::tuple<A0,A1>> {
    static std::optional<std::tuple<std::tuple<A0,A1>,size_t>> apply(std::string const &data, size_t start) {
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
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        return std::tuple<std::tuple<A0,A1>,size_t> { std::tuple<A0,A1> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
        }, accumLen};
    }
};
template <class A0, class A1>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1>, 2> {
    static std::optional<std::tuple<std::tuple<A0,A1>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,2> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
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
        if (std::get<0>(*n) != 2) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        for (size_t ii=0; ii<2; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1>,size_t> { std::tuple<A0,A1> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
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
    static std::optional<std::tuple<std::variant<A0,A1>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2>> {
    static std::string apply(std::tuple<A0,A1,A2> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << s2;
        return oss.str();
    }
};
template <class A0, class A1, class A2>
struct RunCBORSerializer<std::tuple<A0,A1,A2>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(3);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        return v;
    }
};
template <class A0, class A1, class A2>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2>, 3> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2> const &data, std::array<std::string, 3> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(3);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        return v;
    }
};
template <class A0, class A1, class A2>
struct RunDeserializer<std::tuple<A0,A1,A2>> {
    static std::optional<std::tuple<A0,A1,A2>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+len));
        if (!a2) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
        };
    }
};
template <class A0, class A1, class A2>
struct RunCBORDeserializer<std::tuple<A0,A1,A2>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 3) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        return std::tuple<std::tuple<A0,A1,A2>,size_t> { std::tuple<A0,A1,A2> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
        }, accumLen};
    }
};
template <class A0, class A1, class A2>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2>, 3> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,3> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
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
        if (std::get<0>(*n) != 3) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        for (size_t ii=0; ii<3; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2>,size_t> { std::tuple<A0,A1,A2> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3>> {
    static std::string apply(std::tuple<A0,A1,A2,A3> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << s3;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(4);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3>, 4> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3> const &data, std::array<std::string, 4> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(4);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3>
struct RunDeserializer<std::tuple<A0,A1,A2,A3>> {
    static std::optional<std::tuple<A0,A1,A2,A3>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+len));
        if (!a3) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
        };
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 4) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        return std::tuple<std::tuple<A0,A1,A2,A3>,size_t> { std::tuple<A0,A1,A2,A3> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3>, 4> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,4> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
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
        if (std::get<0>(*n) != 4) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        for (size_t ii=0; ii<4; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3>,size_t> { std::tuple<A0,A1,A2,A3> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3,A4>> {
    static std::string apply(std::tuple<A0,A1,A2,A3,A4> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        std::string s4 = RunSerializer<A4>::apply(std::get<4>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        int32_t sSize3 = s3.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << RunSerializer<int32_t>::apply(sSize3)
            << s3
            << s4;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3,A4>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(5);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3,A4>, 5> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4> const &data, std::array<std::string, 5> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(5);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto s4 = RunCBORSerializer<std::string>::apply(nameList[4]);
        v.insert(v.end(), s4.begin(), s4.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunDeserializer<std::tuple<A0,A1,A2,A3,A4>> {
    static std::optional<std::tuple<A0,A1,A2,A3,A4>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize3 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize3) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize3) {
            return std::nullopt;
        }
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+(*aSize3)));
        if (!a3) {
            return std::nullopt;
        }
        p += *aSize3;
        len -= *aSize3;
        auto a4 = RunDeserializer<A4>::apply(std::string(p, p+len));
        if (!a4) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3,A4> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
            , std::move(*a4)
        };
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3,A4>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 5) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        auto a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
        if (!a4) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a4);
        return std::tuple<std::tuple<A0,A1,A2,A3,A4>,size_t> { std::tuple<A0,A1,A2,A3,A4> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3, class A4>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3,A4>, 5> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,5> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
            , {nameList[4], 4}
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
        if (std::get<0>(*n) != 5) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        std::optional<std::tuple<A4, size_t>> a4 = std::nullopt;
        for (size_t ii=0; ii<5; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            case 4:
                if (a4) {
                    return std::nullopt;
                }
                a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!a4) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a4);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3,A4>,size_t> { std::tuple<A0,A1,A2,A3,A4> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3,A4,A5>> {
    static std::string apply(std::tuple<A0,A1,A2,A3,A4,A5> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        std::string s4 = RunSerializer<A4>::apply(std::get<4>(data));
        std::string s5 = RunSerializer<A5>::apply(std::get<5>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        int32_t sSize3 = s3.length();
        int32_t sSize4 = s4.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << RunSerializer<int32_t>::apply(sSize3)
            << s3
            << RunSerializer<int32_t>::apply(sSize4)
            << s4
            << s5;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3,A4,A5>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(6);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5>, 6> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5> const &data, std::array<std::string, 6> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(6);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto s4 = RunCBORSerializer<std::string>::apply(nameList[4]);
        v.insert(v.end(), s4.begin(), s4.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto s5 = RunCBORSerializer<std::string>::apply(nameList[5]);
        v.insert(v.end(), s5.begin(), s5.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunDeserializer<std::tuple<A0,A1,A2,A3,A4,A5>> {
    static std::optional<std::tuple<A0,A1,A2,A3,A4,A5>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize3 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize3) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize3) {
            return std::nullopt;
        }
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+(*aSize3)));
        if (!a3) {
            return std::nullopt;
        }
        p += *aSize3;
        len -= *aSize3;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize4 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize4) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize4) {
            return std::nullopt;
        }
        auto a4 = RunDeserializer<A4>::apply(std::string(p, p+(*aSize4)));
        if (!a4) {
            return std::nullopt;
        }
        p += *aSize4;
        len -= *aSize4;
        auto a5 = RunDeserializer<A5>::apply(std::string(p, p+len));
        if (!a5) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3,A4,A5> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
            , std::move(*a4)
            , std::move(*a5)
        };
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3,A4,A5>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 6) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        auto a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
        if (!a4) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a4);
        auto a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
        if (!a5) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a5);
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5>, 6> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,6> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
            , {nameList[4], 4}
            , {nameList[5], 5}
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
        if (std::get<0>(*n) != 6) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        std::optional<std::tuple<A4, size_t>> a4 = std::nullopt;
        std::optional<std::tuple<A5, size_t>> a5 = std::nullopt;
        for (size_t ii=0; ii<6; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            case 4:
                if (a4) {
                    return std::nullopt;
                }
                a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!a4) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a4);
                break;
            case 5:
                if (a5) {
                    return std::nullopt;
                }
                a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!a5) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a5);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6>> {
    static std::string apply(std::tuple<A0,A1,A2,A3,A4,A5,A6> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        std::string s4 = RunSerializer<A4>::apply(std::get<4>(data));
        std::string s5 = RunSerializer<A5>::apply(std::get<5>(data));
        std::string s6 = RunSerializer<A6>::apply(std::get<6>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        int32_t sSize3 = s3.length();
        int32_t sSize4 = s4.length();
        int32_t sSize5 = s5.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << RunSerializer<int32_t>::apply(sSize3)
            << s3
            << RunSerializer<int32_t>::apply(sSize4)
            << s4
            << RunSerializer<int32_t>::apply(sSize5)
            << s5
            << s6;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(7);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6>, 7> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6> const &data, std::array<std::string, 7> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(7);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto s4 = RunCBORSerializer<std::string>::apply(nameList[4]);
        v.insert(v.end(), s4.begin(), s4.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto s5 = RunCBORSerializer<std::string>::apply(nameList[5]);
        v.insert(v.end(), s5.begin(), s5.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto s6 = RunCBORSerializer<std::string>::apply(nameList[6]);
        v.insert(v.end(), s6.begin(), s6.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6>> {
    static std::optional<std::tuple<A0,A1,A2,A3,A4,A5,A6>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize3 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize3) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize3) {
            return std::nullopt;
        }
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+(*aSize3)));
        if (!a3) {
            return std::nullopt;
        }
        p += *aSize3;
        len -= *aSize3;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize4 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize4) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize4) {
            return std::nullopt;
        }
        auto a4 = RunDeserializer<A4>::apply(std::string(p, p+(*aSize4)));
        if (!a4) {
            return std::nullopt;
        }
        p += *aSize4;
        len -= *aSize4;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize5 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize5) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize5) {
            return std::nullopt;
        }
        auto a5 = RunDeserializer<A5>::apply(std::string(p, p+(*aSize5)));
        if (!a5) {
            return std::nullopt;
        }
        p += *aSize5;
        len -= *aSize5;
        auto a6 = RunDeserializer<A6>::apply(std::string(p, p+len));
        if (!a6) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3,A4,A5,A6> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
            , std::move(*a4)
            , std::move(*a5)
            , std::move(*a6)
        };
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 7) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        auto a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
        if (!a4) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a4);
        auto a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
        if (!a5) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a5);
        auto a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
        if (!a6) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a6);
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6>, 7> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,7> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
            , {nameList[4], 4}
            , {nameList[5], 5}
            , {nameList[6], 6}
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
        if (std::get<0>(*n) != 7) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        std::optional<std::tuple<A4, size_t>> a4 = std::nullopt;
        std::optional<std::tuple<A5, size_t>> a5 = std::nullopt;
        std::optional<std::tuple<A6, size_t>> a6 = std::nullopt;
        for (size_t ii=0; ii<7; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            case 4:
                if (a4) {
                    return std::nullopt;
                }
                a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!a4) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a4);
                break;
            case 5:
                if (a5) {
                    return std::nullopt;
                }
                a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!a5) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a5);
                break;
            case 6:
                if (a6) {
                    return std::nullopt;
                }
                a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!a6) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a6);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::string apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        std::string s4 = RunSerializer<A4>::apply(std::get<4>(data));
        std::string s5 = RunSerializer<A5>::apply(std::get<5>(data));
        std::string s6 = RunSerializer<A6>::apply(std::get<6>(data));
        std::string s7 = RunSerializer<A7>::apply(std::get<7>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        int32_t sSize3 = s3.length();
        int32_t sSize4 = s4.length();
        int32_t sSize5 = s5.length();
        int32_t sSize6 = s6.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << RunSerializer<int32_t>::apply(sSize3)
            << s3
            << RunSerializer<int32_t>::apply(sSize4)
            << s4
            << RunSerializer<int32_t>::apply(sSize5)
            << s5
            << RunSerializer<int32_t>::apply(sSize6)
            << s6
            << s7;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(8);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        auto v7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
        v.insert(v.end(), v7.begin(), v7.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>, 8> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7> const &data, std::array<std::string, 8> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(8);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto s4 = RunCBORSerializer<std::string>::apply(nameList[4]);
        v.insert(v.end(), s4.begin(), s4.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto s5 = RunCBORSerializer<std::string>::apply(nameList[5]);
        v.insert(v.end(), s5.begin(), s5.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto s6 = RunCBORSerializer<std::string>::apply(nameList[6]);
        v.insert(v.end(), s6.begin(), s6.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        auto s7 = RunCBORSerializer<std::string>::apply(nameList[7]);
        v.insert(v.end(), s7.begin(), s7.end());
        auto v7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
        v.insert(v.end(), v7.begin(), v7.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::optional<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize3 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize3) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize3) {
            return std::nullopt;
        }
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+(*aSize3)));
        if (!a3) {
            return std::nullopt;
        }
        p += *aSize3;
        len -= *aSize3;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize4 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize4) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize4) {
            return std::nullopt;
        }
        auto a4 = RunDeserializer<A4>::apply(std::string(p, p+(*aSize4)));
        if (!a4) {
            return std::nullopt;
        }
        p += *aSize4;
        len -= *aSize4;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize5 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize5) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize5) {
            return std::nullopt;
        }
        auto a5 = RunDeserializer<A5>::apply(std::string(p, p+(*aSize5)));
        if (!a5) {
            return std::nullopt;
        }
        p += *aSize5;
        len -= *aSize5;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize6 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize6) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize6) {
            return std::nullopt;
        }
        auto a6 = RunDeserializer<A6>::apply(std::string(p, p+(*aSize6)));
        if (!a6) {
            return std::nullopt;
        }
        p += *aSize6;
        len -= *aSize6;
        auto a7 = RunDeserializer<A7>::apply(std::string(p, p+len));
        if (!a7) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3,A4,A5,A6,A7> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
            , std::move(*a4)
            , std::move(*a5)
            , std::move(*a6)
            , std::move(*a7)
        };
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 8) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        auto a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
        if (!a4) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a4);
        auto a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
        if (!a5) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a5);
        auto a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
        if (!a6) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a6);
        auto a7 = RunCBORDeserializer<A7>::apply(data, start+accumLen);
        if (!a7) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a7);
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6,A7> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
            , std::move(std::get<0>(*a7))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>, 8> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,8> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
            , {nameList[4], 4}
            , {nameList[5], 5}
            , {nameList[6], 6}
            , {nameList[7], 7}
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
        if (std::get<0>(*n) != 8) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        std::optional<std::tuple<A4, size_t>> a4 = std::nullopt;
        std::optional<std::tuple<A5, size_t>> a5 = std::nullopt;
        std::optional<std::tuple<A6, size_t>> a6 = std::nullopt;
        std::optional<std::tuple<A7, size_t>> a7 = std::nullopt;
        for (size_t ii=0; ii<8; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            case 4:
                if (a4) {
                    return std::nullopt;
                }
                a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!a4) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a4);
                break;
            case 5:
                if (a5) {
                    return std::nullopt;
                }
                a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!a5) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a5);
                break;
            case 6:
                if (a6) {
                    return std::nullopt;
                }
                a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!a6) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a6);
                break;
            case 7:
                if (a7) {
                    return std::nullopt;
                }
                a7 = RunCBORDeserializer<A7>::apply(data, start+accumLen);
                if (!a7) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a7);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6,A7> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
            , std::move(std::get<0>(*a7))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::string apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        std::string s4 = RunSerializer<A4>::apply(std::get<4>(data));
        std::string s5 = RunSerializer<A5>::apply(std::get<5>(data));
        std::string s6 = RunSerializer<A6>::apply(std::get<6>(data));
        std::string s7 = RunSerializer<A7>::apply(std::get<7>(data));
        std::string s8 = RunSerializer<A8>::apply(std::get<8>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        int32_t sSize3 = s3.length();
        int32_t sSize4 = s4.length();
        int32_t sSize5 = s5.length();
        int32_t sSize6 = s6.length();
        int32_t sSize7 = s7.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << RunSerializer<int32_t>::apply(sSize3)
            << s3
            << RunSerializer<int32_t>::apply(sSize4)
            << s4
            << RunSerializer<int32_t>::apply(sSize5)
            << s5
            << RunSerializer<int32_t>::apply(sSize6)
            << s6
            << RunSerializer<int32_t>::apply(sSize7)
            << s7
            << s8;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(9);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        auto v7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
        v.insert(v.end(), v7.begin(), v7.end());
        auto v8 = RunCBORSerializer<A8>::apply(std::get<8>(data));
        v.insert(v.end(), v8.begin(), v8.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>, 9> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8> const &data, std::array<std::string, 9> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(9);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto s4 = RunCBORSerializer<std::string>::apply(nameList[4]);
        v.insert(v.end(), s4.begin(), s4.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto s5 = RunCBORSerializer<std::string>::apply(nameList[5]);
        v.insert(v.end(), s5.begin(), s5.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto s6 = RunCBORSerializer<std::string>::apply(nameList[6]);
        v.insert(v.end(), s6.begin(), s6.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        auto s7 = RunCBORSerializer<std::string>::apply(nameList[7]);
        v.insert(v.end(), s7.begin(), s7.end());
        auto v7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
        v.insert(v.end(), v7.begin(), v7.end());
        auto s8 = RunCBORSerializer<std::string>::apply(nameList[8]);
        v.insert(v.end(), s8.begin(), s8.end());
        auto v8 = RunCBORSerializer<A8>::apply(std::get<8>(data));
        v.insert(v.end(), v8.begin(), v8.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::optional<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize3 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize3) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize3) {
            return std::nullopt;
        }
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+(*aSize3)));
        if (!a3) {
            return std::nullopt;
        }
        p += *aSize3;
        len -= *aSize3;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize4 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize4) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize4) {
            return std::nullopt;
        }
        auto a4 = RunDeserializer<A4>::apply(std::string(p, p+(*aSize4)));
        if (!a4) {
            return std::nullopt;
        }
        p += *aSize4;
        len -= *aSize4;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize5 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize5) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize5) {
            return std::nullopt;
        }
        auto a5 = RunDeserializer<A5>::apply(std::string(p, p+(*aSize5)));
        if (!a5) {
            return std::nullopt;
        }
        p += *aSize5;
        len -= *aSize5;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize6 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize6) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize6) {
            return std::nullopt;
        }
        auto a6 = RunDeserializer<A6>::apply(std::string(p, p+(*aSize6)));
        if (!a6) {
            return std::nullopt;
        }
        p += *aSize6;
        len -= *aSize6;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize7 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize7) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize7) {
            return std::nullopt;
        }
        auto a7 = RunDeserializer<A7>::apply(std::string(p, p+(*aSize7)));
        if (!a7) {
            return std::nullopt;
        }
        p += *aSize7;
        len -= *aSize7;
        auto a8 = RunDeserializer<A8>::apply(std::string(p, p+len));
        if (!a8) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
            , std::move(*a4)
            , std::move(*a5)
            , std::move(*a6)
            , std::move(*a7)
            , std::move(*a8)
        };
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 9) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        auto a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
        if (!a4) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a4);
        auto a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
        if (!a5) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a5);
        auto a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
        if (!a6) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a6);
        auto a7 = RunCBORDeserializer<A7>::apply(data, start+accumLen);
        if (!a7) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a7);
        auto a8 = RunCBORDeserializer<A8>::apply(data, start+accumLen);
        if (!a8) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a8);
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
            , std::move(std::get<0>(*a7))
            , std::move(std::get<0>(*a8))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>, 9> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,9> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
            , {nameList[4], 4}
            , {nameList[5], 5}
            , {nameList[6], 6}
            , {nameList[7], 7}
            , {nameList[8], 8}
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
        if (std::get<0>(*n) != 9) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        std::optional<std::tuple<A4, size_t>> a4 = std::nullopt;
        std::optional<std::tuple<A5, size_t>> a5 = std::nullopt;
        std::optional<std::tuple<A6, size_t>> a6 = std::nullopt;
        std::optional<std::tuple<A7, size_t>> a7 = std::nullopt;
        std::optional<std::tuple<A8, size_t>> a8 = std::nullopt;
        for (size_t ii=0; ii<9; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            case 4:
                if (a4) {
                    return std::nullopt;
                }
                a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!a4) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a4);
                break;
            case 5:
                if (a5) {
                    return std::nullopt;
                }
                a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!a5) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a5);
                break;
            case 6:
                if (a6) {
                    return std::nullopt;
                }
                a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!a6) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a6);
                break;
            case 7:
                if (a7) {
                    return std::nullopt;
                }
                a7 = RunCBORDeserializer<A7>::apply(data, start+accumLen);
                if (!a7) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a7);
                break;
            case 8:
                if (a8) {
                    return std::nullopt;
                }
                a8 = RunCBORDeserializer<A8>::apply(data, start+accumLen);
                if (!a8) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a8);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
            , std::move(std::get<0>(*a7))
            , std::move(std::get<0>(*a8))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8>,size_t>> apply(std::string const &data, size_t start) {
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
struct RunSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::string apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data) {
        std::string s0 = RunSerializer<A0>::apply(std::get<0>(data));
        std::string s1 = RunSerializer<A1>::apply(std::get<1>(data));
        std::string s2 = RunSerializer<A2>::apply(std::get<2>(data));
        std::string s3 = RunSerializer<A3>::apply(std::get<3>(data));
        std::string s4 = RunSerializer<A4>::apply(std::get<4>(data));
        std::string s5 = RunSerializer<A5>::apply(std::get<5>(data));
        std::string s6 = RunSerializer<A6>::apply(std::get<6>(data));
        std::string s7 = RunSerializer<A7>::apply(std::get<7>(data));
        std::string s8 = RunSerializer<A8>::apply(std::get<8>(data));
        std::string s9 = RunSerializer<A9>::apply(std::get<9>(data));
        int32_t sSize0 = s0.length();
        int32_t sSize1 = s1.length();
        int32_t sSize2 = s2.length();
        int32_t sSize3 = s3.length();
        int32_t sSize4 = s4.length();
        int32_t sSize5 = s5.length();
        int32_t sSize6 = s6.length();
        int32_t sSize7 = s7.length();
        int32_t sSize8 = s8.length();
        std::ostringstream oss;
        oss
            << RunSerializer<int32_t>::apply(sSize0)
            << s0
            << RunSerializer<int32_t>::apply(sSize1)
            << s1
            << RunSerializer<int32_t>::apply(sSize2)
            << s2
            << RunSerializer<int32_t>::apply(sSize3)
            << s3
            << RunSerializer<int32_t>::apply(sSize4)
            << s4
            << RunSerializer<int32_t>::apply(sSize5)
            << s5
            << RunSerializer<int32_t>::apply(sSize6)
            << s6
            << RunSerializer<int32_t>::apply(sSize7)
            << s7
            << RunSerializer<int32_t>::apply(sSize8)
            << s8
            << s9;
        return oss.str();
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORSerializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(10);
        v[0] = v[0] | 0x80;
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        auto v7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
        v.insert(v.end(), v7.begin(), v7.end());
        auto v8 = RunCBORSerializer<A8>::apply(std::get<8>(data));
        v.insert(v.end(), v8.begin(), v8.end());
        auto v9 = RunCBORSerializer<A9>::apply(std::get<9>(data));
        v.insert(v.end(), v9.begin(), v9.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORSerializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>, 10> {
    static std::vector<uint8_t> apply(std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> const &data, std::array<std::string, 10> const &nameList) {
        auto v = RunCBORSerializer<size_t>::apply(10);
        v[0] = v[0] | 0xA0;
        auto s0 = RunCBORSerializer<std::string>::apply(nameList[0]);
        v.insert(v.end(), s0.begin(), s0.end());
        auto v0 = RunCBORSerializer<A0>::apply(std::get<0>(data));
        v.insert(v.end(), v0.begin(), v0.end());
        auto s1 = RunCBORSerializer<std::string>::apply(nameList[1]);
        v.insert(v.end(), s1.begin(), s1.end());
        auto v1 = RunCBORSerializer<A1>::apply(std::get<1>(data));
        v.insert(v.end(), v1.begin(), v1.end());
        auto s2 = RunCBORSerializer<std::string>::apply(nameList[2]);
        v.insert(v.end(), s2.begin(), s2.end());
        auto v2 = RunCBORSerializer<A2>::apply(std::get<2>(data));
        v.insert(v.end(), v2.begin(), v2.end());
        auto s3 = RunCBORSerializer<std::string>::apply(nameList[3]);
        v.insert(v.end(), s3.begin(), s3.end());
        auto v3 = RunCBORSerializer<A3>::apply(std::get<3>(data));
        v.insert(v.end(), v3.begin(), v3.end());
        auto s4 = RunCBORSerializer<std::string>::apply(nameList[4]);
        v.insert(v.end(), s4.begin(), s4.end());
        auto v4 = RunCBORSerializer<A4>::apply(std::get<4>(data));
        v.insert(v.end(), v4.begin(), v4.end());
        auto s5 = RunCBORSerializer<std::string>::apply(nameList[5]);
        v.insert(v.end(), s5.begin(), s5.end());
        auto v5 = RunCBORSerializer<A5>::apply(std::get<5>(data));
        v.insert(v.end(), v5.begin(), v5.end());
        auto s6 = RunCBORSerializer<std::string>::apply(nameList[6]);
        v.insert(v.end(), s6.begin(), s6.end());
        auto v6 = RunCBORSerializer<A6>::apply(std::get<6>(data));
        v.insert(v.end(), v6.begin(), v6.end());
        auto s7 = RunCBORSerializer<std::string>::apply(nameList[7]);
        v.insert(v.end(), s7.begin(), s7.end());
        auto v7 = RunCBORSerializer<A7>::apply(std::get<7>(data));
        v.insert(v.end(), v7.begin(), v7.end());
        auto s8 = RunCBORSerializer<std::string>::apply(nameList[8]);
        v.insert(v.end(), s8.begin(), s8.end());
        auto v8 = RunCBORSerializer<A8>::apply(std::get<8>(data));
        v.insert(v.end(), v8.begin(), v8.end());
        auto s9 = RunCBORSerializer<std::string>::apply(nameList[9]);
        v.insert(v.end(), s9.begin(), s9.end());
        auto v9 = RunCBORSerializer<A9>::apply(std::get<9>(data));
        v.insert(v.end(), v9.begin(), v9.end());
        return v;
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::optional<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> apply(std::string const &data) {
        const char *p = data.c_str();
        size_t len = data.length();
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize0 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize0) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize0) {
            return std::nullopt;
        }
        auto a0 = RunDeserializer<A0>::apply(std::string(p, p+(*aSize0)));
        if (!a0) {
            return std::nullopt;
        }
        p += *aSize0;
        len -= *aSize0;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize1 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize1) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize1) {
            return std::nullopt;
        }
        auto a1 = RunDeserializer<A1>::apply(std::string(p, p+(*aSize1)));
        if (!a1) {
            return std::nullopt;
        }
        p += *aSize1;
        len -= *aSize1;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize2 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize2) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize2) {
            return std::nullopt;
        }
        auto a2 = RunDeserializer<A2>::apply(std::string(p, p+(*aSize2)));
        if (!a2) {
            return std::nullopt;
        }
        p += *aSize2;
        len -= *aSize2;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize3 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize3) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize3) {
            return std::nullopt;
        }
        auto a3 = RunDeserializer<A3>::apply(std::string(p, p+(*aSize3)));
        if (!a3) {
            return std::nullopt;
        }
        p += *aSize3;
        len -= *aSize3;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize4 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize4) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize4) {
            return std::nullopt;
        }
        auto a4 = RunDeserializer<A4>::apply(std::string(p, p+(*aSize4)));
        if (!a4) {
            return std::nullopt;
        }
        p += *aSize4;
        len -= *aSize4;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize5 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize5) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize5) {
            return std::nullopt;
        }
        auto a5 = RunDeserializer<A5>::apply(std::string(p, p+(*aSize5)));
        if (!a5) {
            return std::nullopt;
        }
        p += *aSize5;
        len -= *aSize5;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize6 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize6) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize6) {
            return std::nullopt;
        }
        auto a6 = RunDeserializer<A6>::apply(std::string(p, p+(*aSize6)));
        if (!a6) {
            return std::nullopt;
        }
        p += *aSize6;
        len -= *aSize6;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize7 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize7) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize7) {
            return std::nullopt;
        }
        auto a7 = RunDeserializer<A7>::apply(std::string(p, p+(*aSize7)));
        if (!a7) {
            return std::nullopt;
        }
        p += *aSize7;
        len -= *aSize7;
        if (len < sizeof(int32_t)) {
            return std::nullopt;
        }
        auto aSize8 = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
        if (!aSize8) {
            return std::nullopt;
        }
        p += sizeof(int32_t);
        len -= sizeof(int32_t);
        if (len < *aSize8) {
            return std::nullopt;
        }
        auto a8 = RunDeserializer<A8>::apply(std::string(p, p+(*aSize8)));
        if (!a8) {
            return std::nullopt;
        }
        p += *aSize8;
        len -= *aSize8;
        auto a9 = RunDeserializer<A9>::apply(std::string(p, p+len));
        if (!a9) {
            return std::nullopt;
        }
        return std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {
            std::move(*a0)
            , std::move(*a1)
            , std::move(*a2)
            , std::move(*a3)
            , std::move(*a4)
            , std::move(*a5)
            , std::move(*a6)
            , std::move(*a7)
            , std::move(*a8)
            , std::move(*a9)
        };
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORDeserializer<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t>> apply(std::string const &data, size_t start) {
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
        if (std::get<0>(*n) != 10) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        auto a0 = RunCBORDeserializer<A0>::apply(data, start+accumLen);
        if (!a0) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a0);
        auto a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
        if (!a1) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a1);
        auto a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
        if (!a2) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a2);
        auto a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
        if (!a3) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a3);
        auto a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
        if (!a4) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a4);
        auto a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
        if (!a5) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a5);
        auto a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
        if (!a6) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a6);
        auto a7 = RunCBORDeserializer<A7>::apply(data, start+accumLen);
        if (!a7) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a7);
        auto a8 = RunCBORDeserializer<A8>::apply(data, start+accumLen);
        if (!a8) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a8);
        auto a9 = RunCBORDeserializer<A9>::apply(data, start+accumLen);
        if (!a9) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*a9);
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
            , std::move(std::get<0>(*a7))
            , std::move(std::get<0>(*a8))
            , std::move(std::get<0>(*a9))
        }, accumLen};
    }
};
template <class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct RunCBORDeserializerWithNameList<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>, 10> {
    static std::optional<std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t>> apply(std::string const &data, size_t start, std::array<std::string,10> const &nameList) {
        std::unordered_map<std::string, size_t> nameMap {
            {nameList[0], 0}
            , {nameList[1], 1}
            , {nameList[2], 2}
            , {nameList[3], 3}
            , {nameList[4], 4}
            , {nameList[5], 5}
            , {nameList[6], 6}
            , {nameList[7], 7}
            , {nameList[8], 8}
            , {nameList[9], 9}
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
        if (std::get<0>(*n) != 10) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);
        std::optional<std::tuple<A0, size_t>> a0 = std::nullopt;
        std::optional<std::tuple<A1, size_t>> a1 = std::nullopt;
        std::optional<std::tuple<A2, size_t>> a2 = std::nullopt;
        std::optional<std::tuple<A3, size_t>> a3 = std::nullopt;
        std::optional<std::tuple<A4, size_t>> a4 = std::nullopt;
        std::optional<std::tuple<A5, size_t>> a5 = std::nullopt;
        std::optional<std::tuple<A6, size_t>> a6 = std::nullopt;
        std::optional<std::tuple<A7, size_t>> a7 = std::nullopt;
        std::optional<std::tuple<A8, size_t>> a8 = std::nullopt;
        std::optional<std::tuple<A9, size_t>> a9 = std::nullopt;
        for (size_t ii=0; ii<10; ++ii) {
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
            case 1:
                if (a1) {
                    return std::nullopt;
                }
                a1 = RunCBORDeserializer<A1>::apply(data, start+accumLen);
                if (!a1) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a1);
                break;
            case 2:
                if (a2) {
                    return std::nullopt;
                }
                a2 = RunCBORDeserializer<A2>::apply(data, start+accumLen);
                if (!a2) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a2);
                break;
            case 3:
                if (a3) {
                    return std::nullopt;
                }
                a3 = RunCBORDeserializer<A3>::apply(data, start+accumLen);
                if (!a3) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a3);
                break;
            case 4:
                if (a4) {
                    return std::nullopt;
                }
                a4 = RunCBORDeserializer<A4>::apply(data, start+accumLen);
                if (!a4) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a4);
                break;
            case 5:
                if (a5) {
                    return std::nullopt;
                }
                a5 = RunCBORDeserializer<A5>::apply(data, start+accumLen);
                if (!a5) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a5);
                break;
            case 6:
                if (a6) {
                    return std::nullopt;
                }
                a6 = RunCBORDeserializer<A6>::apply(data, start+accumLen);
                if (!a6) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a6);
                break;
            case 7:
                if (a7) {
                    return std::nullopt;
                }
                a7 = RunCBORDeserializer<A7>::apply(data, start+accumLen);
                if (!a7) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a7);
                break;
            case 8:
                if (a8) {
                    return std::nullopt;
                }
                a8 = RunCBORDeserializer<A8>::apply(data, start+accumLen);
                if (!a8) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a8);
                break;
            case 9:
                if (a9) {
                    return std::nullopt;
                }
                a9 = RunCBORDeserializer<A9>::apply(data, start+accumLen);
                if (!a9) {
                    return std::nullopt;
                }
                accumLen += std::get<1>(*a9);
                break;
            default:
                return std::nullopt;
            }
        }
        return std::tuple<std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t> { std::tuple<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9> {
            std::move(std::get<0>(*a0))
            , std::move(std::get<0>(*a1))
            , std::move(std::get<0>(*a2))
            , std::move(std::get<0>(*a3))
            , std::move(std::get<0>(*a4))
            , std::move(std::get<0>(*a5))
            , std::move(std::get<0>(*a6))
            , std::move(std::get<0>(*a7))
            , std::move(std::get<0>(*a8))
            , std::move(std::get<0>(*a9))
        }, accumLen};
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
    static std::optional<std::tuple<std::variant<A0,A1,A2,A3,A4,A5,A6,A7,A8,A9>,size_t>> apply(std::string const &data, size_t start) {
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