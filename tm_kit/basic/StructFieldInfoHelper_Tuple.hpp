#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_TUPLE_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_TUPLE_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class... Ts>
    class StructFieldInfo<std::tuple<Ts...>> {
    private:
        template <class First, class... More>
        static constexpr bool hasInfoHelper() {
            if constexpr (!StructFieldInfo<First>::HasGeneratedStructFieldInfo) {
                return false;
            } else {
                if constexpr (sizeof...(More) == 0) {
                    return true;
                } else {
                    return hasInfoHelper<More...>();
                }
            }
        }
        template <class First, class... More>
        static constexpr std::size_t fieldCountHelper(std::size_t soFar) {
            if constexpr (sizeof...(More) == 0) {
                return StructFieldInfo<First>::FIELD_NAMES.size()+soFar;
            } else {
                return fieldCountHelper<More...>(StructFieldInfo<First>::FIELD_NAMES.size()+soFar);
            }
        } 
        static constexpr std::size_t N = fieldCountHelper<Ts...>(0);

        template <std::size_t Idx, class First, class... More>
        static constexpr void fieldNamesHelper_internal(std::array<std::string_view, N> &ret) {
            auto const &nm = StructFieldInfo<First>::FIELD_NAMES;
            for (std::size_t ii=0; ii<nm.size(); ++ii) {
                ret[ii+Idx] = nm[ii];
            }
            if constexpr (sizeof...(More) == 0) {
                return;
            } else {
                fieldNamesHelper_internal<Idx+StructFieldInfo<First>::FIELD_NAMES.size(),More...>(ret);
            }
        }
        static constexpr std::array<std::string_view, N> fieldNamesHelper() {
            std::array<std::string_view, N> ret;
            fieldNamesHelper_internal<0, Ts...>(ret);
            return ret;
        }
        template <std::size_t Idx, class First, class... More>
        static constexpr int getFieldIndexHelper(std::string_view const &fieldName) {
            int ret = StructFieldInfo<First>::getFieldIndex(fieldName);
            if (ret >= 0) {
                return ret+Idx;
            } else {
                if constexpr (sizeof...(More) == 0) {
                    return -1;
                } else {
                    return getFieldIndexHelper<Idx+StructFieldInfo<First>::FIELD_NAMES.size(), More...>(fieldName);
                }
            }
        }
    public:
        static constexpr bool HasGeneratedStructFieldInfo = hasInfoHelper<Ts...>();
        static constexpr std::array<std::string_view, N> FIELD_NAMES = fieldNamesHelper();
        static constexpr int getFieldIndex(std::string_view const &fieldName) {
            return getFieldIndexHelper<0,Ts...>(fieldName);
        }
    };

    namespace struct_field_info_helper_internal {
        template <std::size_t TupleStart, int Idx, class... Ts>
        class TupleStructTypeHelper {
        };
        template <std::size_t TupleStart, int Idx>
        class TupleStructTypeHelper<TupleStart, Idx> {
        public:
            using PartialType = void;
            static constexpr int PartialIdx = -1;
            static constexpr int TupleIdx = std::numeric_limits<std::size_t>::max();
        };
        template <std::size_t TupleStart, int Idx, class First, class... More>
        class TupleStructTypeHelper<TupleStart, Idx, First, More...> {
        public:
            using PartialType = std::conditional_t<
                (Idx >= 0 && Idx < StructFieldInfo<First>::FIELD_NAMES.size())
                , First
                , typename TupleStructTypeHelper<
                    TupleStart+1
                    , Idx-StructFieldInfo<First>::FIELD_NAMES.size()
                    , More...
                >::PartialType
            >;
            static constexpr int PartialIdx = (
                (Idx >= 0 && Idx < StructFieldInfo<First>::FIELD_NAMES.size())
                ?
                Idx
                :
                TupleStructTypeHelper<
                    TupleStart+1
                    , Idx-StructFieldInfo<First>::FIELD_NAMES.size()
                    , More...
                >::PartialIdx
            );
            static constexpr std::size_t TupleIdx = (
                (Idx >= 0 && Idx < StructFieldInfo<First>::FIELD_NAMES.size())
                ?
                TupleStart
                :
                TupleStructTypeHelper<
                    TupleStart+1
                    , Idx-StructFieldInfo<First>::FIELD_NAMES.size()
                    , More...
                >::TupleIdx
            );
        };
    }   
    template <int Idx, class... Ts>
    class StructFieldTypeInfo<std::tuple<Ts...>, Idx> {
    private:
        using Delegate = StructFieldTypeInfo<
            typename struct_field_info_helper_internal::TupleStructTypeHelper<0, Idx, Ts...>::PartialType
            , struct_field_info_helper_internal::TupleStructTypeHelper<0, Idx, Ts...>::PartialIdx
        >;
        static constexpr std::size_t TupleIdx = struct_field_info_helper_internal::TupleStructTypeHelper<0, Idx, Ts...>::TupleIdx;
    public:
        using TheType = typename Delegate::TheType;
        static TheType &access(std::tuple<Ts...> &d) {
            return Delegate::access(std::get<TupleIdx>(d));
        }
        static TheType const &constAccess(std::tuple<Ts...> const &d) {
            return Delegate::constAccess(std::get<TupleIdx>(d));
        }
        static TheType &&moveAccess(std::tuple<Ts...> &&d) {
            return Delegate::moveAccess(std::move(std::get<TupleIdx>(d)));
        }
    };

}}}}
#endif