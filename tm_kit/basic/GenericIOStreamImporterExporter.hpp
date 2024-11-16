#ifndef TM_KIT_BASIC_GENERIC_IO_STREAM_IMPORTER_EXPORTER_HPP_
#define TM_KIT_BASIC_GENERIC_IO_STREAM_IMPORTER_EXPORTER_HPP_

#include <type_traits>
#include <iostream>
#include <chrono>
#include <cstddef>
#include <vector>
#include <cstring>
#include <string_view>
#include <boost/endian/conversion.hpp>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/basic/ByteDataWithTopicRecordFile.hpp>
#include <tm_kit/basic/ByteData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    template <class App>
    class GenericIOStreamImporterExporter {
    };

    template <class App>
    struct OneGenericIStreamImporterSpec {
        std::shared_ptr<std::istream> stream;
        std::function<typename App::Duration(typename App::TimePoint const &)> delayer;
    };
    template <class App>
    using GenericIStreamImporterSpec = std::vector<OneGenericIStreamImporterSpec<App>>;

    class GenericIOStreamImporterExporterHelper {
    private:
        template <class App, class T, class DataComparer=std::less<T>>
        struct QueueItem {
            typename App::TimePoint tp;
            T data;
            OneGenericIStreamImporterSpec<App> *spec;
            DataComparer *comparer;
            std::size_t index;

            bool operator<(QueueItem const &item) const {
                if (this == &item) {
                    return false;
                }
                if (tp > item.tp) {
                    return true;
                }
                if (tp < item.tp) {
                    return false;
                }
                if ((*comparer)(item.data, data)) {
                    return true;
                }
                if ((*comparer)(data, item.data)) {
                    return false;
                }
                return (index > item.index);
            }
        };
    public:
        template <class App, class T, class DataComparer=std::less<T>>
        using Queue = std::priority_queue<QueueItem<App,T,DataComparer>>;

        template <class App, class T, class Reader, class DataComparer>
        static void startQueue(typename App::EnvironmentType *env, Queue<App,T,DataComparer> &queue, GenericIStreamImporterSpec<App> &spec, Reader &reader, DataComparer &comparer) {
            for (std::size_t ii=0; ii<spec.size(); ++ii) {
                reader.start(env, *(spec[ii].stream));
                std::optional<std::tuple<typename App::TimePoint,T>> data = reader.readOne(env, *(spec[ii].stream));
                if (data) {
                    auto tp = std::get<0>(*data);
                    if (spec[ii].delayer) {
                        tp += spec[ii].delayer(tp);
                    }
                    queue.push(
                        QueueItem<App,T,DataComparer> {
                            tp, std::move(std::get<1>(*data)), &(spec[ii]), &comparer, ii
                        }
                    );
                }
            }
        }
        template <class App, class T, class Reader, class DataComparer>
        static typename App::template Data<T> stepQueue(typename App::EnvironmentType *env, Queue<App,T,DataComparer> &queue, Reader &reader) {
            if (queue.empty()) {
                return std::nullopt;
            }
            auto item = queue.top();
            queue.pop();
            std::optional<std::tuple<typename App::TimePoint,T>> data = reader.readOne(env, *(item.spec->stream));
            if (data) {
                auto tp = std::get<0>(*data);
                if (item.spec->delayer) {
                    tp += item.spec->delayer(tp);
                }
                queue.push(
                    QueueItem<App,T,DataComparer> {
                        tp, std::move(std::get<1>(*data)), item.spec, item.comparer, item.index
                    }
                );
            }
            return typename App::template InnerData<T> {
                env 
                , {
                    item.tp 
                    , item.data 
                    , queue.empty()
                }
            };
        }

        template <std::size_t BufferSize>
        class BufferedReader {
        private:
            std::unique_ptr<char[]> buf_[2];
            int bufIndex_;
            std::size_t bufPos_[2], bufEndPos_[2];
            bool fileEof_;
            bool done_;
        public:
            BufferedReader() 
                : buf_{
                    std::make_unique<char[]>(BufferSize)
                    , std::make_unique<char[]>(BufferSize)
                }
                , bufIndex_(0)
                , bufPos_{0,0}
                , bufEndPos_{0,0}
                , fileEof_(false)
                , done_(false) 
            {}
            ~BufferedReader() = default;
            BufferedReader(BufferedReader &&) = default;
            BufferedReader &operator=(BufferedReader &&) = default;
            std::string_view read(std::istream &is, std::size_t n) {
                if (n > BufferSize) {
                    return {nullptr, 0};
                }
                if (done_) {
                    return {nullptr, 0};
                }
                if (bufEndPos_[bufIndex_] < bufPos_[bufIndex_]+n) {
                    std::size_t needed = bufPos_[bufIndex_]+n-bufEndPos_[bufIndex_];
                    if (fileEof_) {
                        done_ = true;
                        return {nullptr,0};
                    }
                    is.read(buf_[1-bufIndex_].get(), BufferSize);
                    bufEndPos_[1-bufIndex_] = is.gcount();
                    fileEof_ = (bufEndPos_[1-bufIndex_] != BufferSize);

                    if (bufEndPos_[1-bufIndex_] < needed) {
                        done_ = true;
                        return {nullptr,0};
                    }
                    std::memmove(buf_[bufIndex_].get(), buf_[bufIndex_].get()+bufPos_[bufIndex_], n-needed);
                    std::memcpy(buf_[bufIndex_].get()+(n-needed), buf_[1-bufIndex_].get(), needed);
                    bufPos_[bufIndex_] = 0;
                    bufEndPos_[bufIndex_] = 0;
                    bufPos_[1-bufIndex_] = needed;
                    bufIndex_ = 1-bufIndex_;
                    return {buf_[1-bufIndex_].get(), n};
                } else {
                    bufPos_[bufIndex_] += n;
                    return {buf_[bufIndex_].get()+bufPos_[bufIndex_]-n,n};
                }
            }
        };
    };

    template <class Env>
    class GenericIOStreamImporterExporter<infra::BasicWithTimeApp<Env>> {
    public:
        template <class T, class Reader, class DataComparer=std::less<T>>
        static std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template Importer<T>>
        createImporter(
            GenericIStreamImporterSpec<infra::BasicWithTimeApp<Env>> const &spec
            , Reader &&reader
            , DataComparer &&dataComparer = DataComparer {}
        ) {
            return infra::BasicWithTimeApp<Env>::template vacuousImporter<T>();
        }
        
        template <class T, class Writer>
        static std::shared_ptr<typename infra::BasicWithTimeApp<Env>::template Exporter<T>>
        createExporter(std::ostream &os, Writer &&writer, bool separateThread=false) {
            return infra::BasicWithTimeApp<Env>::template trivialExporter<T>();           
        }
    };

    template <class Env>
    class GenericIOStreamImporterExporter<infra::RealTimeApp<Env>> {
    public:
        template <class T, class Reader, class DataComparer=std::less<T>>
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<T>>
        createImporter(
            GenericIStreamImporterSpec<infra::RealTimeApp<Env>> const &spec
            , Reader &&reader
            , DataComparer &&dataComparer = DataComparer {}
        ) {
            class LocalI : public infra::RealTimeApp<Env>::template AbstractImporter<T>, public virtual infra::IControllableNode<Env> {
            private:
                GenericIStreamImporterSpec<infra::RealTimeApp<Env>> spec_;
                Reader reader_;
                DataComparer dataComparer_;
                GenericIOStreamImporterExporterHelper::Queue<infra::RealTimeApp<Env>,T,DataComparer> queue_;

                Env *env_;
                std::thread th_;
                std::atomic<bool> running_;

                void run() {
                    GenericIOStreamImporterExporterHelper::template startQueue<infra::RealTimeApp<Env>,T,Reader,DataComparer>(
                        env_, queue_, spec_, reader_, dataComparer_
                    );
                    while (running_) {
                        TM_INFRA_IMPORTER_TRACER(env_);
                        auto t = env_->now();
                        while (true) {
                            auto ret = GenericIOStreamImporterExporterHelper::template stepQueue<infra::RealTimeApp<Env>,T,Reader,DataComparer>(
                                env_, queue_, reader_
                            );
                            if (!ret) {
                                running_ = false;
                                break;
                            }
                            
                            if (ret->timedData.timePoint < t) {
                                if (ret->timedData.timePoint >= t-std::chrono::seconds(1)) {
                                    this->publish(std::move(*ret));
                                } else {
                                    continue;
                                }
                            } else if (ret->timedData.timePoint == t) {
                                this->publish(std::move(*ret));
                            } else {
                                env_->sleepFor(ret->timedData.timePoint-t);
                                this->publish(std::move(*ret));
                                break;
                            }
                        }
                    }
                }
            public:
                LocalI(
                    GenericIStreamImporterSpec<infra::RealTimeApp<Env>> const &spec
                    , Reader &&reader
                    , DataComparer &&dataComparer
                ) :
                    spec_(spec)
                    , reader_(std::move(reader))
                    , dataComparer_(std::move(dataComparer))
                    , queue_()
                    , env_(nullptr)
                    , th_()
                    , running_(false)
                {}
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
            return infra::RealTimeApp<Env>::template importer<T>(new LocalI(spec, std::move(reader), std::move(dataComparer)));
        }

        template <class T, class Reader>
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Importer<T>>
        createImporter(
            std::shared_ptr<std::istream> const &stream
            , Reader &&reader = Reader {}
        ) {
            class LocalI : public infra::RealTimeApp<Env>::template AbstractImporter<T>, public virtual infra::IControllableNode<Env> {
            private:
                std::shared_ptr<std::istream> stream_;
                Reader reader_;
                
                Env *env_;
                std::thread th_;
                std::atomic<bool> running_;

                void run() {
                    reader_.start(env_, *stream_);
                    std::optional<std::tuple<typename infra::RealTimeApp<Env>::TimePoint,T>> data = reader_.readOne(env_, *stream_);
                    if (data) {
                        while (data && running_) {
                            TM_INFRA_IMPORTER_TRACER(env_);
                            std::optional<std::tuple<typename infra::RealTimeApp<Env>::TimePoint,T>> newData = reader_.readOne(env_, *stream_);
                            auto t = env_->now();
                            auto dataT = std::get<0>(*data);
                            if (dataT < t) {
                                if (dataT >= t-std::chrono::seconds(1)) {
                                    this->publish(typename infra::RealTimeApp<Env>::template InnerData<T> {
                                        env_
                                        , {
                                            dataT
                                            , std::move(std::get<1>(*data))
                                            , !((bool) newData)
                                        }
                                    });
                                }
                            } else if (dataT == t) {
                                this->publish(typename infra::RealTimeApp<Env>::template InnerData<T> {
                                    env_
                                    , {
                                        dataT
                                        , std::move(std::get<1>(*data))
                                        , !((bool) newData)
                                    }
                                });
                            } else {
                                env_->sleepFor(dataT-t);
                                this->publish(typename infra::RealTimeApp<Env>::template InnerData<T> {
                                    env_
                                    , {
                                        dataT
                                        , std::move(std::get<1>(*data))
                                        , !((bool) newData)
                                    }
                                });
                            }
                            data = std::move(newData);
                        }
                    }
                }
            public:
                LocalI(
                    std::shared_ptr<std::istream> const &stream
                    , Reader &&reader
                ) :
                    stream_(stream)
                    , reader_(std::move(reader))
                    , env_(nullptr)
                    , th_()
                    , running_(false)
                {}
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
            return infra::RealTimeApp<Env>::template importer<T>(new LocalI(stream, std::move(reader)));
        }
        
        template <class T, class Writer>
        static std::shared_ptr<typename infra::RealTimeApp<Env>::template Exporter<T>>
        createExporter(std::ostream &os, Writer &&writer, bool separateThread=false) {
            class LocalENonThreaded final : public infra::RealTimeApp<Env>::template AbstractExporter<T> {
            private:
                std::ostream *os_;
                Writer writer_;
            public:
                LocalENonThreaded(std::ostream &os, Writer &&writer) : os_(&os), writer_(std::move(writer)) {}
                virtual void start(Env *env) override final {
                    writer_.start(env, *os_);
                }
                virtual void handle(typename infra::RealTimeApp<Env>::template InnerData<T> &&d) override final {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeOne(d.environment, *os_, std::move(d.timedData));
                    os_->flush();
                }
            };
            class LocalEThreaded final : public infra::RealTimeApp<Env>::template AbstractExporter<T>, public infra::RealTimeAppComponents<Env>::template ThreadedHandler<T,LocalEThreaded> {
            private:
                std::ostream *os_;
                Writer writer_;
            public:
            #ifdef _MSC_VER
                LocalEThreaded(std::ostream &os, Writer &&writer) : os_(&os), writer_(std::move(writer)) {
            #else
                LocalEThreaded(std::ostream &os, Writer &&writer) : infra::RealTimeAppComponents<Env>::template ThreadedHandler<T, LocalEThreaded>(), os_(&os), writer_(std::move(writer)) {
            #endif
                    this->startThread();
                }
                virtual void start(Env *env) override final {
                    writer_.start(env, *os_);
                }
            public:
                void actuallyHandle(typename infra::RealTimeApp<Env>::template InnerData<T> &&d) {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeOne(d.environment, *os_, std::move(d.timedData));
                    os_->flush();
                }
                void idleWork() {}
                void waitForStart() {}
            };
            if (separateThread) {
                return infra::RealTimeApp<Env>::template exporter(new LocalEThreaded(os, std::move(writer)));
            } else {
                return infra::RealTimeApp<Env>::template exporter(new LocalENonThreaded(os, std::move(writer)));
            }           
        }
    };

    template <class Env>
    class GenericIOStreamImporterExporter<infra::SinglePassIterationApp<Env>> {
    public:
        template <class T, class Reader, class DataComparer=std::less<T>>
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Importer<T>>
        createImporter(
            GenericIStreamImporterSpec<infra::SinglePassIterationApp<Env>> const &spec
            , Reader &&reader
            , DataComparer &&dataComparer = DataComparer {}
        ) {
            class LocalI final : public infra::SinglePassIterationApp<Env>::template AbstractImporter<T> {
            private:
                GenericIStreamImporterSpec<infra::SinglePassIterationApp<Env>> spec_;
                Reader reader_;
                DataComparer dataComparer_;
                GenericIOStreamImporterExporterHelper::Queue<infra::SinglePassIterationApp<Env>,T,DataComparer> queue_;

                Env *env_;
            public:
                LocalI(
                    GenericIStreamImporterSpec<infra::SinglePassIterationApp<Env>> const &spec
                    , Reader &&reader
                    , DataComparer &&dataComparer
                ) :
                    spec_(spec)
                    , reader_(std::move(reader))
                    , dataComparer_(std::move(dataComparer))
                    , queue_()
                    , env_(nullptr)
                {}
                virtual void start(Env *env) override final {
                    env_ = env;
                    GenericIOStreamImporterExporterHelper::template startQueue<infra::SinglePassIterationApp<Env>,T,Reader,DataComparer>(
                        env_, queue_, spec_, reader_, dataComparer_
                    );
                }
                virtual typename infra::SinglePassIterationApp<Env>::template Data<T> 
                generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    return GenericIOStreamImporterExporterHelper::template stepQueue<infra::SinglePassIterationApp<Env>,T,Reader,DataComparer>(
                        env_, queue_, reader_
                    );
                }
            };
            return infra::SinglePassIterationApp<Env>::template importer<T>(new LocalI(spec, std::move(reader), std::move(dataComparer)));
        }
        
        template <class T, class Reader>
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Importer<T>>
        createImporter(
            std::shared_ptr<std::istream> const &stream
            , Reader &&reader = Reader {}
        ) {
            class LocalI final : public infra::SinglePassIterationApp<Env>::template AbstractImporter<T> {
            private:
                std::shared_ptr<std::istream> stream_;
                Reader reader_;
                std::optional<std::tuple<typename infra::SinglePassIterationApp<Env>::TimePoint,T>> data_;
                
                Env *env_;
            public:
                LocalI(
                    std::shared_ptr<std::istream> const &stream
                    , Reader &&reader
                ) :
                    stream_(stream)
                    , reader_(std::move(reader))
                    , data_(std::nullopt)
                    , env_(nullptr)
                {}
                virtual void start(Env *env) override final {
                    env_ = env;
                    reader_.start(env_, *stream_);
                    data_ = reader_.readOne(env_, *stream_);
                }
                virtual typename infra::SinglePassIterationApp<Env>::template Data<T> 
                generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    if (!data_) {
                        return std::nullopt;
                    }
                    std::optional<std::tuple<typename infra::SinglePassIterationApp<Env>::TimePoint,T>> newData = reader_.readOne(env_, *stream_);
                    auto ret = typename infra::SinglePassIterationApp<Env>::template InnerData<T> {
                        env_
                        , {
                            std::get<0>(*data_)
                            , std::move(std::get<1>(*data_))
                            , !((bool) newData)
                        }
                    };
                    data_ = std::move(newData);
                    return std::move(ret);
                }
            };
            return infra::SinglePassIterationApp<Env>::template importer<T>(new LocalI(stream, std::move(reader)));
        }

        template <class T, class Writer>
        static std::shared_ptr<typename infra::SinglePassIterationApp<Env>::template Exporter<T>>
        createExporter(std::ostream &os, Writer &&writer, bool separateThread=false) {
            class LocalE final : public infra::SinglePassIterationApp<Env>::template AbstractExporter<T> {
            private:
                std::ostream *os_;
                Writer writer_;
            public:
                LocalE(std::ostream &os, Writer &&writer) : os_(&os), writer_(std::move(writer)) {}
                virtual void start(Env *env) override final {
                    writer_.start(env, *os_);
                }
                virtual void handle(typename infra::SinglePassIterationApp<Env>::template InnerData<T> &&d) override final {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeOne(d.environment, *os_, std::move(d.timedData));
                    os_->flush();
                }
            };
            return infra::SinglePassIterationApp<Env>::template exporter(new LocalE(os, std::move(writer)));
        }
    };

    template <class Env>
    class GenericIOStreamImporterExporter<infra::TopDownSinglePassIterationApp<Env>> {
    public:
        template <class T, class Reader, class DataComparer=std::less<T>>
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template Importer<T>>
        createImporter(
            GenericIStreamImporterSpec<infra::TopDownSinglePassIterationApp<Env>> const &spec
            , Reader &&reader
            , DataComparer &&dataComparer = DataComparer {}
        ) {
            class LocalI final : public infra::TopDownSinglePassIterationApp<Env>::template AbstractImporter<T> {
            private:
                GenericIStreamImporterSpec<infra::TopDownSinglePassIterationApp<Env>> spec_;
                Reader reader_;
                DataComparer dataComparer_;
                GenericIOStreamImporterExporterHelper::Queue<infra::TopDownSinglePassIterationApp<Env>,T,DataComparer> queue_;

                Env *env_;
            public:
                LocalI(
                    GenericIStreamImporterSpec<infra::TopDownSinglePassIterationApp<Env>> const &spec
                    , Reader &&reader
                    , DataComparer &&dataComparer
                ) :
                    spec_(spec)
                    , reader_(std::move(reader))
                    , dataComparer_(std::move(dataComparer))
                    , queue_()
                    , env_(nullptr)
                {}
                virtual void start(Env *env) override final {
                    env_ = env;
                    GenericIOStreamImporterExporterHelper::template startQueue<infra::TopDownSinglePassIterationApp<Env>,T,Reader,DataComparer>(
                        env_, queue_, spec_, reader_, dataComparer_
                    );
                }
                virtual std::tuple<bool, typename infra::TopDownSinglePassIterationApp<Env>::template Data<T>> 
                generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    auto ret = GenericIOStreamImporterExporterHelper::template stepQueue<infra::TopDownSinglePassIterationApp<Env>,T,Reader,DataComparer>(
                        env_, queue_, reader_
                    );
                    if (!ret) {
                        return {false, std::nullopt};
                    }
                    return {!(ret->timedData.finalFlag), std::move(ret)};
                }
            };
            return infra::TopDownSinglePassIterationApp<Env>::template importer<T>(new LocalI(spec, std::move(reader), std::move(dataComparer)));
        }
        
        template <class T, class Reader>
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template Importer<T>>
        createImporter(
            std::shared_ptr<std::istream> const &stream
            , Reader &&reader = Reader {}
        ) {
            class LocalI final : public infra::TopDownSinglePassIterationApp<Env>::template AbstractImporter<T> {
            private:
                std::shared_ptr<std::istream> stream_;
                Reader reader_;
                std::optional<std::tuple<typename infra::TopDownSinglePassIterationApp<Env>::TimePoint,T>> data_;
                
                Env *env_;
            public:
                LocalI(
                    std::shared_ptr<std::istream> const &stream
                    , Reader &&reader
                ) :
                    stream_(stream)
                    , reader_(std::move(reader))
                    , data_(std::nullopt)
                    , env_(nullptr)
                {}
                virtual void start(Env *env) override final {
                    env_ = env;
                    reader_.start(env_, *stream_);
                    data_ = reader_.readOne(env_, *stream_);
                }
                virtual std::tuple<bool, typename infra::TopDownSinglePassIterationApp<Env>::template Data<T>>
                generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    if (!data_) {
                        return {false, std::nullopt};
                    }
                    std::optional<std::tuple<typename infra::TopDownSinglePassIterationApp<Env>::TimePoint,T>> newData = reader_.readOne(env_, *stream_);
                    auto ret = typename infra::SinglePassIterationApp<Env>::template InnerData<T> {
                        env_
                        , {
                            std::get<0>(*data_)
                            , std::move(std::get<1>(*data_))
                            , !((bool) newData)
                        }
                    };
                    data_ = std::move(newData);
                    return {!(ret.timedData.finalFlag), {std::move(ret)}};
                }
            };
            return infra::TopDownSinglePassIterationApp<Env>::template importer<T>(new LocalI(stream, std::move(reader)));
        }

        template <class T, class Writer>
        static std::shared_ptr<typename infra::TopDownSinglePassIterationApp<Env>::template Exporter<T>>
        createExporter(std::ostream &os, Writer &&writer, bool separateThread=false) {
            class LocalE final : public infra::TopDownSinglePassIterationApp<Env>::template AbstractExporter<T> {
            private:
                std::ostream *os_;
                Writer writer_;
            public:
                LocalE(std::ostream &os, Writer &&writer) : os_(&os), writer_(std::move(writer)) {}
                virtual void start(Env *env) override final {
                    writer_.start(env, *os_);
                }
                virtual void handle(typename infra::TopDownSinglePassIterationApp<Env>::template InnerData<T> &&d) override final {
                    TM_INFRA_EXPORTER_TRACER(d.environment);
                    writer_.writeOne(d.environment, *os_, std::move(d.timedData));
                    os_->flush();
                }
            };
            return infra::TopDownSinglePassIterationApp<Env>::template exporter(new LocalE(os, std::move(writer)));
        }
    };

    template <class T, class SizeType=std::size_t>
    class SimpleSerializingWriter {
    public:
        template <class Env>
        static void start(Env *env, std::ostream &os) {}
        template <class Env, class TimePoint>
        static void writeOne(Env *env, std::ostream &os, infra::WithTime<T, TimePoint> &&data) {
            if constexpr (bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()) {
                std::string s = bytedata_utils::RunSerializer<T>::apply(data.value);
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            } else {
                std::string s = bytedata_utils::RunCBORSerializer<T>::apply(data.value);
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            }
        }
    };
    template <class T, class SizeType>
    class SimpleSerializingWriter<std::shared_ptr<const T>, SizeType> {
    public:
        template <class Env>
        static void start(Env *env, std::ostream &os) {
        }
        template <class Env, class TimePoint>
        static void writeOne(Env *env, std::ostream &os, infra::WithTime<std::shared_ptr<const T>, TimePoint> &&data) {
            if constexpr (bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()) {
                std::string s = bytedata_utils::RunSerializer<T>::apply(*(data.value));
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            } else {
                std::string s = bytedata_utils::RunCBORSerializer<T>::apply(*(data.value));
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            }
        }
    };
    template <class T, class TimeExtractor, class SizeType=std::size_t, std::size_t BufferSize=4*1024*1024>
    class SimpleDeserializingReader {
    private:
        TimeExtractor timeExtractor_;
        std::vector<GenericIOStreamImporterExporterHelper::BufferedReader<BufferSize>> bufferedReaders_;
        std::unordered_map<std::istream *, std::size_t> readerIdx_;
    public:
        SimpleDeserializingReader(TimeExtractor &&timeExtractor = TimeExtractor {}) : timeExtractor_(std::move(timeExtractor)), bufferedReaders_(), readerIdx_() {}
        template <class Env>
        void start(Env *env, std::istream &is) {
            bufferedReaders_.push_back({});
            readerIdx_.insert({&is, bufferedReaders_.size()-1});
        }
        template <class Env>
        std::optional<std::tuple<typename Env::TimePointType, T>> readOne(Env *env, std::istream &is) {
            auto idxIter = readerIdx_.find(&is);
            if (idxIter == readerIdx_.end()) {
                return std::nullopt;
            }
            auto &bufferedReader = bufferedReaders_[idxIter->second];
            auto sizeBuf = bufferedReader.read(is, sizeof(SizeType));
            if (sizeBuf.length() == 0) {
                return std::nullopt;
            }
            SizeType l;
            std::memcpy(&l, sizeBuf.data(), sizeof(SizeType));
            l = boost::endian::little_to_native<SizeType>(l);
            auto dataBuf = bufferedReader.read(is, l);
            if (dataBuf.length() == 0) {
                return std::nullopt;
            }
            if constexpr (bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()) {
                auto ret = bytedata_utils::RunDeserializer<T>::apply(dataBuf);
                if (!ret) {
                    return std::nullopt;
                }
                auto tp = timeExtractor_(*ret);
                return std::tuple<typename Env::TimePointType, T> {
                    tp
                    , std::move(*ret)
                };
            } else {
                auto ret = bytedata_utils::RunCBORDeserializer<T>::apply(dataBuf, 0);
                if (!ret) {
                    return std::nullopt;
                }
                if (std::get<1>(*ret) != l) {
                    return std::nullopt;
                }
                auto tp = timeExtractor_(std::get<0>(*ret));
                return std::tuple<typename Env::TimePointType, T> {
                    tp
                    , std::move(std::get<0>(*ret))
                };
            }
        }
    };

    template <class T, class TimeWriter, class SizeType=std::size_t>
    class SimpleSerializingWriterWithTime {
    private:
        TimeWriter timeWriter_;
    public:
        SimpleSerializingWriterWithTime(TimeWriter &&timeWriter = TimeWriter {}) : timeWriter_(std::move(timeWriter)) {}
        template <class Env>
        static void start(Env *env, std::ostream &os) {}
        template <class Env, class TimePoint>
        void writeOne(Env *env, std::ostream &os, infra::WithTime<T, TimePoint> &&data) {
            timeWriter_(env, os, data.timePoint);
            if constexpr (bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()) {
                std::string s = bytedata_utils::RunSerializer<T>::apply(data.value);
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            } else {
                std::string s = bytedata_utils::RunCBORSerializer<T>::apply(data.value);
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            }
        }
    };
    template <class T, class TimeWriter, class SizeType>
    class SimpleSerializingWriterWithTime<std::shared_ptr<const T>, TimeWriter, SizeType> {
    private:
        TimeWriter timeWriter_;
    public:
        SimpleSerializingWriterWithTime(TimeWriter &&timeWriter = TimeWriter {}) : timeWriter_(std::move(timeWriter)) {}
        template <class Env>
        static void start(Env *env, std::ostream &os) {}
        template <class Env, class TimePoint>
        void writeOne(Env *env, std::ostream &os, infra::WithTime<std::shared_ptr<const T>, TimePoint> &&data) {
            timeWriter_(env, os, data.timePoint);
            if constexpr (bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()) {
                std::string s = bytedata_utils::RunSerializer<T>::apply(*(data.value));
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            } else {
                std::string s = bytedata_utils::RunCBORSerializer<T>::apply(*(data.value));
                SizeType l = boost::endian::native_to_little<SizeType>(s.length());
                os.write(reinterpret_cast<char *>(&l), sizeof(SizeType));
                os.write(s.data(), s.length());
            }
        }
    };
    template <class T, class TimeReader, class SizeType=std::size_t, std::size_t BufferSize=4*1024*1024>
    class SimpleDeserializingReaderWithTime {
    private:
        TimeReader timeReader_;
        std::vector<GenericIOStreamImporterExporterHelper::BufferedReader<BufferSize>> bufferedReaders_;
        std::unordered_map<std::istream *, std::size_t> readerIdx_;
    public:
        SimpleDeserializingReaderWithTime(TimeReader &&timeReader = TimeReader {}) : timeReader_(std::move(timeReader)), bufferedReaders_(), readerIdx_() {}
        template <class Env>
        void start(Env *env, std::istream &is) {
            bufferedReaders_.push_back({});
            readerIdx_.insert({&is, bufferedReaders_.size()-1});
        }
        template <class Env>
        std::optional<std::tuple<typename Env::TimePointType, T>> readOne(Env *env, std::istream &is) {
            auto idxIter = readerIdx_.find(&is);
            if (idxIter == readerIdx_.end()) {
                return std::nullopt;
            }
            auto &bufferedReader = bufferedReaders_[idxIter->second];
            auto timeBuf = bufferedReader.read(is, TimeReader::TimeSize);
            if (timeBuf.length() == 0) {
                return std::nullopt;
            }
            std::optional<typename Env::TimePointType> tp = timeReader_(env, timeBuf);
            if (!tp) {
                return std::nullopt;
            }
            auto sizeBuf = bufferedReader.read(is, sizeof(SizeType));
            if (sizeBuf.length() == 0) {
                return std::nullopt;
            }
            SizeType l;
            std::memcpy(&l, sizeBuf.data(), sizeof(SizeType));
            l = boost::endian::little_to_native<SizeType>(l);
            auto dataBuf = bufferedReader.read(is, l);
            if (dataBuf.length() == 0) {
                return std::nullopt;
            }
            if constexpr (bytedata_utils::DirectlySerializableChecker<T>::IsDirectlySerializable()) {
                auto ret = bytedata_utils::RunDeserializer<T>::apply(dataBuf);
                if (!ret) {
                    return std::nullopt;
                }
                return std::tuple<typename Env::TimePointType, T> {
                    *tp
                    , std::move(*ret)
                };
            } else {
                auto ret = bytedata_utils::RunCBORDeserializer<T>::apply(dataBuf, 0);
                if (!ret) {
                    return std::nullopt;
                }
                if (std::get<1>(*ret) != l) {
                    return std::nullopt;
                }
                return std::tuple<typename Env::TimePointType, T> {
                    *tp
                    , std::move(std::get<0>(*ret))
                };
            }
        }
    };

    template <class Duration, class TimeType=int64_t>
    class SystemTimePointWriter {
    public:
        void operator()(void *, std::ostream &os, std::chrono::system_clock::time_point const &tp) {
            TimeType t = boost::endian::native_to_little<TimeType>((TimeType) infra::withtime_utils::sinceEpoch<Duration>(tp));
            os.write(reinterpret_cast<char *>(&t), sizeof(TimeType));
        }
    };
    template <class Duration, class TimeType=int64_t>
    class SystemTimePointReader {
    public:
        static constexpr std::size_t TimeSize = sizeof(TimeType);
        std::optional<std::chrono::system_clock::time_point> operator()(void *, std::string_view const &timeData) {
            TimeType t;
            std::memcpy(&t, timeData.data(), std::min(timeData.length(), sizeof(TimeType)));
            t = boost::endian::little_to_native<TimeType>(t);
            return infra::withtime_utils::epochDurationToTime<Duration>(t);
        }
    };

} } } }

#endif
