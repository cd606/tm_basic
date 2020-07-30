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

    template <class TimeComponent, bool LogThreadID=true>
    class TimeComponentEnhancedWithSpdLogging : public TimeComponent {
    private:
        class LocalInitializer {
        public:
            LocalInitializer() {
                spdlog::set_pattern("%v");
            }
        };
    public:
        TimeComponentEnhancedWithSpdLogging() : TimeComponent() {
            static LocalInitializer s_localInitializer; //Force running initializer
        }
        void log(infra::LogLevel l, std::string const &s) {
            std::ostringstream oss;
            oss << '[' << infra::logLevelToString(l) << "] ";
            oss << '[' << infra::withtime_utils::genericLocalTimeString(TimeComponent::now()) << "] ";
            if (LogThreadID) {
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
    };

} } } }

#endif