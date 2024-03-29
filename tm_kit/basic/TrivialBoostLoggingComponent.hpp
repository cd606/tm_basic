#ifndef TM_KIT_BASIC_TRIVIAL_BOOST_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_TRIVIAL_BOOST_LOGGING_COMPONENT_HPP_

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <tm_kit/infra/LogLevel.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/infra/PidUtil.hpp>

#include <tm_kit/basic/LoggingComponentBase.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    struct TrivialBoostLoggingComponent : public virtual LoggingComponentBase {
        virtual ~TrivialBoostLoggingComponent() {}
        static void log(infra::LogLevel l, std::string const &s) {
            switch (l) {
                case infra::LogLevel::Trace:
                    BOOST_LOG_TRIVIAL(trace) << s;
                    break;
                case infra::LogLevel::Debug:
                    BOOST_LOG_TRIVIAL(debug) << s;
                    break;
                case infra::LogLevel::Info:
                    BOOST_LOG_TRIVIAL(info) << s;
                    break;
                case infra::LogLevel::Warning:
                    BOOST_LOG_TRIVIAL(warning) << s;
                    break;
                case infra::LogLevel::Error:
                    BOOST_LOG_TRIVIAL(error) << s;
                    break;
                case infra::LogLevel::Critical:
                    BOOST_LOG_TRIVIAL(fatal) << s;
                    break;
                default:
                    break;
            }
        }
        virtual void logThroughLoggingComponentBase(infra::LogLevel l, std::string const &s) override {
            TrivialBoostLoggingComponent::log(l, s);
        }
    };

    template <class TimeComponent, bool LogThreadID=true, bool ForceActualTimeLogging=false>
    class TimeComponentEnhancedWithBoostTrivialLogging : public TimeComponent, public virtual LoggingComponentBase {
    private:
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::atomic<bool>,bool> firstTime_;
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::mutex,bool> firstTimeMutex_;
        bool isActualClock_;
        boost::shared_ptr<boost::log::sinks::sink> originalSink_;
        void initialSetup() {
            boost::log::add_common_attributes();
            boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
            if constexpr (!ForceActualTimeLogging) {
                if constexpr (!TimeComponent::CanBeActualTimeClock) {
                    originalSink_ = boost::log::add_console_log(
                        std::cout
                        , boost::log::keywords::format = "%Message%"
                    );
                }
            }
        }
        void doSetLogFilePrefix(std::string const &prefix, bool containTimePart, std::optional<uintmax_t> maxSize) {
            if (prefixSet_) {
                return;
            }
            std::string nowStr;
            if constexpr (ForceActualTimeLogging) {
                nowStr = infra::withtime_utils::genericLocalTimeString(std::chrono::system_clock::now()).substr(0, (containTimePart?19:10));
            } else {
                nowStr = infra::withtime_utils::genericLocalTimeString(TimeComponent::now()).substr(0, (containTimePart?19:10));
            }
            std::replace(nowStr.begin(), nowStr.end(), '-', '_');
            std::replace(nowStr.begin(), nowStr.end(), ':', '_');
            std::replace(nowStr.begin(), nowStr.end(), ' ', '_');
            int64_t pid = infra::pid_util::getpid();
            
            if (originalSink_) {
                boost::log::core::get()->remove_sink(originalSink_);
            }
            if constexpr (ForceActualTimeLogging) {
                if constexpr (LogThreadID) {
                    boost::log::add_file_log(
                        prefix+"."+nowStr+"."+std::to_string(pid)+".log"
                        , boost::log::keywords::format = "[%Severity%] [%TimeStamp%] [Thread %ThreadID%] %Message%"
                        , boost::log::keywords::open_mode = std::ios_base::app
                        , boost::log::keywords::auto_flush = true
                        , boost::log::keywords::max_size = (maxSize?*maxSize:std::numeric_limits<uintmax_t>::max())
                    );
                } else {
                    boost::log::add_file_log(
                        prefix+"."+nowStr+"."+std::to_string(pid)+".log"
                        , boost::log::keywords::format = "[%Severity%] [%TimeStamp%] %Message%"
                        , boost::log::keywords::open_mode = std::ios_base::app
                        , boost::log::keywords::auto_flush = true
                        , boost::log::keywords::max_size = (maxSize?*maxSize:std::numeric_limits<uintmax_t>::max())
                    );
                }
            } else {
                if constexpr (!TimeComponent::CanBeActualTimeClock) {
                    boost::log::add_file_log(
                        prefix+"."+nowStr+"."+std::to_string(pid)+".log"
                        , boost::log::keywords::format = "%Message%"
                        , boost::log::keywords::open_mode = std::ios_base::app
                        , boost::log::keywords::auto_flush = true
                        , boost::log::keywords::max_size = (maxSize?*maxSize:std::numeric_limits<uintmax_t>::max())
                    );
                } else {
                    if (isActualClock_) {
                        if constexpr (LogThreadID) {
                            boost::log::add_file_log(
                                prefix+"."+nowStr+"."+std::to_string(pid)+".log"
                                , boost::log::keywords::format = "[%Severity%] [%TimeStamp%] [Thread %ThreadID%] %Message%"
                                , boost::log::keywords::open_mode = std::ios_base::app
                                , boost::log::keywords::auto_flush = true
                                , boost::log::keywords::max_size = (maxSize?*maxSize:std::numeric_limits<uintmax_t>::max())
                            );
                        } else {
                            boost::log::add_file_log(
                                prefix+"."+nowStr+"."+std::to_string(pid)+".log"
                                , boost::log::keywords::format = "[%Severity%] [%TimeStamp%] %Message%"
                                , boost::log::keywords::open_mode = std::ios_base::app
                                , boost::log::keywords::auto_flush = true
                                , boost::log::keywords::max_size = (maxSize?*maxSize:std::numeric_limits<uintmax_t>::max())
                            );
                        }
                    } else {
                        boost::log::add_file_log(
                            prefix+"."+nowStr+"."+std::to_string(pid)+".log"
                            , boost::log::keywords::format = "%Message%"
                            , boost::log::keywords::open_mode = std::ios_base::app
                            , boost::log::keywords::auto_flush = true
                            , boost::log::keywords::max_size = (maxSize?*maxSize:std::numeric_limits<uintmax_t>::max())
                        );
                    }
                }
            }
            prefixSet_ = true;
        }
        std::optional<std::string> logFilePrefix_;
        bool logFilePrefixContainTimePart_;
        std::optional<uintmax_t> maxSize_;
        bool prefixSet_;
    public:
        TimeComponentEnhancedWithBoostTrivialLogging() : TimeComponent(), firstTime_(true), firstTimeMutex_(), isActualClock_(false), originalSink_(), logFilePrefix_(std::nullopt), logFilePrefixContainTimePart_(false), maxSize_(std::nullopt), prefixSet_(false) {
            initialSetup();
        }
        virtual ~TimeComponentEnhancedWithBoostTrivialLogging() {}
        void setLogFilePrefix(std::string const &prefix, bool containTimePart=false, std::optional<uintmax_t> maxSize=std::nullopt) {
            if constexpr (ForceActualTimeLogging) {
                doSetLogFilePrefix(prefix, containTimePart, maxSize);
            } else {
                logFilePrefix_ = prefix;
                logFilePrefixContainTimePart_ = containTimePart;
                maxSize_ = maxSize;
            }
        }
        void log(infra::LogLevel l, std::string const &s) {
            if constexpr (ForceActualTimeLogging) {
                TrivialBoostLoggingComponent::log(l, s);
            } else {
                if constexpr (TimeComponent::CanBeActualTimeClock) {
                    if (firstTime_) {
                        std::lock_guard<std::mutex> _(firstTimeMutex_);
                        if (firstTime_) {
                            isActualClock_ = this->isActualClock();
                            if (logFilePrefix_) {
                                doSetLogFilePrefix(*logFilePrefix_, logFilePrefixContainTimePart_, maxSize_);
                            } else {
                                if (!isActualClock_) {
                                    boost::log::add_console_log(
                                        std::cout
                                        , boost::log::keywords::format = "%Message%"
                                    );
                                }
                            }
                            firstTime_ = false;
                        }
                    }
                    if (isActualClock_) {
                        TrivialBoostLoggingComponent::log(l, s);
                        return;
                    }
                } else {
                    if (firstTime_) {
                        if (TimeComponent::now() != typename TimeComponent::TimePointType {}) {
                            if (logFilePrefix_) {
                                doSetLogFilePrefix(*logFilePrefix_, logFilePrefixContainTimePart_, maxSize_);
                            }
                            firstTime_ = false;
                        }
                    }
                }
                std::ostringstream oss;
                oss << '[' << infra::logLevelToString(l) << "] ";
                oss << '[' << infra::withtime_utils::genericLocalTimeString(TimeComponent::now()) << "] ";
                if constexpr (LogThreadID) {
                    oss << "[Thread " << std::this_thread::get_id() << "] ";
                }
                oss << s;
                switch (l) {
                    case infra::LogLevel::Trace:
                        BOOST_LOG_TRIVIAL(trace) << oss.str();
                        break;
                    case infra::LogLevel::Debug:
                        BOOST_LOG_TRIVIAL(debug) << oss.str();
                        break;
                    case infra::LogLevel::Info:
                        BOOST_LOG_TRIVIAL(info) << oss.str();
                        break;
                    case infra::LogLevel::Warning:
                        BOOST_LOG_TRIVIAL(warning) << oss.str();
                        break;
                    case infra::LogLevel::Error:
                        BOOST_LOG_TRIVIAL(error) << oss.str();
                        break;
                    case infra::LogLevel::Critical:
                        BOOST_LOG_TRIVIAL(fatal) << oss.str();
                        break;
                    default:
                        break;
                }
            }
        }
        virtual void logThroughLoggingComponentBase(infra::LogLevel l, std::string const &s) override {
            log(l, s);
        }
    };

} } } }

#endif
