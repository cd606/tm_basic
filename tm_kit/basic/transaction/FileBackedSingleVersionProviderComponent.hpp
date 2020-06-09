#ifndef TM_KIT_BASIC_TRANSACTION_FILE_BACKED_SINGLE_VERSION_PROVIDER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_FILE_BACKED_SINGLE_VERSION_PROVIDER_COMPONENT_HPP_

#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>
#include <cstring>

#include <tm_kit/basic/transaction/VersionProviderComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    //This implementation does not lock the file, it is suitable for
    //preserving version history across different runs, but not for
    //across simultaneous processes
    //It is named 'single' because there is only one version number for
    //all the keys
    template <class KeyType, class DataType>
    class FileBackedSingleVersionProviderComponent
        : public VersionProviderComponent<KeyType,DataType,int64_t> {
    private:
        std::mutex mutex_;
        int64_t version_;
        std::unique_ptr<std::ofstream> ofs_;
    public:
        FileBackedSingleVersionProviderComponent()
            : mutex_(), version_(0), ofs_()
        {}
        FileBackedSingleVersionProviderComponent(std::string const &filePath) 
            : mutex_(), version_(0), ofs_()
        {
            if (std::filesystem::exists(filePath)) {
                std::ifstream ifs(filePath);
                if (ifs.good()) {
                    char buf[sizeof(int64_t)];
                    ifs.read(buf, sizeof(int64_t));
                    ifs.close();
                    std::memcpy(&version_, buf, sizeof(int64_t));
                }
            } 
            ofs_ = std::make_unique<std::ofstream>(filePath);
            ofs_->seekp(0);
            ofs_->write(reinterpret_cast<const char *>(&version_), sizeof(int64_t));
            ofs_->flush();
        }
        FileBackedSingleVersionProviderComponent(FileBackedSingleVersionProviderComponent &&f) 
            : mutex_(), version_(std::move(f.version_)), ofs_(std::move(f.ofs_))
        {}
        FileBackedSingleVersionProviderComponent &operator=(FileBackedSingleVersionProviderComponent &&f) 
        {
            if (this != &f) {
                std::lock_guard<std::mutex> _(mutex_);
                version_ = std::move(f.version_);
                ofs_ = std::move(f.ofs_);
            }
            return *this;
        }
        virtual ~FileBackedSingleVersionProviderComponent() {
            std::lock_guard<std::mutex> _(mutex_);
            if (ofs_) {
                ofs_->close();
            }
        }
        virtual int64_t getNextVersionForKey(KeyType const &, DataType const *) {
            std::lock_guard<std::mutex> _(mutex_);
            ++version_;
            if (ofs_) {
                ofs_->seekp(0);
                ofs_->write(reinterpret_cast<const char *>(&version_), sizeof(int64_t));
                ofs_->flush();
            }
            return version_;
        }
    };

} } } } }
#endif