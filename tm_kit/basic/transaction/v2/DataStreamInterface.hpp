#ifndef TM_KIT_BASIC_TRANSACTION_V2_DATA_STREAM_INTERFACE_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_DATA_STREAM_INTERFACE_HPP_

#include <tm_kit/basic/ByteData.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <type_traits>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    template <
        class GlobalVersionType
        , class KeyType
        , class VersionType
        , class DataType
        , class GlobalVersionCmpType = std::less<GlobalVersionType>
        , class VersionCmpType = std::less<VersionType>
    >
    using DataStreamUpdate =
        infra::VersionedData<
            GlobalVersionType
            , infra::GroupedVersionedData<
                KeyType
                , VersionType
                , DataType
                , VersionCmpType
            >
            , GlobalVersionCmpType
        >;

    template <
        class GlobalVersionType
        , class KeyType
        , class VersionType
        , class DataType
        , class VersionDeltaType = VersionType
        , class DataDeltaType = DataType
        , class GlobalVersionCmpType = std::less<GlobalVersionType>
        , class VersionCmpType = std::less<VersionType>
        , class Enable = void 
    >
    class DataStreamInterface {};

    template <
        class GlobalVersionType
        , class KeyType
        , class VersionType
        , class DataType
        , class VersionDeltaType
        , class DataDeltaType
        , class GlobalVersionCmpType
        , class VersionCmpType
    >
    class DataStreamInterface<
        GlobalVersionType
        , KeyType
        , VersionType
        , DataType
        , VersionDeltaType
        , DataDeltaType
        , GlobalVersionCmpType
        , VersionCmpType
        , std::enable_if_t<
               std::is_default_constructible_v<GlobalVersionType>
            && std::is_copy_constructible_v<GlobalVersionType>
            && std::is_default_constructible_v<KeyType>
            && std::is_copy_constructible_v<KeyType>
            && std::is_default_constructible_v<VersionType>
            && std::is_copy_constructible_v<VersionType>
        >
    > {
    public:
        using GlobalVersion = GlobalVersionType;
        using Key = KeyType;
        using Version = VersionType;
        using Data = DataType;
        using VersionDelta= VersionDeltaType;
        using DataDelta = DataDeltaType;
        using GlobalVersionCmp = GlobalVersionCmpType;
        using VersionCmp = VersionCmpType;

        using FullUpdate = DataStreamUpdate<GlobalVersion,Key,Version,Data,GlobalVersionCmp,VersionCmp>;
        using DeltaUpdate = DataStreamUpdate<GlobalVersion,Key,VersionDelta,DataDelta,GlobalVersionCmp,VersionCmp>;

        using Update = CBOR<
            std::variant<FullUpdate, DeltaUpdate>
        >;
    };

} }

} } } }


#endif