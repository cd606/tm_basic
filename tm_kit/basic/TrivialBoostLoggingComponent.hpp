#ifndef TM_KIT_BASIC_TRIVIAL_BOOST_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_TRIVIAL_BOOST_LOGGING_COMPONENT_HPP_

#include <iostream>
#include <sstream>
#include <chrono>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <tm_kit/infra/LogLevel.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    struct TrivialBoostLoggingComponent {
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
    };

    template <class TimeType>
    std::string localTimeStringForLogging(TimeType const &t) {
        std::ostringstream oss;
        oss << t;
        return oss.str();
    }
    template <>
    std::string localTimeStringForLogging<std::chrono::system_clock::time_point>(std::chrono::system_clock::time_point const &t) {
        return infra::withtime_utils::localTimeString(t);
    }

    template <class TimeComponent, bool LogThreadID=true>
    class TimeComponentEnhancedWithBoostTrivialLogging : public TimeComponent {
    private:
        class LocalInitializer {
        public:
            LocalInitializer() {
                boost::log::add_console_log(
                    std::cout
                    , boost::log::keywords::format = "%Message%"
                );
            }
        };
    public:
        TimeComponentEnhancedWithBoostTrivialLogging() : TimeComponent() {
            static LocalInitializer s_localInitializer; //Force running initializer
        }
        void log(infra::LogLevel l, std::string const &s) {
            std::ostringstream oss;
            oss << '[' << infra::logLevelToString(l) << "] ";
            oss << '[' << localTimeStringForLogging(TimeComponent::now()) << "] ";
            if (LogThreadID) {
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
    };

} } } }

#endif