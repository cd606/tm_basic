#ifndef TM_KIT_BASIC_CONCAT_STRUCT_FIELD_INFO_HPP_
#define TM_KIT_BASIC_CONCAT_STRUCT_FIELD_INFO_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/StructFieldInfoBasedTuplefy.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <string_view>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    namespace struct_field_info_utils {
        namespace internal {
            template <class... Ss>
            class CheckAllHaveStructFieldInfo {
            };
            template <>
            class CheckAllHaveStructFieldInfo<> {
            public:
                static constexpr bool value = true;
            };
            template <class FirstS, class... RestSs>
            class CheckAllHaveStructFieldInfo<FirstS, RestSs...> {
            public:
                static constexpr bool value = 
                    StructFieldInfo<FirstS>::HasGeneratedStructFieldInfo 
                    &&
                    CheckAllHaveStructFieldInfo<RestSs...>::value
                    ;
            };

            class CheckNoDuplicateNameImpl {
            private:
                template <class A, class B>
                static constexpr bool doesNotHaveDuplicateNames2() {
                    auto aNames = StructFieldInfo<A>::FIELD_NAMES;
                    auto bNames = StructFieldInfo<B>::FIELD_NAMES;
                    for (auto const &an : aNames) {
                        for (auto const &bn : bNames) {
                            if (an == bn) {
                                return false;
                            }
                        }
                    }
                    return true;
                }
                template <class X, class Y, class... Rest>
                static constexpr bool doesNotHaveDuplicateNames2OrMore() {
                    if constexpr (!doesNotHaveDuplicateNames2<X,Y>()) {
                        return false;
                    }
                    if constexpr (sizeof...(Rest) > 0) {
                        return doesNotHaveDuplicateNames2OrMore<X,Rest...>();
                    } else {
                        return true;
                    }
                }
            public:
                template <class X, class... Rest>
                static constexpr bool doesNotHaveDuplicateNamesCross() {
                    if constexpr (sizeof...(Rest) == 0) {
                        return true;
                    } else {
                        if constexpr (sizeof...(Rest) == 1) {
                            return doesNotHaveDuplicateNames2OrMore<X,Rest...>();
                        } else {
                            if constexpr (!doesNotHaveDuplicateNames2OrMore<X,Rest...>()) {
                                return false;
                            } else {
                                return doesNotHaveDuplicateNamesCross<Rest...>();
                            }
                        }
                    }
                }
            };

            template <class... Ss>
            class CheckNoNameDuplicate {
            public:
                static constexpr bool value = CheckNoDuplicateNameImpl::doesNotHaveDuplicateNamesCross<Ss...>();
            };
            template <>
            class CheckNoNameDuplicate<> {
            public:
                static constexpr bool value = true;
            };

            template <class... Ss>
            struct AddFieldCount {
            };
            template <>
            struct AddFieldCount<> {
                static constexpr std::size_t value = 0;
            };
            template <class FirstS, class... RestSs>
            struct AddFieldCount<FirstS, RestSs...> {
                static constexpr std::size_t value = StructFieldInfo<FirstS>::FIELD_NAMES.size()+AddFieldCount<RestSs...>::value;
            };

            template <class... Ss>
            struct ConcatFieldNames {
            };
            template <>
            struct ConcatFieldNames<> {
                static constexpr std::array<std::string_view, 0> value = {};
            };
            template <class FirstS, class... RestSs>
            struct ConcatFieldNames<FirstS, RestSs...> {
            private:
                static constexpr std::array<std::string_view, AddFieldCount<FirstS,RestSs...>::value>
                concatRes() {
                    std::array<std::string_view, AddFieldCount<FirstS,RestSs...>::value> ret;
                    constexpr std::size_t n1 = StructFieldInfo<FirstS>::FIELD_NAMES.size();
                    for (std::size_t ii=0; ii<n1; ++ii) {
                        ret[ii] = StructFieldInfo<FirstS>::FIELD_NAMES[ii];
                    }
                    constexpr auto more = ConcatFieldNames<RestSs...>::value;
                    for (std::size_t ii=n1; ii<ret.size(); ++ii) {
                        ret[ii] = more[ii-n1];
                    }
                    return ret;
                }
            public:
                static constexpr std::array<std::string_view, AddFieldCount<FirstS,RestSs...>::value> value = concatRes();
            };
            template <int Idx, class... Ss>
            struct IdxLookupInConcatStruct {};
            template <int Idx>
            struct IdxLookupInConcatStruct<Idx> {
                using TheSubStruct = void;
                static constexpr int TranslatedIdx = 0;
            };
            template <int Idx, class FirstS, class... RestSs>
            struct IdxLookupInConcatStruct<Idx,FirstS,RestSs...> {
                using TheSubStruct = std::conditional_t<
                    Idx < StructFieldInfo<FirstS>::FIELD_NAMES.size()
                    , FirstS
                    , typename IdxLookupInConcatStruct<std::max(Idx-(int)StructFieldInfo<FirstS>::FIELD_NAMES.size(),0),RestSs...>::TheSubStruct
                >;
                static constexpr int TranslatedIdx = (
                    Idx < StructFieldInfo<FirstS>::FIELD_NAMES.size()
                    ?
                    Idx 
                    :
                    IdxLookupInConcatStruct<std::max(Idx-(int)StructFieldInfo<FirstS>::FIELD_NAMES.size(),0), RestSs...>::TranslatedIdx
                );
            };
        }
        
        template <class... Ss>
        struct ConcatStructFields : public Ss... {
            static_assert(internal::CheckAllHaveStructFieldInfo<Ss...>::value, "All structs to be concatenated must be struct-field-info-supportable");
            static_assert(internal::CheckNoNameDuplicate<Ss...>::value, "All structs to be concatenated must not share field names");
        };

    }

    template <class... Ss>
    class StructFieldInfo<struct_field_info_utils::ConcatStructFields<Ss...>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr bool EncDecWithFieldNames = false;
        static constexpr std::string_view REFERENCE_NAME = "struct_field_info_utils::ConcatStructFields<>"; \
        static constexpr auto FIELD_NAMES = struct_field_info_utils::internal::ConcatFieldNames<Ss...>::value;
        static constexpr int getFieldIndex(std::string_view const &fieldName) { 
            for (int ii=0; ii<FIELD_NAMES.size(); ++ii) {
                if (fieldName == FIELD_NAMES[ii]) {
                    return ii;
                }
            }
            return -1;
        }
    };
    template <class... Ss, int Idx>
    class StructFieldTypeInfo<struct_field_info_utils::ConcatStructFields<Ss...>, Idx> {
    private:
        using L = struct_field_info_utils::internal::IdxLookupInConcatStruct<Idx,Ss...>;
    public:
        using TheType = typename StructFieldTypeInfo<
            typename L::TheSubStruct, L::TranslatedIdx
        >::TheType;
        static TheType &access(struct_field_info_utils::ConcatStructFields<Ss...> &d) {
            return StructFieldTypeInfo< 
                typename L::TheSubStruct, L::TranslatedIdx
            >::access(static_cast<typename L::TheSubStruct &>(d));
        }
        static TheType const &constAccess(struct_field_info_utils::ConcatStructFields<Ss...> const &d) {
            return StructFieldTypeInfo< 
                typename L::TheSubStruct, L::TranslatedIdx
            >::constAccess(static_cast<typename L::TheSubStruct const &>(d));
        }
        static TheType &&moveAccess(struct_field_info_utils::ConcatStructFields<Ss...> &&d) {
            return StructFieldTypeInfo< 
                typename L::TheSubStruct, L::TranslatedIdx
            >::moveAccess(static_cast<typename L::TheSubStruct &&>(std::move(d)));
        }
    };

    namespace bytedata_utils {
        template <class... Ss>
        struct RunCBORSerializer<struct_field_info_utils::ConcatStructFields<Ss...>, void> {
            static std::string apply(struct_field_info_utils::ConcatStructFields<Ss...> const &x) {
                std::string s;
                s.resize(calculateSize(x));
                apply(x, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(struct_field_info_utils::ConcatStructFields<Ss...> const &x, char *output) {
                return RunCBORSerializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::ConstPtrTupleType
                >::apply(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::toConstPtrTuple(x)
                    , output
                );
            }
            static std::size_t calculateSize(struct_field_info_utils::ConcatStructFields<Ss...> const &x) {
                return RunCBORSerializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::ConstPtrTupleType
                >::calculateSize(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::toConstPtrTuple(x)
                );
            }
        };
        template <class... Ss>
        struct RunSerializer<struct_field_info_utils::ConcatStructFields<Ss...>, void> {
            static std::string apply(struct_field_info_utils::ConcatStructFields<Ss...> const &x) {
                return RunCBORSerializer<struct_field_info_utils::ConcatStructFields<Ss...>, void>::apply(x);
            }
        };

        template <class... Ss>
        struct RunCBORDeserializer<struct_field_info_utils::ConcatStructFields<Ss...>, void> {
            static std::optional<std::tuple<struct_field_info_utils::ConcatStructFields<Ss...>, size_t>> apply(std::string_view const &s, size_t start) {
                auto t = RunCBORDeserializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::TupleType
                >::apply(s, start);
                if (!t) {
                    return std::nullopt; 
                }
                return std::tuple<struct_field_info_utils::ConcatStructFields<Ss...>, size_t> {
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::fromTupleByMove(std::move(std::get<0>(*t)))
                    , std::get<1>(*t)
                };
            }
            static std::optional<size_t> applyInPlace(struct_field_info_utils::ConcatStructFields<Ss...> &x, std::string_view const &s, size_t start) {
                return RunCBORDeserializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::PtrTupleTypeType
                >::applyInPlace(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_utils::ConcatStructFields<Ss...>
                    >::toPtrTuple(x)
                    , s
                    , start
                );
            }
        };
        template <class... Ss>
        struct RunDeserializer<struct_field_info_utils::ConcatStructFields<Ss...>, void> {
            static std::optional<struct_field_info_utils::ConcatStructFields<Ss...>> apply(std::string_view const &s) {
                auto t = RunDeserializer<CBOR<struct_field_info_utils::ConcatStructFields<Ss...>>, void>::apply(s);
                if (!t) {
                    return std::nullopt;
                }
                return std::move(t->value);
            }
            static std::optional<struct_field_info_utils::ConcatStructFields<Ss...>> apply(std::string const &s) {
                return apply(std::string_view {s});
            }
            static bool applyInPlace(struct_field_info_utils::ConcatStructFields<Ss...> &output, std::string_view const &s) {
                auto t = RunCBORDeserializer<struct_field_info_utils::ConcatStructFields<Ss...>, void>::applyInPlace(output, s, 0);
                if (!t) {
                    return false;
                }
                if (*t != s.length()) {
                    return false;
                }
                return true;
            }
            static bool applyInPlace(struct_field_info_utils::ConcatStructFields<Ss...> &output, std::string const &s) {
                return applyInPlace(output, std::string_view {s});
            }
        };
    }
    
} } } }

#endif