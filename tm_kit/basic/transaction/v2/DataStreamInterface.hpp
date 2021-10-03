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

        using OneFullUpdateItem = infra::GroupedVersionedData<
            KeyType
            , VersionType
            , std::optional<DataType>
            , VersionCmpType
        >;
        using OneDeltaUpdateItem = std::tuple<
            KeyType
            , VersionDelta
            , DataDelta
        >;
        using OneUpdateItem = SingleLayerWrapper<std::variant<
            OneFullUpdateItem
            , OneDeltaUpdateItem
        >>;
        using Update = infra::VersionedData<
            GlobalVersion
            , std::vector<OneUpdateItem>
            , GlobalVersionCmp
        >;
        //This is for client's convenience
        using FullUpdate = infra::VersionedData<
            GlobalVersion
            , std::vector<OneFullUpdateItem>
            , GlobalVersionCmp
        >;
        static std::optional<FullUpdate> getFullUpdatesOnly(Update &&u) {
            std::vector<OneFullUpdateItem> v;
            for (auto &item : u.data) {
                std::visit([&v](auto &&x) {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, OneFullUpdateItem>) {
                        v.push_back(std::move(x));
                    }
                }, std::move(item.value));
            }
            if (v.empty()) {
                return std::nullopt;
            }
            return FullUpdate {
                std::move(u.version), std::move(v)
            };
        }
    };

} }

} } } }


#endif