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
        template <class Format>
        static std::shared_ptr<typename infra::RealTimeMonad<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalI : public infra::RealTimeMonad<Env>::template AbstractImporter<ByteDataWithTopic> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Format> reader_;
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
        
        template <class Format>
        static std::shared_ptr<typename infra::RealTimeMonad<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>(), bool separateThread=false) {
            class LocalENonThreaded final : public infra::RealTimeMonad<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
            public:
                LocalENonThreaded(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
                virtual void handle(typename infra::RealTimeMonad<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            class LocalEThreaded final : public infra::RealTimeMonad<Env>::template AbstractExporter<ByteDataWithTopic>, public infra::RealTimeMonadComponents<Env>::template ThreadedHandler<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
            public:
            #ifdef _MSC_VER
                LocalEThreaded(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
            #else
                LocalEThreaded(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : infra::RealTimeMonadComponents<Env>::template ThreadedHandler<ByteDataWithTopic>(), os_(&os), writer_(fileMagic, recordMagic) {}
            #endif
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
            private:
                virtual void actuallyHandle(typename infra::RealTimeMonad<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            if (separateThread) {
                return infra::RealTimeMonad<Env>::template exporter(new LocalEThreaded(os, fileMagic, recordMagic));
            } else {
                return infra::RealTimeMonad<Env>::template exporter(new LocalENonThreaded(os, fileMagic, recordMagic));
            }           
        }

    };

    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::SinglePassIterationMonad<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Format>
        static std::shared_ptr<typename infra::SinglePassIterationMonad<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalI final : public infra::SinglePassIterationMonad<Env>::template AbstractImporter<ByteDataWithTopic> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Format> reader_;
                Env *env_;
                std::optional<typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic>> data_;
            public:
                LocalI(std::istream &is, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : is_(&is), reader_(fileMagic, recordMagic), env_(nullptr), data_(std::nullopt) {}
                virtual void start(Env *env) override final {
                    reader_.startReadingByteDataWithTopicRecordFile(*is_);
                    env_ = env;
                }
                virtual typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> 
                generate() override final {
                    if (!data_) {
                        //the first read
                        auto d = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d) {
                            return typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> {
                                env_, 
                                {env_->now(), ByteDataWithTopic {}, true}
                            };
                        }
                        auto ret = typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> {
                            env_, *d
                        };
                        auto d1 = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d1) {
                            ret.timedData.finalFlag = true;
                        } else {
                            data_ = typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d1
                            };
                        }
                        return ret;
                    } else {
                        auto ret = std::move(*data_);
                        auto d = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d) {
                            ret.timedData.finalFlag = true;
                            data_ = std::nullopt;
                        } else {
                            data_ = typename infra::SinglePassIterationMonad<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d
                            };
                        }
                        return ret;
                    }
                }
            };
            return infra::SinglePassIterationMonad<Env>::template importer(new LocalI(is, fileMagic, recordMagic));
        }

        template <class Format>
        static std::shared_ptr<typename infra::SinglePassIterationMonad<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalE final : public infra::SinglePassIterationMonad<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
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