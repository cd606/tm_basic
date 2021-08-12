#ifndef TM_KIT_BASIC_STRUCT_FIELD_FLATTENED_INFO_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_FLATTENED_INFO_HPP_

#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <chrono>
#include <utility>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {

    namespace internal {

        template <class SequencePrefix, class T, std::size_t FieldCount, std::size_t FieldIndex, class TypesSoFar, bool LastOne=(FieldIndex>=FieldCount)>
        class StructFieldFlattenedInfoImpl {
        };

        template <class SequencePrefix, class T, std::size_t FieldCount, std::size_t FieldIndex, class TypesSoFar>
        class StructFieldFlattenedInfoImpl<SequencePrefix,T,FieldCount,FieldIndex,TypesSoFar,true> {
        public:
            using TheType = TypesSoFar;
        };

        template <std::size_t ToAppend, class ExistingSeq>
        class AppendToIndexSeq {};
        template <std::size_t ToAppend>
        class AppendToIndexSeq<ToAppend,std::index_sequence<>> {
        public:
            using TheType = std::index_sequence<ToAppend>; 
        };
        template <std::size_t ToAppend, std::size_t... ExistingIndices>
        class AppendToIndexSeq<ToAppend,std::index_sequence<ExistingIndices...>> {
        public:
            using TheType = std::index_sequence<ExistingIndices..., ToAppend>; 
        };

        template <class ToAppend, class ExistingTuple>
        class AppendToTuple {};
        template <class ToAppend>
        class AppendToTuple<ToAppend,std::tuple<>> {
        public:
            using TheType = std::tuple<ToAppend>; 
        };
        template <class ToAppend, class... ExistingTypes>
        class AppendToTuple<ToAppend,std::tuple<ExistingTypes...>> {
        public:
            using TheType = std::tuple<ExistingTypes..., ToAppend>; 
        };

        template <class TailTuple, class ExistingTuple>
        class AppendTupleToTuple {};
        template <class ExistingTuple>
        class AppendTupleToTuple<std::tuple<>,ExistingTuple> {
        public:
            using TheType = ExistingTuple; 
        };
        template <class ExistingTuple, class FirstTailType, class... OtherTailTypes>
        class AppendTupleToTuple<std::tuple<FirstTailType,OtherTailTypes...>,ExistingTuple> {
        public:
            using TheType = typename AppendTupleToTuple<
                std::tuple<OtherTailTypes...> 
                , typename AppendToTuple<FirstTailType,ExistingTuple>::TheType
            >::TheType;
        };

        template <class SequencePrefix, class F, std::size_t FieldIndex, class TypesSoFar, bool HasSubFields=StructFieldInfo<F>::HasGeneratedStructFieldInfo>
        class AppendOneStructFieldToFlattenedInfo {
        };
        template <class SequencePrefix, class F, std::size_t FieldIndex, class TypesSoFar>
        class AppendOneStructFieldToFlattenedInfo<SequencePrefix,F,FieldIndex,TypesSoFar,false> {
        public:
            using TheType = typename AppendToTuple<
                std::tuple<
                    typename AppendToIndexSeq<FieldIndex,SequencePrefix>::TheType
                    , F
                >
                , TypesSoFar
            >::TheType;
        };
        template <class SequencePrefix, class F, std::size_t FieldIndex, class TypesSoFar>
        class AppendOneStructFieldToFlattenedInfo<SequencePrefix,F,FieldIndex,TypesSoFar,true> {
        public:
            using TheType = typename AppendTupleToTuple<
                typename StructFieldFlattenedInfoImpl<
                    typename AppendToIndexSeq<FieldIndex,SequencePrefix>::TheType 
                    , F 
                    , StructFieldInfo<F>::FIELD_NAMES.size() 
                    , 0
                    , std::tuple<>
                >::TheType
                , TypesSoFar
            >::TheType;
        };

        template <class SequencePrefix, class T, std::size_t FieldCount, std::size_t FieldIndex, class TypesSoFar>
        class StructFieldFlattenedInfoImpl<SequencePrefix,T,FieldCount,FieldIndex,TypesSoFar,false> {
        public:
            using TheType = typename StructFieldFlattenedInfoImpl<
                SequencePrefix,T,FieldCount,FieldIndex+1,
                typename AppendOneStructFieldToFlattenedInfo<
                    SequencePrefix 
                    , typename StructFieldTypeInfo<T,FieldIndex>::TheType
                    , FieldIndex
                    , TypesSoFar 
                >::TheType
            >::TheType;
        };
    }

    template <class T, typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>>
    class StructFieldFlattenedInfo {
    public:
        using TheType = typename internal::StructFieldFlattenedInfoImpl<
            std::index_sequence<>, T, StructFieldInfo<T>::FIELD_NAMES.size(), 0, std::tuple<>
        >::TheType;
    };

    template <class FieldType, std::size_t... CursorSequence>
    using StructFieldFlattenedInfoCursor = std::tuple<
        std::index_sequence<CursorSequence...>
        , FieldType 
    >;

    template <class T, class FieldCursor>
    class StructFieldFlattenedInfoCursorBasedAccess {};

    template <class T>
    class StructFieldFlattenedInfoCursorBasedAccess<T, StructFieldFlattenedInfoCursor<T>> {
    public:
        using TheType = T;
        static T *valuePointer(T &data) {
            return &data;
        }
        static T const *constValuePointer(T const &data) {
            return &data;
        }
    };

    template <class T, class FieldType, std::size_t FirstIndex, std::size_t... RemainingIndices>
    class StructFieldFlattenedInfoCursorBasedAccess<T, StructFieldFlattenedInfoCursor<FieldType,FirstIndex,RemainingIndices...>> {
    public:
        using TheType = FieldType;
        static FieldType *valuePointer(T &data) {
            return StructFieldFlattenedInfoCursorBasedAccess<
                typename StructFieldTypeInfo<T,FirstIndex>::TheType 
                , StructFieldFlattenedInfoCursor<FieldType,RemainingIndices...>
            >::valuePointer(data.*(StructFieldTypeInfo<T,FirstIndex>::fieldPointer()));
        }
        static FieldType const *constValuePointer(T const &data) {
            return StructFieldFlattenedInfoCursorBasedAccess<
                typename StructFieldTypeInfo<T,FirstIndex>::TheType 
                , StructFieldFlattenedInfoCursor<FieldType,RemainingIndices...>
            >::constValuePointer(data.*(StructFieldTypeInfo<T,FirstIndex>::fieldPointer()));
        }
    };

} } } } }
 
#endif