#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_UTILS_HPP_

#include <string>
#include <sstream>

#include <tm_kit/basic/StructFieldInfoHelper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {
    template <class T>
    class StructFieldInfoBasedDataFiller {
    private:
        template <class R, int FieldCount, int FieldIndex>
        static void fillData_internal(T &data, R const &source) {
            if constexpr (FieldIndex >= 0 && FieldIndex < FieldCount) {
                data.*(StructFieldTypeInfo<T,FieldIndex>::fieldPointer()) = source.template get<typename StructFieldTypeInfo<T,FieldIndex>::TheType>(FieldIndex);
                if constexpr (FieldIndex < FieldCount-1) {
                    fillData_internal<R,FieldCount,FieldIndex+1>(data, source);
                }
            }
        }
    public:
        static std::string commaSeparatedFieldNames() {
            bool begin = true;
            std::ostringstream oss;
            for (auto const &n : StructFieldInfo<T>::FIELD_NAMES) {
                if (!begin) {
                    oss << ", ";
                }
                begin = false;
                oss << n;
            }
            return oss.str();
        }
        template <class R>
        static void fillData(T &data, R const &source) {
            fillData_internal<R, StructFieldInfo<T>::FIELD_NAMES.size(), 0>(data, source);
        }
        template <class R>
        static T retrieveData(R const &source) {
            T ret;
            fillData(ret, source);
            return ret;
        }
    };  
}}}}}

#endif