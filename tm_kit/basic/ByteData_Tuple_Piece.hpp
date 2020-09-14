namespace bytedata_tuple_helper {
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    void runTupleSerializerPiece(std::ostringstream &oss, TupleType const &data) {
        std::string s = RunSerializer<FirstRemainingA>::apply(std::get<Idx>(data));
        if constexpr (sizeof...(OtherRemainingAs) == 0) {
            oss << s;
        } else {
            int32_t sSize = s.length();
            oss
                << RunSerializer<int32_t>::apply(sSize)
                << s;
            runTupleSerializerPiece<TupleType, Idx+1, OtherRemainingAs...>(oss, data);
        }
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    void runTupleCBORSerializerPiece(std::vector<uint8_t> &output, TupleType const &data) {
        auto v = RunCBORSerializer<FirstRemainingA>::apply(std::get<Idx>(data));
        output.insert(output.end(), v.begin(), v.end());
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            runTupleCBORSerializerPiece<TupleType, Idx+1, OtherRemainingAs...>(output, data);
        }
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    void runTupleCBORSerializerWithNameListPiece(std::vector<uint8_t> &output, TupleType const &data, std::array<std::string, Idx+sizeof...(OtherRemainingAs)+1> const &nameList) {
        auto s = RunCBORSerializer<std::string>::apply(nameList[Idx]);
        output.insert(output.end(), s.begin(), s.end());
        auto v = RunCBORSerializer<FirstRemainingA>::apply(std::get<Idx>(data));
        output.insert(output.end(), v.begin(), v.end());
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            runTupleCBORSerializerWithNameListPiece<TupleType, Idx+1, OtherRemainingAs...>(output, data, nameList);
        }
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    std::size_t runTupleUnsafeCBORSerializerPiece(TupleType const &data, char *output) {
        auto s = RunCBORSerializer<FirstRemainingA>::apply(std::get<Idx>(data), output);
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            s += runTupleUnsafeCBORSerializerPiece<TupleType, Idx+1, OtherRemainingAs...>(data, output+s);
        }
        return s;
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    std::size_t runTupleUnsafeCBORSerializerWithNameListPiece(TupleType const &data, std::array<std::string, Idx+sizeof...(OtherRemainingAs)+1> const &nameList, char *output) {
        auto s = RunCBORSerializer<std::string>::apply(nameList[Idx], output);
        auto v = RunCBORSerializer<FirstRemainingA>::apply(std::get<Idx>(data), output+s);
        s += v;
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            s += runTupleUnsafeCBORSerializerWithNameListPiece<TupleType, Idx+1, OtherRemainingAs...>(data, nameList, output+s);
        }
        return s;
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    std::size_t runTupleCBORSizeCalculationPiece(TupleType const &data) {
        auto s = RunCBORSerializer<FirstRemainingA>::calculateSize(std::get<Idx>(data));
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            s += runTupleCBORSizeCalculationPiece<TupleType, Idx+1, OtherRemainingAs...>(data);
        }
        return s;
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    std::size_t runTupleCBORSizeCalculationWithNameListPiece(TupleType const &data, std::array<std::string, Idx+sizeof...(OtherRemainingAs)+1> const &nameList) {
        auto s = RunCBORSerializer<std::string>::calculateSize(nameList[Idx]);
        auto v = RunCBORSerializer<FirstRemainingA>::calculateSize(std::get<Idx>(data));
        s += v;
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            s += runTupleCBORSizeCalculationWithNameListPiece<TupleType, Idx+1, OtherRemainingAs...>(data, nameList);
        }
        return s;
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    bool runTupleDeserializerPiece(TupleType &output, char const *p, size_t len) {
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            if (len < sizeof(int32_t)) {
                return false;
            }
            auto aSize = RunDeserializer<int32_t>::apply(std::string(p, p+sizeof(int32_t)));
            if (!aSize) {
                return false;
            }
            p += sizeof(int32_t);
            len -= sizeof(int32_t);
            if (len < *aSize) {
                return false;
            }
            auto a = RunDeserializer<FirstRemainingA>::apply(std::string(p, p+(*aSize)));
            if (!a) {
                return false;
            }
            std::get<Idx>(output) = std::move(*a);
            p += *aSize;
            len -= *aSize;
            return runTupleDeserializerPiece<TupleType, Idx+1, OtherRemainingAs...>(output, p, len);
        } else {
            auto a = RunDeserializer<FirstRemainingA>::apply(std::string(p, p+len));
            if (!a) {
                return false;
            }
            std::get<Idx>(output) = std::move(*a);
            return true;
        }
    }
    template <class TupleType, int Idx, class CurrentA>
    int32_t oneCBORFieldParsingFunc(TupleType &output, std::string_view const &data, size_t start, size_t &accumLen) {
        auto a = RunCBORDeserializer<CurrentA>::apply(data, start);
        if (!a) {
            return -1;
        }
        std::get<Idx>(output) = std::move(std::get<0>(*a));
        accumLen += std::get<1>(*a);
        return static_cast<int32_t>(std::get<1>(*a));
    }
    template <class TupleType, int Idx, class FirstRemainingA, class... OtherRemainingAs>
    bool runTupleCBORDeserializerPiece(TupleType &output, std::string_view const &data, size_t start, size_t &accumLen) {
        int32_t oneParsingRet = oneCBORFieldParsingFunc<TupleType, Idx, FirstRemainingA>(output, data, start, accumLen);
        if (oneParsingRet < 0) {
            return false;
        }
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            return runTupleCBORDeserializerPiece<TupleType, Idx+1, OtherRemainingAs...>(output, data, start+static_cast<size_t>(oneParsingRet), accumLen);
        } else {
            return true;
        }
    }

    template <class... As>
    using CBORFieldParsingFunc = int32_t (*)(std::tuple<As...> &output, std::string_view const &data, size_t start, size_t &accumLen);
    template <class... As>
    using CBORFieldParsingFuncMap = std::unordered_map<std::string, std::tuple<bool, CBORFieldParsingFunc<As...>>>;
    
    template <int Idx, int N, class ParserType, class MapType, class TupleType, class FirstRemainingA, class... OtherRemainingAs>
    void buildCBORParsingMapPiece(std::array<std::string, N> const &nameList, MapType &output) {
        ParserType f = &(oneCBORFieldParsingFunc<TupleType, Idx, FirstRemainingA>);
        output.insert(std::make_pair(
            nameList[Idx]
            , std::tuple<bool, ParserType> {
                false, f
            }
        ));
        if constexpr (sizeof...(OtherRemainingAs) > 0) {
            buildCBORParsingMapPiece<Idx+1, N, ParserType, MapType, TupleType, OtherRemainingAs...>(nameList, output);
        }
    }

    template <class... As>
    CBORFieldParsingFuncMap<As...> buildCBORParsingMap(std::array<std::string, sizeof...(As)> const &nameList) 
    {
        CBORFieldParsingFuncMap<As...> parsingMap;
        buildCBORParsingMapPiece<0, sizeof...(As), CBORFieldParsingFunc<As...>, CBORFieldParsingFuncMap<As...>, std::tuple<As...>, As...>(nameList, parsingMap);
        return parsingMap;
    }
}

