#ifndef TM_KIT_BASIC_LOGGING_COMPONENT_BASE_HPP_
#define TM_KIT_BASIC_LOGGING_COMPONENT_BASE_HPP_

#include <tm_kit/infra/LogLevel.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    class LoggingComponentBase {
    public:
        virtual ~LoggingComponentBase() {}
        virtual void logThroughLoggingComponentBase(infra::LogLevel, std::string const &) = 0;
    };
} } } }

#endif