#ifndef TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <tm_kit/infra/LogLevel.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/infra/PidUtil.hpp>

#include <tm_kit/basic/LoggingComponentBase.hpp>
#include <tm_kit/basic/TimePointAsString.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    struct SpdLoggingComponent : public virtual LoggingComponentBase {
        virtual ~SpdLoggingComponent() {}
        static void log(infra::LogLevel l, std::string const &s) {
            switch (l) {
                case infra::LogLevel::Trace:
                    spdlog::trace(s);
                    break;
                case infra::LogLevel::Debug:
                    spdlog::debug(s);
                    break;
                case infra::LogLevel::Info:
                    spdlog::info(s);
                    break;
                case infra::LogLevel::Warning:
                    spdlog::warn(s);
                    break;
                case infra::LogLevel::Error:
                    spdlog::error(s);
                    break;
                case infra::LogLevel::Critical:
                    spdlog::critical(s);
                    break;
                default:
                    break;
            }
        }
        virtual void logThroughLoggingComponentBase(infra::LogLevel l, std::string const &s) override {
            SpdLoggingComponent::log(l, s);
        }
    };

    template <class TimeComponent, bool LogThreadID=true, bool ForceActualTimeLogging=false, typename TimeZone=basic::time_zone_spec::Local>
    class TimeComponentEnhancedWithSpdLogging : public TimeComponent, public virtual LoggingComponentBase {
    private:
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::atomic<bool>,bool> firstTime_;
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::mutex,bool> firstTimeMutex_;
        bool isActualClock_;
        void initialSetup() {
	    spdlog::set_level(spdlog::level::trace);
            if constexpr (ForceActualTimeLogging) {
                if constexpr (LogThreadID) {
                    spdlog::set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] [Thread %t] %v");
                } else {
                    spdlog::set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] %v");
                }
            } else {
                if constexpr (!TimeComponent::CanBeActualTimeClock) {
                    spdlog::set_pattern("%v");
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
                if constexpr (std::is_same_v<typename TimeComponent::TimePointType, std::chrono::system_clock::time_point>) {
                    nowStr = dev::cd606::tm::basic::TimePointAsString<TimeZone>(TimeComponent::now()).asString().substr(0, (containTimePart?19:10));
                } else {
                    nowStr = infra::withtime_utils::genericLocalTimeString(TimeComponent::now()).substr(0, (containTimePart?19:10));
                }
            }
            std::replace(nowStr.begin(), nowStr.end(), '-', '_');
            std::replace(nowStr.begin(), nowStr.end(), ':', '_');
            std::replace(nowStr.begin(), nowStr.end(), ' ', '_');
            int64_t pid = infra::pid_util::getpid();
            if (maxSize) {
                auto logger = spdlog::rotating_logger_mt("logger", prefix+"."+nowStr+"."+std::to_string(pid)+".log", static_cast<std::size_t>(*maxSize), 1);
                logger->set_level(spdlog::level::trace);
                logger->flush_on(spdlog::level::trace); //always flush
                spdlog::set_default_logger(logger);
            } else {
                auto logger = spdlog::basic_logger_mt("logger", prefix+"."+nowStr+"."+std::to_string(pid)+".log");
                logger->set_level(spdlog::level::trace);
                logger->flush_on(spdlog::level::trace); //always flush
                spdlog::set_default_logger(logger);
            }
            prefixSet_ = true;
        }
        std::optional<std::string> logFilePrefix_;
        bool logFilePrefixContainTimePart_;
        std::optional<uintmax_t> maxSize_;
        bool prefixSet_;
    public:
        TimeComponentEnhancedWithSpdLogging() : TimeComponent(), firstTime_(true), firstTimeMutex_(), isActualClock_(false), logFilePrefix_(std::nullopt), logFilePrefixContainTimePart_(false), maxSize_(std::nullopt), prefixSet_(false) {
            initialSetup();
        }
        virtual ~TimeComponentEnhancedWithSpdLogging() {}
        void setLogFilePrefix(std::string const &prefix, bool containTimePart=false, std::optional<uintmax_t> maxSize=std::nullopt) {
            if constexpr (ForceActualTimeLogging) {
                doSetLogFilePrefix(prefix, containTimePart, maxSize);
                initialSetup();
            } else {
                logFilePrefix_ = prefix;
                logFilePrefixContainTimePart_ = containTimePart;
                maxSize_ = maxSize;
            }
        }
        void log(infra::LogLevel l, std::string const &s) {
            if constexpr (ForceActualTimeLogging) {
                SpdLoggingComponent::log(l, s);
            } else {
                if constexpr (TimeComponent::CanBeActualTimeClock) {
                    if (firstTime_) {
                        std::lock_guard<std::mutex> _(firstTimeMutex_);
                        if (firstTime_) {
                            if (logFilePrefix_) {
                                doSetLogFilePrefix(*logFilePrefix_, logFilePrefixContainTimePart_, maxSize_);
                            }
                            isActualClock_ = this->isActualClock();
                            if (isActualClock_) {
                                if constexpr (LogThreadID) {
                                    spdlog::set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] [Thread %t] %v");
                                } else {
                                    spdlog::set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] %v");
                                }
                            } else {
                                spdlog::set_pattern("%v");
                            }
                            firstTime_ = false;
                        }
                    }
                    if (isActualClock_) {
                        SpdLoggingComponent::log(l, s);
                        return;
                    }
                } else {
                    if (firstTime_) {
                        if (TimeComponent::now() != typename TimeComponent::TimePointType {}) {
                            if (logFilePrefix_) {
                                doSetLogFilePrefix(*logFilePrefix_, logFilePrefixContainTimePart_, maxSize_);
                                initialSetup();
                            }
                            firstTime_ = false;
                        }
                    }
                }
                std::ostringstream oss;
                oss << '[' << infra::logLevelToString(l) << "] ";                
                if constexpr (std::is_same_v<typename TimeComponent::TimePointType, std::chrono::system_clock::time_point>) {
                    oss << '[' << dev::cd606::tm::basic::TimePointAsString<TimeZone>(TimeComponent::now()).asString() << "] ";
                } else {
                    oss << '[' << infra::withtime_utils::genericLocalTimeString(TimeComponent::now()) << "] ";
                }

                if constexpr (LogThreadID) {
                    thread_local static auto threadId = std::this_thread::get_id();
                    oss << "[Thread " << threadId << "] ";
                }
                oss << s;
                switch (l) {
                    case infra::LogLevel::Trace:
                        spdlog::trace(oss.str());
                        break;
                    case infra::LogLevel::Debug:
                        spdlog::debug(oss.str());
                        break;
                    case infra::LogLevel::Info:
                        spdlog::info(oss.str());
                        break;
                    case infra::LogLevel::Warning:
                        spdlog::warn(oss.str());
                        break;
                    case infra::LogLevel::Error:
                        spdlog::error(oss.str());
                        break;
                    case infra::LogLevel::Critical:
                        spdlog::critical(oss.str());
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
