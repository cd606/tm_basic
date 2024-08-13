#ifndef TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_SPD_LOGGING_COMPONENT_HPP_

#ifdef _MSC_VER
#include <winsock2.h>
#endif

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/pattern_formatter.h>

#include <tm_kit/infra/LogLevel.hpp>
#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/infra/PidUtil.hpp>

#include <tm_kit/basic/LoggingComponentBase.hpp>
#include <tm_kit/basic/TimePointAsString.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    class SpdLoggingComponent : public virtual dev::cd606::tm::basic::LoggingComponentBase {
    private:
        std::shared_ptr<spdlog::logger> logger_{nullptr};

    public:
        SpdLoggingComponent()
        {
            logger_ = spdlog::stdout_logger_mt("tm_basic_spdlog_ex");
            logger_->set_level(spdlog::level::trace);
            logger_->set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] [Thread %t] %v");
        }

        ~SpdLoggingComponent()
        {
            if (logger_) {
                spdlog::drop(logger_->name());
                logger_.reset();
            }
        }

        SpdLoggingComponent(SpdLoggingComponent const &) = default;
        SpdLoggingComponent(SpdLoggingComponent &&) = default;
        SpdLoggingComponent &operator=(SpdLoggingComponent const &) = default;
        SpdLoggingComponent &operator=(SpdLoggingComponent &&) = default;

        void setLevel(dev::cd606::tm::infra::LogLevel l)
        {
            switch (l) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->set_level(spdlog::level::trace);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->set_level(spdlog::level::debug);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->set_level(spdlog::level::info);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->set_level(spdlog::level::warn);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->set_level(spdlog::level::err);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->set_level(spdlog::level::critical);
                break;
            default:
                break;
            }
        }

        void flushOn(dev::cd606::tm::infra::LogLevel l)
        {
            switch (l) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->flush_on(spdlog::level::trace);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->flush_on(spdlog::level::debug);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->flush_on(spdlog::level::info);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->flush_on(spdlog::level::warn);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->flush_on(spdlog::level::err);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->flush_on(spdlog::level::critical);
                break;
            default:
                break;
            }
        }

        void setPattern(std::string const &pattern)
        {
            logger_->set_pattern(pattern);
        }

        template <typename... Args>
        void log(dev::cd606::tm::infra::LogLevel l, spdlog::fmt_lib::format_string<Args...> fmt, Args &&...args)
        {
            switch (l) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->trace(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->debug(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->info(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->warn(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->error(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->critical(fmt, std::forward<Args>(args)...);
                break;
            default:
                break;
            }
        }

        void log(dev::cd606::tm::infra::LogLevel l, std::string const &s)
        {
            switch (l) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->trace(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->debug(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->info(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->warn(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->error(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->critical(s);
                break;
            default:
                break;
            }
        }

        void setLogFilePrefix(std::string const &prefix, bool containTimePart = false, std::optional<uintmax_t> maxSize = std::nullopt)
        {
            if (logger_) {
                spdlog::drop(logger_->name());
                logger_.reset();
            }
            auto nowStr = dev::cd606::tm::infra::withtime_utils::genericLocalTimeString(std::chrono::system_clock::now()).substr(0, (containTimePart ? 19 : 10));
            std::replace(nowStr.begin(), nowStr.end(), '-', '_');
            std::replace(nowStr.begin(), nowStr.end(), ':', '_');
            std::replace(nowStr.begin(), nowStr.end(), ' ', '_');
            int64_t pid = dev::cd606::tm::infra::pid_util::getpid();
            if (maxSize) {
                logger_ = spdlog::rotating_logger_mt(
                    "tm_basic_spdlog", prefix + "." + nowStr + "." + std::to_string(pid) + ".log", static_cast<std::size_t>(*maxSize), std::numeric_limits<std::size_t>::max());
            } else {
                logger_ = spdlog::basic_logger_mt("tm_basic_spdlog", prefix + "." + nowStr + "." + std::to_string(pid) + ".log");
            }
            logger_->set_level(spdlog::level::trace);
            logger_->set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] [Thread %t] %v");
        }

        virtual void logThroughLoggingComponentBase(dev::cd606::tm::infra::LogLevel l, std::string const &s) override
        {
            this->log(l, s);
        }
    };
    using SpdLoggingComponentEx = SpdLoggingComponent;

    struct SpdLoggingAsyncParameter {
        bool blockOnFullQueue;
        std::size_t queueSize;
    };    

    inline constexpr auto defaultSpdLoggingAsyncParameter = SpdLoggingAsyncParameter{true, spdlog::details::default_async_q_size};

    template <class TimeComponent, bool LogThreadID = true, bool ForceActualTimeLogging = false, typename TimeZone = dev::cd606::tm::basic::time_zone_spec::Local>
    class TimeComponentEnhancedWithSpdLogging : public TimeComponent, public virtual dev::cd606::tm::basic::LoggingComponentBase {
    private:
        std::atomic_bool initialized_{false};
        std::mutex mtx_;
        std::shared_ptr<spdlog::logger> logger_;
        std::optional<std::string> logFilePrefix_;
        bool logFilePrefixContainTimePart_{false};
        std::optional<uintmax_t> maxSize_;
        bool isActualClock_{false};
        std::optional<dev::cd606::tm::infra::LogLevel> flushOn_;
        std::optional<dev::cd606::tm::infra::LogLevel> logLevel_;
        bool async_{false};
        std::optional<SpdLoggingAsyncParameter> asyncParam_;
        std::shared_ptr<spdlog::details::thread_pool> asyncThreadPool_;

    private:
        template <typename... Args>
        void doLog(dev::cd606::tm::infra::LogLevel l, spdlog::fmt_lib::format_string<Args...> fmt, Args &&...args)
        {
            switch (l) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->trace(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->debug(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->info(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->warn(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->error(fmt, std::forward<Args>(args)...);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->critical(fmt, std::forward<Args>(args)...);
                break;
            default:
                break;
            }
        }

        void doLog(dev::cd606::tm::infra::LogLevel l, std::string const &s)
        {
            switch (l) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->trace(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->debug(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->info(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->warn(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->error(s);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->critical(s);
                break;
            default:
                break;
            }
        }

        void doSetFlushOn()
        {
            if (!flushOn_) {
                logger_->flush_on(spdlog::level::err);
                return;
            }
            switch (*flushOn_) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->flush_on(spdlog::level::trace);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->flush_on(spdlog::level::debug);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->flush_on(spdlog::level::info);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->flush_on(spdlog::level::warn);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->flush_on(spdlog::level::err);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->flush_on(spdlog::level::critical);
                break;
            default:
                break;
            }
        }

        void doSetLogLevel()
        {
            if (!logLevel_) {
                logger_->set_level(spdlog::level::trace);
                return;
            }
            switch (*logLevel_) {
            case dev::cd606::tm::infra::LogLevel::Trace:
                logger_->set_level(spdlog::level::trace);
                break;
            case dev::cd606::tm::infra::LogLevel::Debug:
                logger_->set_level(spdlog::level::debug);
                break;
            case dev::cd606::tm::infra::LogLevel::Info:
                logger_->set_level(spdlog::level::info);
                break;
            case dev::cd606::tm::infra::LogLevel::Warning:
                logger_->set_level(spdlog::level::warn);
                break;
            case dev::cd606::tm::infra::LogLevel::Error:
                logger_->set_level(spdlog::level::err);
                break;
            case dev::cd606::tm::infra::LogLevel::Critical:
                logger_->set_level(spdlog::level::critical);
                break;
            default:
                break;
            }
        }

        class VirtualClockFormatter : public spdlog::custom_flag_formatter {
        private:
            TimeComponent *clock_;

        public:
            VirtualClockFormatter(TimeComponent *clock)
                : clock_(clock)
            {
            }

            void format(const spdlog::details::log_msg &, const std::tm &, spdlog::memory_buf_t &dest) override
            {
                std::string timeStr;
                if constexpr (std::is_same_v<typename TimeComponent::TimePointType, std::chrono::system_clock::time_point>) {
                    timeStr = dev::cd606::tm::basic::TimePointAsString<TimeZone>(clock_->now()).asString();
                } else {
                    timeStr = dev::cd606::tm::infra::withtime_utils::genericLocalTimeString(clock_->now());
                }
                dest.append(timeStr.data(), timeStr.data() + timeStr.size());
            }

            std::unique_ptr<spdlog::custom_flag_formatter> clone() const override
            {
                return spdlog::details::make_unique<VirtualClockFormatter>(clock_);
            }
        };

        void initializeLogger()
        {
            const std::string logName = "tm_basic_time_component_enhanced_spdlog";
            spdlog::sink_ptr logSink{nullptr};

            if (logFilePrefix_) {
                std::string nowStr;
                if constexpr (ForceActualTimeLogging) {
                    nowStr = dev::cd606::tm::infra::withtime_utils::genericLocalTimeString(std::chrono::system_clock::now()).substr(0, (logFilePrefixContainTimePart_ ? 19 : 10));
                } else {
                    if constexpr (std::is_same_v<typename TimeComponent::TimePointType, std::chrono::system_clock::time_point>) {
                        nowStr = dev::cd606::tm::basic::TimePointAsString<TimeZone>(this->TimeComponent::now()).asString().substr(0, (logFilePrefixContainTimePart_ ? 19 : 10));
                    } else {
                        nowStr = dev::cd606::tm::infra::withtime_utils::genericLocalTimeString(this->TimeComponent::now()).substr(0, (logFilePrefixContainTimePart_ ? 19 : 10));
                    }
                }
                std::replace(nowStr.begin(), nowStr.end(), '-', '_');
                std::replace(nowStr.begin(), nowStr.end(), ':', '_');
                std::replace(nowStr.begin(), nowStr.end(), ' ', '_');

                auto pid = dev::cd606::tm::infra::pid_util::getpid();
                auto logFileName = (*logFilePrefix_) + "." + nowStr + "." + std::to_string(pid) + ".log";
                if (maxSize_) {
                    logSink = std::dynamic_pointer_cast<spdlog::sinks::sink>(
                        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName, *maxSize_, std::numeric_limits<std::size_t>::max(), false, spdlog::file_event_handlers{}));
                } else {
                    logSink = std::dynamic_pointer_cast<spdlog::sinks::sink>(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileName, false, spdlog::file_event_handlers{}));
                }
            } else {
                logSink = std::dynamic_pointer_cast<spdlog::sinks::sink>(std::make_shared<spdlog::sinks::stdout_sink_mt>());
            }

            if (async_) {
                const SpdLoggingAsyncParameter *asyncParam{nullptr};
                if (!asyncParam_) {
                    asyncParam = &defaultSpdLoggingAsyncParameter;
                } else {
                    asyncParam = &(*asyncParam_);
                }
                asyncThreadPool_ = std::make_shared<spdlog::details::thread_pool>(asyncParam->queueSize, 1);
                logger_ = std::make_shared<spdlog::async_logger>(
                    logName, logSink, asyncThreadPool_, asyncParam->blockOnFullQueue ? spdlog::async_overflow_policy::block : spdlog::async_overflow_policy::overrun_oldest);
            } else {
                logger_ = std::make_shared<spdlog::logger>(logName, logSink);
            }

            if constexpr (ForceActualTimeLogging) {
                if constexpr (LogThreadID) {
                    logger_->set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] [Thread %t] %v");
                } else {
                    logger_->set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] %v");
                }
            } else {
                isActualClock_ = false;
                if constexpr (TimeComponent::CanBeActualTimeClock) {
                    isActualClock_ = this->TimeComponent::isActualClock();
                }
                if (isActualClock_) {
                    if constexpr (LogThreadID) {
                        logger_->set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] [Thread %t] %v");
                    } else {
                        logger_->set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%f] %v");
                    }
                } else {
                    auto formatter = std::make_unique<spdlog::pattern_formatter>();
                    formatter->add_flag<VirtualClockFormatter>('*', this).set_pattern("[%l] [%*] [Thread %t] %v");
                    logger_->set_formatter(std::move(formatter));
                }
            }

            doSetFlushOn();
            doSetLogLevel();
        }

    public:
        TimeComponentEnhancedWithSpdLogging()
        {
        }

        virtual ~TimeComponentEnhancedWithSpdLogging()
        {
            if (asyncThreadPool_) {
                asyncThreadPool_.reset();
            }
            if (logger_) {
                spdlog::drop(logger_->name());
                logger_.reset();
            }
        }

        void setLogFilePrefix(
            std::string const &prefix,
            bool containTimePart = false,
            std::optional<uintmax_t> maxSize = std::nullopt,
            bool async = false,
            std::optional<SpdLoggingAsyncParameter> asyncParam = std::nullopt)
        {
            logFilePrefix_ = prefix;
            logFilePrefixContainTimePart_ = containTimePart;
            maxSize_ = maxSize;
            async_ = async;
            asyncParam_ = asyncParam;
        }

        void flushOn(dev::cd606::tm::infra::LogLevel l)
        {
            flushOn_ = l;
        }

        void setLogLevel(dev::cd606::tm::infra::LogLevel l)
        {
            logLevel_ = l;
        }

        template <typename... Args>
        void log(dev::cd606::tm::infra::LogLevel l, spdlog::fmt_lib::format_string<Args...> fmt, Args &&...args)
        {
            if (!initialized_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mtx_);
                if (!initialized_.load(std::memory_order_relaxed)) {
                    initializeLogger();
                    initialized_.store(true, std::memory_order_release);
                }
            }
            doLog(l, fmt, std::forward<Args>(args)...);
        }

        void log(dev::cd606::tm::infra::LogLevel l, std::string const &s) {
            if (!initialized_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mtx_);
                if (!initialized_.load(std::memory_order_relaxed)) {
                    initializeLogger();
                    initialized_.store(true, std::memory_order_release);
                }
            }
            doLog(l, s);
        }

        virtual void logThroughLoggingComponentBase(dev::cd606::tm::infra::LogLevel l, std::string const &s) override
        {
            log(l, s);
        }
    };
    template <class TimeComponent, bool LogThreadID = true, bool ForceActualTimeLogging = false, typename TimeZone = dev::cd606::tm::basic::time_zone_spec::Local>
    using TimeComponentEnhancedWithSpdLoggingEx = TimeComponentEnhancedWithSpdLogging<TimeComponent, LogThreadID, ForceActualTimeLogging, TimeZone>;

} } } }

#endif
