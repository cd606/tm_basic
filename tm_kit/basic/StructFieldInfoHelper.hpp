#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_HELPER_HPP_

#include <tm_kit/infra/VersionedData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T>
    class StructFieldInfo {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = false;
    };
    template <class T, int Idx>
    class StructFieldTypeInfo {
    };

    //specialize for versioned data and grouped versioned data
    template <class VersionType, class DataType, class CmpType>
    class StructFieldInfo<infra::VersionedData<VersionType,DataType,CmpType>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr std::array<std::string_view, 2> FIELD_NAMES = { 
            "version", "data"
        };
        static constexpr int getFieldIndex(std::string_view const &fieldName) { 
            if (fieldName == "version") {
                return 0;
            }
            if (fieldName == "data") {
                return 1;
            }
            return -1;
        }
    };
    template <class VersionType, class DataType, class CmpType> 
    class StructFieldTypeInfo<infra::VersionedData<VersionType,DataType,CmpType>, 0> { 
    public:
        using TheType = VersionType;
        using FieldPointer = VersionType infra::VersionedData<VersionType,DataType,CmpType>::*; 
        static constexpr FieldPointer fieldPointer() {
            return &infra::VersionedData<VersionType,DataType,CmpType>::version;
        } 
    };
    template <class VersionType, class DataType, class CmpType> 
    class StructFieldTypeInfo<infra::VersionedData<VersionType,DataType,CmpType>, 1> { 
    public:
        using TheType = DataType;
        using FieldPointer = DataType infra::VersionedData<VersionType,DataType,CmpType>::*; 
        static constexpr FieldPointer fieldPointer() {
            return &infra::VersionedData<VersionType,DataType,CmpType>::data;
        } 
    };

    template <class GroupIDType, class VersionType, class DataType, class CmpType>
    class StructFieldInfo<infra::GroupedVersionedData<GroupIDType, VersionType,DataType,CmpType>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr std::array<std::string_view, 3> FIELD_NAMES = { 
            "groupID", "version", "data"
        };
        static constexpr int getFieldIndex(std::string_view const &fieldName) { 
            if (fieldName == "groupID") {
                return 0;
            }
            if (fieldName == "version") {
                return 1;
            }
            if (fieldName == "data") {
                return 2;
            }
            return -1;
        }
    };
    template <class GroupIDType, class VersionType, class DataType, class CmpType> 
    class StructFieldTypeInfo<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>, 0> { 
    public:
        using TheType = GroupIDType;
        using FieldPointer = GroupIDType infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>::*; 
        static constexpr FieldPointer fieldPointer() {
            return &infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>::groupID;
        } 
    };
    template <class GroupIDType, class VersionType, class DataType, class CmpType> 
    class StructFieldTypeInfo<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>, 1> { 
    public:
        using TheType = VersionType;
        using FieldPointer = VersionType infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>::*; 
        static constexpr FieldPointer fieldPointer() {
            return &infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>::version;
        } 
    };
    template <class GroupIDType, class VersionType, class DataType, class CmpType> 
    class StructFieldTypeInfo<infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>, 2> { 
    public:
        using TheType = DataType;
        using FieldPointer = DataType infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>::*; 
        static constexpr FieldPointer fieldPointer() {
            return &infra::GroupedVersionedData<GroupIDType,VersionType,DataType,CmpType>::data;
        } 
    };
}}}}
#endif