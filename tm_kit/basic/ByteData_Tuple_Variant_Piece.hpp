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
            std::move(a0)
            , std::move(a1)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
            , std::move(a4)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
            , std::move(a4)
            , std::move(a5)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
            , std::move(a4)
            , std::move(a5)
            , std::move(a6)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
            , std::move(a4)
            , std::move(a5)
            , std::move(a6)
            , std::move(a7)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
            , std::move(a4)
            , std::move(a5)
            , std::move(a6)
            , std::move(a7)
            , std::move(a8)
        };
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
            std::move(a0)
            , std::move(a1)
            , std::move(a2)
            , std::move(a3)
            , std::move(a4)
            , std::move(a5)
            , std::move(a6)
            , std::move(a7)
            , std::move(a8)
            , std::move(a9)
        };
    }
};

