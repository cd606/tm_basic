#ifndef TM_KIT_BASIC_TRANSACTION_FILE_BACKED_SINGLE_VERSION_PROVIDER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_FILE_BACKED_SINGLE_VERSION_PROVIDER_COMPONENT_HPP_

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <mutex>
#include <cstring>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    //This implementation does not lock the file, it is suitable for
    //preserving version history across different runs, but not for
    //across simultaneous processes
    //It is named 'single' because there is only one version number for
    //all the keys
    template <class KeyType, class DataType>
    class FileBackedSingleVersionProviderComponent final
        : public VersionProviderComponent<KeyType,DataType,int64_t> {
    private:
        std::mutex mutex_;
        int64_t version_;
        std::ofstream ofs_;
    public:
        FileBackedSingleVersionProviderComponent(std::string const &filePath) 
            : mutex_(), version_(0), ofs_()
        {
            if (boost::filesystem::exists(filePath)) {
                std::ifstream ifs(filePath);
                if (ifs.good()) {
                    char buf[sizeof(int64_t)];
                    ifs.read(buf, sizeof(int64_t));
                    ifs.close();
                    std::memcpy(&version_, buf, sizeof(int64_t));
                }
            } 
            ofs_ = std::ofstream(filePath);
        }
        virtual ~FileBackedSingleVersionProviderComponent() {
            ofs_.close();
        }
        virtual VersionType getNextVersionForKey(KeyType const &) {
            std::lock_guard<std::mutex> _(mutex_);
            ++version_;
            ofs_.seekp(0);
            ofs_.write(&version_, sizeof(int64_t));
            ofs_.flush();
            return version_;
        }
    };

} } } } }
#endif