#ifndef TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_HPP_
#define TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_HPP_

#include <iostream>
#include <chrono>
#include <cstddef>
#include <vector>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/basic/ByteData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class Duration>
    class ByteDataWithTopicRecordFileWriter {
    private:
        std::vector<std::byte> fileHeaderMagic_;
        std::vector<std::byte> recordHeaderMagic_;
    public:
        ByteDataWithTopicRecordFileWriter(std::vector<std::byte> const &fileHeaderMagic, std::vector<std::byte> const &recordHeaderMagic)
            : fileHeaderMagic_(fileHeaderMagic), recordHeaderMagic_(recordHeaderMagic) {}
        void startWritingByteDataWithTopicRecordFile(std::ostream &os) {
            if (!fileHeaderMagic_.empty()) {
                os.write(reinterpret_cast<char *>(fileHeaderMagic_.data()), fileHeaderMagic_.size());
            }
        }
        void writeByteDataWithTopicRecord(std::ostream &os, infra::WithTime<ByteDataWithTopic, std::chrono::system_clock::time_point> const &d) {
            if (!recordHeaderMagic_.empty()) {
                os.write(reinterpret_cast<char *>(recordHeaderMagic_.data()), recordHeaderMagic_.size());
            }
            int64_t t = infra::withtime_utils::sinceEpoch<Duration>(d.timePoint);
            os.write(reinterpret_cast<char *>(&t), sizeof(int64_t));
            uint16_t lt = d.value.topic.length();
            os.write(reinterpret_cast<char *>(&lt), sizeof(uint16_t));
            if (lt > 0) {
                os.write(d.value.topic.c_str(), lt);
            }
            uint32_t ld = d.value.content.length();
            os.write(reinterpret_cast<char *>(&ld), sizeof(uint32_t));
            if (ld > 0) {
                os.write(d.value.content.c_str(), ld);
            }
            std::byte b = (std::byte) (d.finalFlag?1:0);
            os.write(reinterpret_cast<char *>(&b), 1);
        }
    };

    template <class Duration>
    class ByteDataWithTopicRecordFileReader {
    private:
        std::vector<std::byte> fileHeaderMagic_;
        std::vector<std::byte> recordHeaderMagic_;
        bool readMagic(std::istream &is, std::vector<std::byte> const &magic) {
            if (!magic.empty()) {
                std::size_t pos = 0;
                std::byte b;
                while (pos < magic.size()) {
                    while (!is.eof()) {
                        is.read(reinterpret_cast<char *>(&b), 1);
                        if (is.gcount() != 1) {
                            return false;
                        }
                        if (b == magic[pos]) {
                            ++pos;
                            break;
                        } else if (b == magic[0]) {
                            pos = 1;
                            break;
                        } else {
                            pos = 0;
                        }
                    }
                    if (is.eof()) {
                        break;
                    }
                }
                return (pos == magic.size());
            } else {
                return !is.eof();
            }
        }
    public:
        ByteDataWithTopicRecordFileReader(std::vector<std::byte> const &fileHeaderMagic, std::vector<std::byte> const &recordHeaderMagic)
            : fileHeaderMagic_(fileHeaderMagic), recordHeaderMagic_(recordHeaderMagic) {}
        bool startReadingByteDataWithTopicRecordFile(std::istream &is) {
            return readMagic(is, fileHeaderMagic_);
        }
        infra::OptionalWithTime<ByteDataWithTopic, std::chrono::system_clock::time_point> readByteDataWithTopicRecord(std::istream &is) {
            if (!readMagic(is, recordHeaderMagic_)) {
                return std::nullopt;
            }
            int64_t t;
            is.read(reinterpret_cast<char *>(&t), sizeof(int64_t));
            if (is.gcount() != sizeof(int64_t)) {
                return std::nullopt;
            }
            infra::WithTime<ByteDataWithTopic, std::chrono::system_clock::time_point> ret;
            ret.timePoint = infra::withtime_utils::epochDurationToTime<Duration>(t);

            uint16_t lt;
            is.read(reinterpret_cast<char *>(&lt), sizeof(uint16_t));
            if (is.gcount() != sizeof(uint16_t)) {
                return std::nullopt;
            }
            if (lt > 0) {
                ret.value.topic.resize(lt);
                is.read(&ret.value.topic[0], lt);
                if (is.gcount() != lt) {
                    return std::nullopt;
                }
            }
            uint32_t ld;
            is.read(reinterpret_cast<char *>(&ld), sizeof(uint32_t));
            if (is.gcount() != sizeof(uint32_t)) {
                return std::nullopt;
            }
            if (ld > 0) {
                ret.value.content.resize(ld);
                is.read(&ret.value.content[0], ld);
                if (is.gcount() != ld) {
                    return std::nullopt;
                }
            }
            std::byte b;
            is.read(reinterpret_cast<char *>(&b), 1);
            if (is.gcount() != 1) {
                return std::nullopt;
            }
            ret.finalFlag = ((int) b != 0);

            if (is.eof()) {
                ret.finalFlag = true;
            }
            return ret;
        }
    };

} } } }

#endif