template <>
struct RunSerializer<std::tuple<>> {
    static std::string apply(std::tuple<> const &data) {
        return "";
    }
};
template <>
struct RunCBORSerializer<std::tuple<>> {
    static std::vector<uint8_t> apply(std::tuple<> const &data) {
        return std::vector<uint8_t> {0x80};
    }
    static std::size_t apply(std::tuple<> const &data, char *output) {
        *output = static_cast<char>(0x80);
        return 1;
    }
    static std::size_t calculateSize(std::tuple<> const &data) {
        return 1;
    }
};
template <>
struct RunCBORSerializerWithNameList<std::tuple<>, 0> {
    static std::vector<uint8_t> apply(std::tuple<> const &data, std::array<std::string, 0> const &nameList) {
        return std::vector<uint8_t> {0xA0};
    }
    static std::size_t apply(std::tuple<> const &data, std::array<std::string, 0> const &nameList, char *output) {
        *output = static_cast<char>(0xA0);
        return 1;
    }
    static std::size_t calculateSize(std::tuple<> const &data, std::array<std::string, 0> const &nameList) {
        return 1;
    }
};
template <>
struct RunDeserializer<std::tuple<>> {
    static std::optional<std::tuple<>> apply(std::string const &data) {
        if (data.length() != 0) {
            return std::nullopt;
        }
        return std::tuple<> {};
    }
};
template <>
struct RunCBORDeserializer<std::tuple<>> {
    static std::optional<std::tuple<std::tuple<>,size_t>> apply(std::string_view const &data, size_t start) {
        if (data.length() < start+1) {
            return std::nullopt;
        }
        if (static_cast<uint8_t>(data[start]) != 0x80) {
            return std::nullopt;
        }
        return std::optional<std::tuple<std::tuple<>,size_t>> {
            std::tuple<std::tuple<>,size_t> {
                std::tuple<> {}
                , 1
            }
        };
    }
};
template <>
struct RunCBORDeserializerWithNameList<std::tuple<>, 0> {
    static std::optional<std::tuple<std::tuple<>,size_t>> apply(std::string_view const &data, size_t start, std::array<std::string, 0> const &nameList) {
        if (data.length() < start+1) {
            return std::nullopt;
        }
        if (static_cast<uint8_t>(data[start]) != 0xA0) {
            return std::nullopt;
        }
        return std::optional<std::tuple<std::tuple<>,size_t>> {
            std::tuple<std::tuple<>,size_t> {
                std::tuple<> {}
                , 1
            }
        };
    }
};

