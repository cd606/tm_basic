#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_HDF5_UTILS_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_BASED_HDF5_UTILS_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>

#include <hdf5.h>
#include <H5Cpp.h>

#include <boost/algorithm/string.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace struct_field_info_utils {

    namespace internal {
        template <class T, typename Enable=void>
        class StructFieldInfoBasedHdf5Support {
        public:
            static constexpr bool HasSupport = false;
        };

        template <>
        class StructFieldInfoBasedHdf5Support<uint8_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_UINT8;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<int8_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_INT8;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<uint16_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_UINT16;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<int16_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_INT16;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<uint32_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_UINT32;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<int32_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_INT32;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<uint64_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_UINT64;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<int64_t, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_INT64;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<char, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::C_S1;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<bool, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_UCHAR;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<float, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_FLOAT;
            }
        };
        template <>
        class StructFieldInfoBasedHdf5Support<double, void> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                return H5::PredType::NATIVE_DOUBLE;
            }
        };
        template <class T, std::size_t N>
        class StructFieldInfoBasedHdf5Support<std::array<T, N>, std::enable_if_t<StructFieldInfoBasedHdf5Support<T>::HasSupport, void>> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                static const hsize_t dims[] = {N};
                return H5::ArrayType(StructFieldInfoBasedHdf5Support<T>::TheType(), 1, dims);
            }
        };
        template <class T>
        class StructFieldInfoBasedHdf5Support<T, std::enable_if_t<bytedata_utils::IsEnumWithStringRepresentation<T>::value, void>> {
        public:
            static constexpr bool HasSupport = true;
            static H5::DataType TheType() {
                H5::EnumType ret {H5::PredType::NATIVE_INT};
                int v = 0;
                for (auto const &name : bytedata_utils::IsEnumWithStringRepresentation<T>::names) {
                    ret.insert(std::string {name}, &v);
                    ++v;
                }
                return ret;
            }
        };
        template <class T>
        class StructFieldInfoBasedHdf5Support<T, std::enable_if_t<StructFieldOffsetInfo<T>::HasGeneratedStructFieldOffsetInfo && StructFieldInfo<T>::HasGeneratedStructFieldInfo, void>> {
        private:
            template <std::size_t Idx>
            static constexpr bool allHasSupport() {
                if constexpr (Idx < StructFieldInfo<T>::FIELD_NAMES.size()) {
                    if (!StructFieldInfoBasedHdf5Support<
                        typename StructFieldTypeInfo<T, Idx>::TheType
                    >::HasSupport) {
                        return false;
                    }
                    return allHasSupport<Idx+1>();
                } else {
                    return true;
                }
            }
            template <std::size_t Idx>
            static void fillInType(H5::CompType &ret) {
                if constexpr (Idx < StructFieldInfo<T>::FIELD_NAMES.size()) {
                    static_assert(StructFieldInfoBasedHdf5Support<
                        typename StructFieldTypeInfo<T, Idx>::TheType
                    >::HasSupport);
                    ret.insertMember(
                        std::string {StructFieldInfo<T>::FIELD_NAMES[Idx]}
                        , StructFieldOffsetInfo<T>::FIELD_OFFSETS[Idx]
                        , StructFieldInfoBasedHdf5Support<
                            typename StructFieldTypeInfo<T, Idx>::TheType
                        >::TheType()
                    );
                    fillInType<Idx+1>(ret);
                }
            }
        public:
            static constexpr bool HasSupport = allHasSupport<0>();
            static H5::DataType TheType() {
                H5::CompType ret {sizeof(T)};
                fillInType<0>(ret);
                return ret;
            }
        };
    }

    template <class T>
    class StructFieldInfoBasedHdf5Utils {
    public:
        static H5::DataType hdf5DataType() {
            return internal::StructFieldInfoBasedHdf5Support<T>::TheType();
        }
        static void write(std::vector<T> const &data, std::string const &fileName, std::string const &datasetName) {
            hsize_t dim[1] = {data.size()};
            H5::DataSpace space(1, dim);
            auto file = std::make_unique<H5::H5File>(
                fileName
                , H5F_ACC_RDWR|H5F_ACC_CREAT
            );
            auto theType = internal::StructFieldInfoBasedHdf5Support<T>::TheType();
            std::vector<std::string> parts;
            boost::split(parts, datasetName, boost::is_any_of("/"));
            std::vector<std::string> realParts;
            for (auto const &p : parts) {
                auto p1 = boost::trim_copy(p);
                if (p1 != "") {
                    realParts.push_back(p1);
                }
            }
            if (realParts.empty()) {
                return;
            }
            std::vector<std::unique_ptr<H5::Group>> groups;
            for (std::size_t ii=0; ii<realParts.size()-1; ++ii) {
                std::unique_ptr<H5::Group> gr;
                try {
                    if (ii == 0) {
                        gr = std::make_unique<H5::Group>(
                            file->openGroup(realParts[ii])
                        );
                    } else {
                        gr = std::make_unique<H5::Group>(
                            groups.back()->openGroup(realParts[ii])
                        );
                    }
                } catch (H5::Exception const &) {
                    if (ii == 0) {
                        gr = std::make_unique<H5::Group>(
                            file->createGroup(realParts[ii])
                        );
                    } else {
                        gr = std::make_unique<H5::Group>(
                            groups.back()->createGroup(realParts[ii])
                        );
                    }
                }
                groups.push_back(std::move(gr));
            }
            std::unique_ptr<H5::DataSet> dataset;
            try {
                if (groups.empty()) {
                    file->unlink(realParts.back());
                } else {
                    groups.back()->unlink(realParts.back());
                }
            } catch (H5::Exception const &) {
            }
            if (groups.empty()) {
                dataset = std::make_unique<H5::DataSet>(
                    file->createDataSet(
                        realParts.back()
                        , theType
                        , space
                    )
                );
            } else {
                dataset = std::make_unique<H5::DataSet>(
                    groups.back()->createDataSet(
                        realParts.back()
                        , theType
                        , space
                    )
                );
            }
            dataset->write(data.data(), theType);
        }
        static void read(std::vector<T> &data, std::string const &fileName, std::string const &datasetName) {
            auto file = std::make_unique<H5::H5File>(
                fileName
                , H5F_ACC_RDONLY
            );
            auto theType = internal::StructFieldInfoBasedHdf5Support<T>::TheType();
            std::vector<std::string> parts;
            boost::split(parts, datasetName, boost::is_any_of("/"));
            std::vector<std::string> realParts;
            for (auto const &p : parts) {
                auto p1 = boost::trim_copy(p);
                if (p1 != "") {
                    realParts.push_back(p1);
                }
            }
            if (realParts.empty()) {
                return;
            }
            std::vector<std::unique_ptr<H5::Group>> groups;
            for (std::size_t ii=0; ii<realParts.size()-1; ++ii) {
                std::unique_ptr<H5::Group> gr;
                if (ii == 0) {
                    gr = std::make_unique<H5::Group>(
                        file->openGroup(realParts[ii])
                    );
                } else {
                    gr = std::make_unique<H5::Group>(
                        groups.back()->openGroup(realParts[ii])
                    );
                }
                groups.push_back(std::move(gr));
            }
            std::unique_ptr<H5::DataSet> dataset;
            if (groups.empty()) {
                dataset = std::make_unique<H5::DataSet>(
                    file->openDataSet(realParts.back())
                );
            } else {
                dataset = std::make_unique<H5::DataSet>(
                    groups.back()->openDataSet(realParts.back())
                );
            }
            data.resize(dataset->getSpace().getSimpleExtentNpoints());
            dataset->read(data.data(), theType);
        }
    };

} } } } }

#endif