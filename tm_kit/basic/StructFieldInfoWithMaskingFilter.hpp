#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_WITH_MASKING_FILTER_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_WITH_MASKING_FILTER_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/StructFieldInfoBasedTuplefy.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <string_view>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    namespace struct_field_info_masking {
        namespace internal {
            template <std::size_t N, std::size_t K>
            inline constexpr std::size_t masking_filter_result_size_impl(
                std::array<std::string_view, N> const &input 
                , bool (*f)(std::string_view const &)
                , std::size_t accum
            ) {
                if constexpr (K>=0 && K<N) {
                    return masking_filter_result_size_impl<N,K+1>(
                        input, f, accum+(f(input[K])?1:0)
                    );
                } else {
                    return accum;
                }
            }
            template <std::size_t N>
            inline constexpr std::size_t masking_filter_result_size(
                std::array<std::string_view, N> const &input 
                , bool (*f)(std::string_view const &)
            ) {
                return masking_filter_result_size_impl<N,0>(input, f, 0);
            }

            template <std::size_t N, std::size_t K>
            class MaskingFilterResults {
            public:
                std::array<std::size_t, K> filteredToOriginal {0};
                std::array<std::size_t, N> originalToFiltered {0};
                std::array<std::string_view, K> filteredNames {""};

                constexpr MaskingFilterResults(
                    std::array<std::string_view, N> const &input 
                    , bool (*f)(std::string_view const &)
                ) {
                    int jj=0;
                    for (int ii=0; ii<N; ++ii) {
                        if (f(input[ii])) {
                            originalToFiltered[ii] = jj;
                            filteredToOriginal[jj] = ii;
                            filteredNames[jj] = input[ii];
                            ++jj;
                        } else {
                            originalToFiltered[ii] = N;
                        }
                    }
                }
            };
        }

        template <
            class T
            , bool (*maskF)(std::string_view const &) 
            , typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        >
        struct MaskedStruct : public T {};
    }

    template <
        class T
        , bool (*maskF)(std::string_view const &)
    >
    class StructFieldInfo<struct_field_info_masking::MaskedStruct<T,maskF>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
    private:
        static constexpr std::size_t N = StructFieldInfo<T>::FIELD_NAMES.size();
        static constexpr std::size_t K = struct_field_info_masking::internal::masking_filter_result_size<N>(
            StructFieldInfo<T>::FIELD_NAMES, maskF
        );
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = {
            StructFieldInfo<T>::FIELD_NAMES, maskF
        };
    public:
        static constexpr std::array<std::string_view, K> FIELD_NAMES = filterResults.filteredNames;
        static constexpr int getFieldIndex(std::string_view const &fieldName) { 
            for (int ii=0; ii<K; ++ii) {
                if (fieldName == FIELD_NAMES[ii]) {
                    return ii;
                }
            }
            return -1;
        }
    };
    template <
        class T
        , bool (*maskF)(std::string_view const &)
        , int Idx
    >
    class StructFieldTypeInfo<struct_field_info_masking::MaskedStruct<T,maskF>, Idx> {
    private:
        static constexpr std::size_t N = StructFieldInfo<T>::FIELD_NAMES.size();
        static constexpr std::size_t K = struct_field_info_masking::internal::masking_filter_result_size<N>(
            StructFieldInfo<T>::FIELD_NAMES, maskF
        );
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = {
            StructFieldInfo<T>::FIELD_NAMES, maskF
        };
    public:
        using TheType = typename StructFieldTypeInfo<
            T, filterResults.filteredToOriginal[Idx]
        >::TheType; 
        using FieldPointer = TheType struct_field_info_masking::MaskedStruct<T,maskF>::*;
        static constexpr FieldPointer fieldPointer() {
            return (FieldPointer) StructFieldTypeInfo<
                T, filterResults.filteredToOriginal[Idx]
            >::fieldPointer();
        }
    };

    namespace bytedata_utils {
        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunCBORSerializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::string apply(struct_field_info_masking::MaskedStruct<T,maskF> const &x) {
                std::string s;
                s.resize(calculateSize(x));
                apply(x, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(struct_field_info_masking::MaskedStruct<T,maskF> const &x, char *output) {
                return RunCBORSerializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::ConstPtrTupleType
                >::apply(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::toConstPtrTuple(x)
                    , output
                );
            }
            static std::size_t calculateSize(struct_field_info_masking::MaskedStruct<T,maskF> const &x) { \
                return RunCBORSerializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::ConstPtrTupleType
                >::calculateSize(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::toConstPtrTuple(x)
                );
            }
        };
        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunSerializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::string apply(struct_field_info_masking::MaskedStruct<T,maskF> const &x) {
                return RunCBORSerializer<struct_field_info_masking::MaskedStruct<T,maskF>, void>::apply(x);
            }
        };

        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunCBORDeserializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::optional<std::tuple<struct_field_info_masking::MaskedStruct<T,maskF>, size_t>> apply(std::string_view const &s, size_t start) {
                auto t = RunCBORDeserializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::TupleType
                >::apply(s, start);
                if (!t) {
                    return std::nullopt; 
                }
                return std::tuple<struct_field_info_masking::MaskedStruct<T,maskF>, size_t> {
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::fromTupleByMove(std::move(std::get<0>(*t)))
                    , std::get<1>(*t)
                };
            }
            static std::optional<size_t> applyInPlace(struct_field_info_masking::MaskedStruct<T,maskF> &x, std::string_view const &s, size_t start) {
                return RunCBORDeserializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::PtrTupleType
                >::applyInPlace(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::toPtrTuple(x)
                    , s
                    , start
                );
            }
        };
        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunDeserializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::optional<struct_field_info_masking::MaskedStruct<T,maskF>> apply(std::string_view const &s) {
                auto t = RunDeserializer<CBOR<struct_field_info_masking::MaskedStruct<T,maskF>>, void>::apply(s);
                if (!t) {
                    return std::nullopt;
                }
                return std::move(t->value);
            }
            static std::optional<struct_field_info_masking::MaskedStruct<T,maskF>> apply(std::string const &s) {
                return apply(std::string_view {s});
            }
            static bool applyInPlace(struct_field_info_masking::MaskedStruct<T,maskF> &output, std::string_view const &s) {
                auto t = RunCBORDeserializer<struct_field_info_masking::MaskedStruct<T,maskF>, void>::applyInPlace(output, s, 0);
                if (!t) {
                    return false;
                }
                if (*t != s.length()) {
                    return false;
                }
                return true;
            }
            static bool applyInPlace(struct_field_info_masking::MaskedStruct<T,maskF> &output, std::string const &s) {
                return applyInPlace(output, std::string_view {s});
            }
        };
    }
    
} } } }

#endif