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

    template <class TimeComponent, bool LogThreadID=true, bool ForceActualTimeLogging=false>
    class TimeComponentEnhancedWithBoostTrivialLogging : public TimeComponent {
    private:
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::atomic<bool>,bool> firstTime_;
        std::conditional_t<(TimeComponent::CanBeActualTimeClock&&!ForceActualTimeLogging),std::mutex,bool> firstTimeMutex_;
        bool isActualClock_;
    public:
        TimeComponentEnhancedWithBoostTrivialLogging() : TimeComponent(), firstTime_(true), firstTimeMutex_(), isActualClock_(false) {
            if constexpr (!ForceActualTimeLogging) {
                if constexpr (!TimeComponent::CanBeActualTimeClock) {
                    boost::log::add_console_log(
                        std::cout
                        , boost::log::keywords::format = "%Message%"
                    );
                }
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
                            if (!isActualClock_) {
                                boost::log::add_console_log(
                                    std::cout
                                    , boost::log::keywords::format = "%Message%"
                                );
                            }
                            firstTime_ = false;
                        }
                    }
                    if (isActualClock_) {
                        TrivialBoostLoggingComponent::log(l, s);
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
    };

} } } }

#endif