#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_WITH_MASKING_FILTER_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_WITH_MASKING_FILTER_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
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
    
} } } }

#endif