template <class... As>
struct RunSerializer<std::tuple<As...>> {
    static std::string apply(std::tuple<As...> const &data) {
        std::ostringstream oss;
        bytedata_tuple_helper::runTupleSerializerPiece<
            std::tuple<As...>
            , 0
            , As...
        >(oss, data);
        return oss.str();
    }
};
template <class... As>
struct RunCBORSerializer<std::tuple<As...>> {
    static std::vector<uint8_t> apply(std::tuple<As...> const &data) {
        auto v = RunCBORSerializer<size_t>::apply(sizeof...(As));
        v[0] = v[0] | 0x80;
        bytedata_tuple_helper::runTupleCBORSerializerPiece<
            std::tuple<As...>
            , 0
            , As...
        >(v, data);
        return v;
    }
    static std::size_t apply(std::tuple<As...> const &data, char *output) {
        auto s = RunCBORSerializer<size_t>::apply(sizeof...(As), output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
        s += bytedata_tuple_helper::runTupleUnsafeCBORSerializerPiece<
            std::tuple<As...>
            , 0
            , As...
        >(data, output+s);
        return s;
    }
    static std::size_t calculateSize(std::tuple<As...> const &data) {
        auto s = RunCBORSerializer<size_t>::calculateSize(sizeof...(As));
        s += bytedata_tuple_helper::runTupleCBORSizeCalculationPiece<
            std::tuple<As...>
            , 0
            , As...
        >(data);
        return s;
    }
};
template <class... As, size_t N>
struct RunCBORSerializerWithNameList<std::tuple<As...>, N> {
    static std::vector<uint8_t> apply(std::tuple<As...> const &data, std::array<std::string, N> const &nameList) {
        static_assert(N == sizeof...(As), "name list size must match tuple size");
        auto v = RunCBORSerializer<size_t>::apply(sizeof...(As));
        v[0] = v[0] | 0xA0;
        bytedata_tuple_helper::runTupleCBORSerializerWithNameListPiece<
            std::tuple<As...>
            , 0
            , As...
        >(v, data, nameList);
        return v;
    }
    static std::size_t apply(std::tuple<As...> const &data, std::array<std::string, N> const &nameList, char *output) {
        static_assert(N == sizeof...(As), "name list size must match tuple size");
        auto s = RunCBORSerializer<size_t>::apply(sizeof...(As), output);
        output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0xA0);
        s += bytedata_tuple_helper::runTupleUnsafeCBORSerializerWithNameListPiece<
            std::tuple<As...>
            , 0
            , As...
        >(data, nameList, output+s);
        return s;
    }
    static std::size_t calculateSize(std::tuple<As...> const &data, std::array<std::string, N> const &nameList) {
        static_assert(N == sizeof...(As), "name list size must match tuple size");
        auto s = RunCBORSerializer<size_t>::calculateSize(sizeof...(As));
        s += bytedata_tuple_helper::runTupleCBORSizeCalculationWithNameListPiece<
            std::tuple<As...>
            , 0
            , As...
        >(data, nameList);
        return s;
    }
};
template <class... As>
struct RunDeserializer<std::tuple<As...>> {
    static std::optional<std::tuple<As...>> apply(std::string const &data) {
        std::tuple<As...> ret; 
        if (!bytedata_tuple_helper::runTupleDeserializerPiece<
            std::tuple<As...>
            , 0
            , As...
        >(ret, data.c_str(), data.size())) {
            return std::nullopt;
        }
        return {std::move(ret)};
    }
};
template <class... As>
struct RunCBORDeserializer<std::tuple<As...>> {
    static std::optional<std::tuple<std::tuple<As...>,size_t>> apply(std::string_view const &data, size_t start) {
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
        if (std::get<0>(*n) != sizeof...(As)) {
            return std::nullopt;
        }
        size_t accumLen = std::get<1>(*n);

        std::tuple<As...> ret;
        if (!bytedata_tuple_helper::runTupleCBORDeserializerPiece<
            std::tuple<As...>
            , 0
            , As...
        >(ret, data, start+accumLen, accumLen)) {
            return std::nullopt;
        }
        return std::optional<std::tuple<std::tuple<As...>,size_t>> {
            std::tuple<std::tuple<As...>,size_t> {
                std::move(ret), accumLen
            }
        };
    }
};
template <class... As, size_t N>
struct RunCBORDeserializerWithNameList<std::tuple<As...>, N> {
    static std::optional<std::tuple<std::tuple<As...>,size_t>> apply(std::string_view const &data, size_t start, std::array<std::string, N> const &nameList) {
        static_assert(N == sizeof...(As), "name list size must match tuple size");

        auto parsingMap = bytedata_tuple_helper::buildCBORParsingMap<As...>(nameList);

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
        if (std::get<0>(*n) != sizeof...(As)) {
            return std::nullopt;
        }

        size_t accumLen = std::get<1>(*n);
        std::tuple<As...> ret;
        for (size_t ii=0; ii<sizeof...(As); ++ii) {
            auto nm = RunCBORDeserializer<std::string>::apply(data, start+accumLen);
            if (!nm) {
                return std::nullopt;
            }
            accumLen += std::get<1>(*nm);
            auto parserIter = parsingMap.find(std::get<0>(*nm));
            if (parserIter == parsingMap.end()) {
                return std::nullopt;
            }
            if (std::get<0>(parserIter->second)) {
                return std::nullopt;
            }
            if (!(std::get<1>(parserIter->second))(
                ret, data, start+accumLen, accumLen
            )) {
                return std::nullopt;
            }
            std::get<0>(parserIter->second) = true;
        }
        return std::optional<std::tuple<std::tuple<As...>,size_t>> {
            std::tuple<std::tuple<As...>,size_t> {
                std::move(ret), accumLen
            }
        };
    }
};