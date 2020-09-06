#ifndef TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_

#include <iostream>
#include <sstream>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <tm_kit/infra/LogLevel.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>

#include <tm_kit/basic/PidUtil.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    struct SpdLoggingComponent {
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
    };

    template <class TimeComponent, bool LogThreadID=true, bool ForceActualTimeLogging=false>
    class TimeComponentEnhancedWithSpdLogging : public TimeComponent {
    private:
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::atomic<bool>,bool> firstTime_;
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::mutex,bool> firstTimeMutex_;
        bool isActualClock_;
        void initialSetup() {
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
        void doSetLogFilePrefix(std::string const &prefix, bool containTimePart) {
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
            int64_t pid = pid_util::getpid();
            auto logger = spdlog::basic_logger_mt("logger", prefix+"."+nowStr+"."+std::to_string(pid)+".log");
            logger->flush_on(spdlog::level::trace); //always flush
            spdlog::set_default_logger(logger);
            prefixSet_ = true;
        }
        std::optional<std::string> logFilePrefix_;
        bool logFilePrefixContainTimePart_;
        bool prefixSet_;
    public:
        TimeComponentEnhancedWithSpdLogging() : TimeComponent(), firstTime_(true), firstTimeMutex_(), isActualClock_(false), logFilePrefix_(std::nullopt), logFilePrefixContainTimePart_(false), prefixSet_(false) {
            initialSetup();
        }
        void setLogFilePrefix(std::string const &prefix, bool containTimePart=false) {
            if constexpr (ForceActualTimeLogging) {
                doSetLogFilePrefix(prefix, containTimePart);
                initialSetup();
            } else {
                logFilePrefix_ = prefix;
                logFilePrefixContainTimePart_ = containTimePart;
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
                                doSetLogFilePrefix(*logFilePrefix_, logFilePrefixContainTimePart_);
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
                                doSetLogFilePrefix(*logFilePrefix_, logFilePrefixContainTimePart_);
                                initialSetup();
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
    };

} } } }

#endif