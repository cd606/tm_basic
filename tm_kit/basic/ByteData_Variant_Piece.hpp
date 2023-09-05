template <typename... Args>
struct RunCBORSerializer<std::variant<Args...>> {
    using IndexType = uint8_t;
    static_assert(sizeof...(Args) <= std::numeric_limits<IndexType>::max(), "variant size exceeds max numeric limits of index type");

    static std::string apply(std::variant<Args...> const& data) {
        std::string s;
        s.resize(calculateSize(data));
        apply(data, const_cast<char*>(s.data()));
        return s;
    }
    static std::size_t apply(std::variant<Args...> const& data, char* output) {
        return std::visit(
            [output, index=data.index()](auto const& x) {
                using T = std::decay_t<decltype(x)>;
                auto s = RunCBORSerializer<std::size_t>::apply(2, output);
                output[0] = static_cast<char>(static_cast<uint8_t>(output[0]) | 0x80);
                auto idxS = RunCBORSerializer<IndexType>::apply((IndexType)index, output+s);
                s += idxS;
                s += RunCBORSerializer<T>::apply(x, output + s);
                return s;
            },
            data
        );
    }
    static std::size_t calculateSize(std::variant<Args...> const& data) {
        return std::visit(
            [index=data.index()](auto const& x) {
                using T = std::decay_t<decltype(x)>;
                auto s = RunCBORSerializer<std::size_t>::calculateSize(2);
                auto idxS = RunCBORSerializer<IndexType>::calculateSize((IndexType)index);
                s += idxS;
                auto dataS = RunCBORSerializer<T>::calculateSize(x);
                s += dataS;
                return s;
            },
            data
        );
    }
};

namespace bytedata_variant_helper {   
    template <class... As>
    class CBORDeserializerHelperInternal {
    public:
        using TheVariant = std::variant<As...>;

        using CBORFieldParsingFunc = std::optional<std::tuple<TheVariant, std::size_t>> (*)(std::string_view const &data, std::size_t start);
        using CBORFieldInPlaceParsingFunc = std::optional<std::size_t> (*)(TheVariant &output, std::string_view const &data, std::size_t start);
        using CBORFieldParsingFuncArray = std::array<CBORFieldParsingFunc, sizeof...(As)>;
        using CBORFieldInPlaceParsingFuncArray = std::array<CBORFieldInPlaceParsingFunc, sizeof...(As)>;
        
        template <std::size_t Idx>
        static std::optional<std::tuple<TheVariant, std::size_t>> parseOneField(std::string_view const &data, std::size_t start) {
            if constexpr (Idx < sizeof...(As)) {
                using T = std::variant_alternative_t<Idx, TheVariant>;
                auto res = RunCBORDeserializer<T>::apply(data, start);
                if (!res) {
                    return std::nullopt;
                }
                return std::tuple<TheVariant, std::size_t> {
                    TheVariant {std::in_place_index<Idx>, std::move(std::get<0>(*res))}, std::get<1>(*res)
                };
            } else {
                return std::nullopt;
            }
        }
        template <std::size_t Idx>
        static std::optional<std::size_t> parseOneFieldInPlace(TheVariant &output, std::string_view const &data, std::size_t start) {
            if constexpr (Idx < sizeof...(As)) {
                using T = std::variant_alternative_t<Idx, TheVariant>;
                output = TheVariant {std::in_place_index<Idx>, T {}};
                return RunCBORDeserializer<T>::applyInPlace(std::get<Idx>(output), data, start);
            } else {
                return std::nullopt;
            }
        }
        template <std::size_t Idx>
        static constexpr void buildParserArray_internal(CBORFieldParsingFuncArray &output) {
            if constexpr (Idx < sizeof...(As)) {
                output[Idx] = &(parseOneField<Idx>);
                buildParserArray_internal<Idx+1>(output);
            }
        }
        template <std::size_t Idx>
        static constexpr void buildInPlaceParserArray_internal(CBORFieldInPlaceParsingFuncArray &output) {
            if constexpr (Idx < sizeof...(As)) {
                output[Idx] = &(parseOneFieldInPlace<Idx>);
                buildInPlaceParserArray_internal<Idx+1>(output);
            }
        }
        
        static constexpr CBORFieldParsingFuncArray buildParserArray() {
            CBORFieldParsingFuncArray ret {};
            buildParserArray_internal<0>(ret);
            return ret;
        }
        static constexpr CBORFieldInPlaceParsingFuncArray buildInPlaceParserArray() {
            CBORFieldInPlaceParsingFuncArray ret {};
            buildInPlaceParserArray_internal<0>(ret);
            return ret;
        }
    };

    template <class... As>
    class CBORDeserializerHelper {
    private:
        static constexpr auto parserArray = CBORDeserializerHelperInternal<As...>::buildParserArray();
        static constexpr auto inPlaceParserArray = CBORDeserializerHelperInternal<As...>::buildInPlaceParserArray();        
    public:   
        static std::optional<std::tuple<std::variant<As...>,std::size_t>> parse(std::size_t idx, std::string_view const &data, size_t start) {
            if (idx < sizeof...(As)) {
                return (*parserArray[idx])(data, start);
            } else {
                return std::nullopt;
            }
        }     
        static std::optional<std::size_t> parseInPlace(std::variant<As...> &output, std::size_t idx, std::string_view const &data, size_t start) {
            if (idx < sizeof...(As)) {
                return (*inPlaceParserArray[idx])(output, data, start);
            } else {
                return std::nullopt;
            }
        }
    };
};

template <typename... Args>
struct RunCBORDeserializer<std::variant<Args...>, void> {
    using IndexType = uint8_t;
    
    static std::optional<std::tuple<std::variant<Args...>, std::size_t>> apply(std::string_view const& data, std::size_t start) {
        if (data.length() < start+1) {
            return std::nullopt;
        }
        if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
            return std::nullopt;
        }
        auto n = parseCBORUnsignedInt<std::size_t>(data, start);
        if (!n) {
            return std::nullopt;
        }
        if (std::get<0>(*n) != 2) {
            return std::nullopt;
        }
        std::size_t accumLen = std::get<1>(*n);
        auto which = parseCBORUnsignedInt<IndexType>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);

        auto res = bytedata_variant_helper::CBORDeserializerHelper<Args...>::parse(std::get<0>(*which), data, start+accumLen);
        if (!res) {
            return std::nullopt;
        }
        std::get<1>(*res) += accumLen;
        return res;
    }
    static std::optional<std::size_t> applyInPlace(std::variant<Args...>& output, std::string_view const& data, std::size_t start) {
        if (data.length() < start+1) {
            return std::nullopt;
        }
        if ((static_cast<uint8_t>(data[start]) & (uint8_t) 0xe0) != 0x80) {
            return std::nullopt;
        }
        auto n = parseCBORUnsignedInt<std::size_t>(data, start);
        if (!n) {
            return std::nullopt;
        }
        if (std::get<0>(*n) != 2) {
            return std::nullopt;
        }
        std::size_t accumLen = std::get<1>(*n);
        auto which = parseCBORUnsignedInt<IndexType>(data, start+accumLen);
        if (!which) {
            return std::nullopt;
        }
        accumLen += std::get<1>(*which);

        auto res = bytedata_variant_helper::CBORDeserializerHelper<Args...>::parseInPlace(output, std::get<0>(*which), data, start+accumLen);
        if (!res) {
            return std::nullopt;
        }
        accumLen += *res;
        return accumLen;
    }
};