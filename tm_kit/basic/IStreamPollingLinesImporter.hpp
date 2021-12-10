#ifndef TM_KIT_BASIC_ISTREAM_POLLING_LINES_IMPORTER_HPP_
#define TM_KIT_BASIC_ISTREAM_POLLING_LINES_IMPORTER_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <istream>
#include <vector>
#include <memory>
#include <chrono>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class Env>
    class IStreamPollingLinesImporter {
    private:
        using M = infra::RealTimeApp<Env>;
    public:
        static auto createImporter(std::shared_ptr<std::istream> const &stream, std::chrono::system_clock::duration const &realPollingInterval)
            -> std::shared_ptr<typename M::template Importer<std::vector<std::string>>>
        {
            return M::template uniformSimpleImporter<std::vector<std::string>>(
                [stream,realPollingInterval](Env *env)
                    -> std::tuple<bool, typename M::template Data<std::vector<std::string>>>
                {
                    static std::size_t bufSize = 2048;
                    static std::unique_ptr<char[]> buf = std::make_unique<char[]>(bufSize);
                    static std::size_t bufEnd = 0;
                    static std::streampos pos = 0;
                    
                    std::this_thread::sleep_for(realPollingInterval);

                    if (stream->bad()) {
                        buf.reset();
                        return {false, std::nullopt};
                    }
                    stream->clear();
                    stream->seekg(pos);
                    while (true) {
                        stream->read(buf.get()+bufEnd, bufSize-bufEnd);
                        auto actualSize = stream->gcount();
                        pos += actualSize;
                        if (actualSize == 0) {
                            return {true, std::nullopt};
                        }
                        std::size_t idx = actualSize-1;
                        while (idx > bufEnd) {
                            if (buf[idx] == '\n') {
                                break;
                            }
                            --idx;
                        }
                        if (buf[idx] != '\n') {
                            if (actualSize == bufSize-bufEnd) {
                                auto oldBufSize = bufSize;
                                if (bufSize < 65536) {
                                    bufSize *= 2;
                                } else {
                                    bufSize += 65536;
                                }
                                auto newBuf = std::make_unique<char[]>(bufSize);
                                std::memcpy(newBuf.get(), buf.get(), oldBufSize);
                                buf = std::move(newBuf);
                                bufEnd = oldBufSize;
                            } else {
                                return {true, std::nullopt};
                            }
                        } else {
                            bufEnd += actualSize;
                            break;
                        }                       
                    }
                    char *p = buf.get();
                    char *end = p+bufEnd;
                    char *q = p;
                    std::vector<std::string> v;
                    while (q != end) {
                        if (*q == '\n') {
                            v.push_back(std::string(p, q));
                            p = q+1;
                            q = q+1;
                        } else {
                            ++q;
                        }
                    }
                    if (p == end) {
                        bufEnd = 0;
                    } else {
                        std::memmove(buf.get(), p, end-p);
                        bufEnd = end-p;
                    }
                    return {
                        true 
                        , typename M::template InnerData<std::vector<std::string>> {
                            env 
                            , {
                                env->now() 
                                , std::move(v)
                                , false
                            }
                        }
                    };
                }
                , infra::LiftParameters<typename M::TimePoint>().SuggestThreaded(true)
            );
        }
    };
} } } }

#endif