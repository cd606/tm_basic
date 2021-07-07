#ifndef TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_IMPORTER_EXPORTER_HPP_
#define TM_KIT_BASIC_BYTE_DATA_WITH_TOPIC_RECORD_FILE_IMPORTER_EXPORTER_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/basic/ByteDataWithTopicRecordFile.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class App, class TP=typename App::TimePoint>
    class ByteDataWithTopicRecordFileImporterExporter {
    };

    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::RealTimeApp<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Format, bool PublishFinalEmptyMessage=false>
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>(), bool overrideDate=false) {
            class LocalI : public infra::RealTimeApp<Env>::template AbstractImporter<ByteDataWithTopic>, public virtual infra::IControllableNode<Env> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Format> reader_;
                Env *env_;
                std::thread th_;
                bool overrideDate_;
                std::optional<std::chrono::system_clock::time_point> virtualToday_;
                std::atomic<bool> running_;

                void run() {
                    reader_.startReadingByteDataWithTopicRecordFile(*is_);
                    while (running_) {
                        TM_INFRA_IMPORTER_TRACER(env_);
                        auto t = env_->now();
                        while (true) {
                            auto d = reader_.readByteDataWithTopicRecord(*is_);
                            if (!d) {
                                if constexpr (PublishFinalEmptyMessage) {
                                    this->publish(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> {
                                        env_, {env_->now(), ByteDataWithTopic {std::string{}, std::string{}}, true}
                                    });
                                }
                                running_ = false;
                                break;
                            }
                            
                            if (overrideDate_) {
    #ifdef _MSC_VER
                                auto sinceMidnight = infra::withtime_utils::sinceMidnight<std::chrono::microseconds>(d->timePoint);
    #else
                                auto sinceMidnight = infra::withtime_utils::sinceMidnight<std::chrono::nanoseconds>(d->timePoint);
    #endif
                                if (!virtualToday_) {
                                    std::time_t tt = std::chrono::system_clock::to_time_t(t);
                                    std::tm *m = std::localtime(&tt);
                                    m->tm_hour = 0;
                                    m->tm_min = 0;
                                    m->tm_sec = 0;
                                    m->tm_isdst = -1;
                                    virtualToday_ =  std::chrono::system_clock::from_time_t(std::mktime(m));
                                }
    #ifdef _MSC_VER
                                d->timePoint = *virtualToday_+std::chrono::microseconds(sinceMidnight);
    #else
                                d->timePoint = *virtualToday_+std::chrono::nanoseconds(sinceMidnight);
    #endif
                            }
                            if (d->timePoint < t) {
                                if (d->timePoint >= t-std::chrono::seconds(1)) {
                                    this->publish(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> {
                                        env_, *d
                                    });
                                } else {
                                    continue;
                                }
                            } else if (d->timePoint == t) {
                                this->publish(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> {
                                    env_, *d
                                });
                            } else {
                                env_->sleepFor(d->timePoint-t);
                                this->publish(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> {
                                    env_, *d
                                });
                                break;
                            }
                        }
                    }
                }
            public:
                LocalI(std::istream &is, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic, bool overrideDate) : is_(&is), reader_(fileMagic, recordMagic), env_(nullptr), th_(), overrideDate_(overrideDate), virtualToday_(std::nullopt), running_(false) {}
                virtual void start(Env *env) override final {
                    env_ = env;
                    running_ = true;
                    th_ = std::thread(&LocalI::run, this);
                    th_.detach();
                }
                virtual void control(Env *env, std::string const &command, std::vector<std::string> const &params) override final {
                    if (command == "stop") {
                        running_ = false;
                        try {
                            th_.join();
                        } catch (...) {
                        }
                    }
                }
            };
            return infra::RealTimeApp<Env>::template importer(new LocalI(is, fileMagic, recordMagic, overrideDate));
        }
        
        template <class Format>
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>(), bool separateThread=false) {
            class LocalENonThreaded final : public infra::RealTimeApp<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
            public:
                LocalENonThreaded(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
                virtual void handle(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            class LocalEThreaded final : public infra::RealTimeApp<Env>::template AbstractExporter<ByteDataWithTopic>, public infra::RealTimeAppComponents<Env>::template ThreadedHandler<ByteDataWithTopic,LocalEThreaded> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
            public:
            #ifdef _MSC_VER
                LocalEThreaded(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
            #else
                LocalEThreaded(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : infra::RealTimeAppComponents<Env>::template ThreadedHandler<ByteDataWithTopic,LocalEThreaded>(), os_(&os), writer_(fileMagic, recordMagic) {}
            #endif
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
            public:
                void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> &&d) {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
                void idleWork() {}
            };
            if (separateThread) {
                return infra::RealTimeApp<Env>::template exporter(new LocalEThreaded(os, fileMagic, recordMagic));
            } else {
                return infra::RealTimeApp<Env>::template exporter(new LocalENonThreaded(os, fileMagic, recordMagic));
            }           
        }

    };

    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::SinglePassIterationApp<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Format>
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalI final : public infra::SinglePassIterationApp<Env>::template AbstractImporter<ByteDataWithTopic> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Format> reader_;
                Env *env_;
                std::optional<typename infra::SinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic>> data_;
            public:
                LocalI(std::istream &is, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : is_(&is), reader_(fileMagic, recordMagic), env_(nullptr), data_(std::nullopt) {}
                virtual void start(Env *env) override final {
                    reader_.startReadingByteDataWithTopicRecordFile(*is_);
                    env_ = env;
                }
                virtual typename infra::SinglePassIterationApp<Env>::template Data<ByteDataWithTopic> 
                generate(ByteDataWithTopic const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    if (!data_) {
                        //the first read
                        auto d = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d) {
                            return { typename infra::SinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                                env_, 
                                {env_->now(), ByteDataWithTopic {}, true}
                            } };
                        }
                        auto ret = typename infra::SinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                            env_, *d
                        };
                        auto d1 = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d1) {
                            ret.timedData.finalFlag = true;
                        } else {
                            data_ = typename infra::SinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
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
                            data_ = typename infra::SinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d
                            };
                        }
                        return ret;
                    }
                }
            };
            return infra::SinglePassIterationApp<Env>::template importer(new LocalI(is, fileMagic, recordMagic));
        }

        template <class Format>
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalE final : public infra::SinglePassIterationApp<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
            public:
                LocalE(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
                virtual void handle(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            return infra::SinglePassIterationApp<Env>::template exporter(new LocalE(os, fileMagic, recordMagic));
        }
    };


    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::TopDownSinglePassIterationApp<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Format>
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalI final : public infra::TopDownSinglePassIterationApp<Env>::template AbstractImporter<ByteDataWithTopic> {
            private:
                std::istream *is_;
                ByteDataWithTopicRecordFileReader<Format> reader_;
                Env *env_;
                std::optional<typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic>> data_;
            public:
                LocalI(std::istream &is, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : is_(&is), reader_(fileMagic, recordMagic), env_(nullptr), data_(std::nullopt) {}
                virtual void start(Env *env) override final {
                    reader_.startReadingByteDataWithTopicRecordFile(*is_);
                    env_ = env;
                }
                virtual std::tuple<bool, typename infra::TopDownSinglePassIterationApp<Env>::template Data<ByteDataWithTopic>> 
                generate(ByteDataWithTopic const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    if (!data_) {
                        //the first read
                        auto d = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d) {
                            return {false, { typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                                env_, 
                                {env_->now(), ByteDataWithTopic {}, true}
                            } } };
                        }
                        auto ret = typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                            env_, *d
                        };
                        auto d1 = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d1) {
                            ret.timedData.finalFlag = true;
                        } else {
                            data_ = typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d1
                            };
                        }
                        return {!(ret.timedData.finalFlag), std::move(ret)};
                    } else {
                        auto ret = std::move(*data_);
                        auto d = reader_.readByteDataWithTopicRecord(*is_);
                        if (!d) {
                            ret.timedData.finalFlag = true;
                            data_ = std::nullopt;
                        } else {
                            data_ = typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<ByteDataWithTopic> {
                                env_, *d
                            };
                        }
                        return {!(ret.timedData.finalFlag), std::move(ret)};
                    }
                }
            };
            return infra::TopDownSinglePassIterationApp<Env>::template importer(new LocalI(is, fileMagic, recordMagic));
        }

        template <class Format>
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            class LocalE final : public infra::TopDownSinglePassIterationApp<Env>::template AbstractExporter<ByteDataWithTopic> {
            private:
                std::ostream *os_;
                ByteDataWithTopicRecordFileWriter<Format> writer_;
            public:
                LocalE(std::ostream &os, std::vector<std::byte> const &fileMagic, std::vector<std::byte> const &recordMagic) : os_(&os), writer_(fileMagic, recordMagic) {}
                virtual void start(Env *) override final {
                    writer_.startWritingByteDataWithTopicRecordFile(*os_);
                }
                virtual void handle(typename infra::RealTimeApp<Env>::template InnerData<ByteDataWithTopic> &&d) override final {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeByteDataWithTopicRecord(*os_, d.timedData);
                    os_->flush();
                }
            };
            return infra::TopDownSinglePassIterationApp<Env>::template exporter(new LocalE(os, fileMagic, recordMagic));
        }
    };

    template <class Env>
    class ByteDataWithTopicRecordFileImporterExporter<infra::BasicWithTimeApp<Env>, std::chrono::system_clock::time_point> {
    public:
        template <class Format, bool PublishFinalEmptyMessage=false>
        static std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template Importer<ByteDataWithTopic>>
        createImporter(std::istream &is, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>()) {
            return infra::BasicWithTimeApp<Env>::template vacuousImporter<ByteDataWithTopic>();
        }
        
        template <class Format>
        static std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template Exporter<ByteDataWithTopic>>
        createExporter(std::ostream &os, std::vector<std::byte> const &fileMagic=std::vector<std::byte>(), std::vector<std::byte> const &recordMagic=std::vector<std::byte>(), bool separateThread=false) {
            return infra::BasicWithTimeApp<Env>::template trivialExporter<ByteDataWithTopic>();         
        }

    };

} } } }

#endif