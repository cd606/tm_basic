#ifndef TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_

#include <iostream>
#include <sstream>
#include <chrono>
#include <spdlog/spdlog.h>

#include <tm_kit/infra/LogLevel.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>

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
    public:
        TimeComponentEnhancedWithSpdLogging() : TimeComponent(), firstTime_(true), firstTimeMutex_(), isActualClock_(false) {
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
        void log(infra::LogLevel l, std::string const &s) {
            if constexpr (ForceActualTimeLogging) {
                SpdLoggingComponent::log(l, s);
            } else {
                if constexpr (TimeComponent::CanBeActualTimeClock) {
                    if (firstTime_) {
                        std::lock_guard<std::mutex> _(firstTimeMutex_);
                        if (firstTime_) {
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