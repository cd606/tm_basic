#ifndef TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_HPP_
#define TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_HPP_

#include <iostream>
#include <chrono>
#include <cstddef>
#include <vector>
#include <cstring>
#include <boost/endian/conversion.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/basic/ByteData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class TimeDuration, class TimeStorageT=int64_t, class TopicLenT=uint32_t, class ContentLenT=uint32_t, bool HasFinal=true>
    struct ByteDataWithTopicRecordFileFormat {
        using Duration = TimeDuration;
        using TimeStorageType = TimeStorageT;
        using TopicLengthType = TopicLenT;
        using ContentLengthType = ContentLenT;
        static constexpr bool HasFinalFlag = HasFinal;
    };
    template <class Format>
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
            typename Format::TimeStorageType t = boost::endian::native_to_little<typename Format::TimeStorageType>(infra::withtime_utils::sinceEpoch<typename Format::Duration>(d.timePoint));
            os.write(reinterpret_cast<char *>(&t), sizeof(typename Format::TimeStorageType));
            typename Format::TopicLengthType lt = boost::endian::native_to_little<typename Format::TopicLengthType>(d.value.topic.length());
            os.write(reinterpret_cast<char *>(&lt), sizeof(typename Format::TopicLengthType));
            if (lt > 0) {
                os.write(d.value.topic.c_str(), lt);
            }
            typename Format::ContentLengthType ld = boost::endian::native_to_little<typename Format::ContentLengthType>(d.value.content.length());
            os.write(reinterpret_cast<char *>(&ld), sizeof(typename Format::ContentLengthType));
            if (ld > 0) {
                os.write(d.value.content.c_str(), ld);
            }
            if (Format::HasFinalFlag) {
                std::byte b = (std::byte) (d.finalFlag?1:0);
                os.write(reinterpret_cast<char *>(&b), 1);
            }
        }
    };

    template <class Format>
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
            char buf[sizeof(typename Format::TimeStorageType)];
            typename Format::TimeStorageType t;
            is.read(buf, sizeof(typename Format::TimeStorageType));
            if (is.gcount() != sizeof(typename Format::TimeStorageType)) {
                return std::nullopt;
            }
            std::memcpy(&t, buf, sizeof(typename Format::TimeStorageType));
            t = boost::endian::little_to_native<typename Format::TimeStorageType>(t);
            infra::WithTime<ByteDataWithTopic, std::chrono::system_clock::time_point> ret;
            ret.timePoint = infra::withtime_utils::epochDurationToTime<typename Format::Duration>(t);

            typename Format::TopicLengthType lt;
            is.read(buf, sizeof(typename Format::TopicLengthType));
            if (is.gcount() != sizeof(typename Format::TopicLengthType)) {
                return std::nullopt;
            }
            std::memcpy(&lt, buf, sizeof(typename Format::TopicLengthType));
            lt = boost::endian::little_to_native<typename Format::TopicLengthType>(lt);
            if (lt > 0) {
                ret.value.topic.resize(lt);
                is.read(&ret.value.topic[0], lt);
                if (is.gcount() != lt) {
                    return std::nullopt;
                }
            }
            typename Format::ContentLengthType ld;
            is.read(buf, sizeof(typename Format::ContentLengthType));
            if (is.gcount() != sizeof(typename Format::ContentLengthType)) {
                return std::nullopt;
            }
            std::memcpy(&ld, buf, sizeof(typename Format::ContentLengthType));
            ld = boost::endian::little_to_native<typename Format::ContentLengthType>(ld);
            if (ld > 0) {
                ret.value.content.resize(ld);
                is.read(&ret.value.content[0], ld);
                if (is.gcount() != ld) {
                    return std::nullopt;
                }
            }
            if (Format::HasFinalFlag) {
                std::byte b;
                is.read(reinterpret_cast<char *>(&b), 1);
                if (is.gcount() != 1) {
                    return std::nullopt;
                }
                ret.finalFlag = ((int) b != 0); 
            } else {
                ret.finalFlag = false;
            }
            if (is.eof()) {
                ret.finalFlag = true;
            }
            return ret;
        }
    };

} } } }

#endif