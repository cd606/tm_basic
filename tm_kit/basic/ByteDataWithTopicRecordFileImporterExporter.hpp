#ifndef TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_IMPORTER_EXPORTER_HPP_
#define TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_IMPORTER_EXPORTER_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeMonad.hpp>
#include <tm_kit/infra/SinglePassIterationMonad.hpp>
#include <tm_kit/basic/ByteDataWithTopicRecordFile.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class Monad, class TP=typename Monad::TimePoint>
    class ByteDataWithTopicRecordFileImporterExporter {
    };

    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::RealTimeMonad<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Duration>
        static std::shared_ptr<typename infra::RealTimeMonad<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalI : public infra::RealTimeMonad<Env>::template AbstractImporter<ByteDataWithTopic> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Duration> reader_;
                Env *env_;
                std::thread th_;
                void run() {
                    reader_.startReadingByteDataWithTopicRecordFile(*is_);
                    while (true) {
                        auto d = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d) {
                            break;
                        }
                        auto t = env_->now();
                        if (d->timePoint < t) {
                            continue;
                        } else if (d->timePoint == t) {
                            this->publish(typename infra::RealTimeMonad<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d
                            });
                        } else {
                            env_->sleepFor(d->timePoint-t);
                            this->publish(typename infra::RealTimeMonad<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d
                            });
                        }
                    }
                }
            public:
                LocalI(std::istream &is, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : is_(&is), reader_(fileMagic, recordMagic), env_(nullptr), th_() {}
                virtual void start(Env *env) override final {
                    env_ = env;
                    th_ = std::thread(&LocalI::run, this);
                    th_.detach();
                }
            };
            return infra::RealTimeMonad<Env>::template importer(new LocalI(is, fileMagic, recordMagic));
        }
        
        template <class Duration>
        static std::shared_ptr<typename infra::RealTimeMonad<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalE final : public infra::RealTimeMonad<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Duration> writer_;
            public:
                LocalE(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
                virtual void handle(typename infra::RealTimeMonad<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            return infra::RealTimeMonad<Env>::template exporter(new LocalE(os, fileMagic, recordMagic));
        }

    };

    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::SinglePassIterationMonad<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Duration>
        static std::shared_ptr<typename infra::SinglePassIterationMonad<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalI final : public infra::SinglePassIterationMonad<Env>::template AbstractImporter<ByteDataWithTopic> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Duration> reader_;
                Env *env_;
            public:
                LocalI(std::istream &is, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : is_(&is), reader_(fileMagic, recordMagic), env_(nullptr) {}
                virtual void start(Env *env) override final {
                    reader_.startReadingByteDataWithTopicRecordFile(*is_);
                    env_ = env;
                }
                virtual typename infra::SinglePassIterationMonad<Env>::template Data<ByteDataWithTopic> 
                generate() override final {
                    auto d = reader_.readByteDataWithTopicRecord(*is_);
                    if (!d) {
                        //for bad data, return a final input
                        return typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> {
                            env_, 
                            {env_->now(), ByteDataWithTopic {}, true}
                        };
                    }
                    return typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> {
                        env_, *d
                    };
                }
            };
            return infra::SinglePassIterationMonad<Env>::template importer(new LocalI(is, fileMagic, recordMagic));
        }

        template <class Duration>
        static std::shared_ptr<typename infra::SinglePassIterationMonad<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalE final : public infra::SinglePassIterationMonad<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Duration> writer_;
            public:
                LocalE(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
                virtual void handle(typename infra::RealTimeMonad<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            return infra::SinglePassIterationMonad<Env>::template exporter(new LocalE(os, fileMagic, recordMagic));
        }
    };

} } } }

#endif