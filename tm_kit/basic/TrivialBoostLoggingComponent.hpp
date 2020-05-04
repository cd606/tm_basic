#ifndef TM_KIT_BASIC_TRIVIAL_BOOST_LOGGING_COMPONENT_HPP_
#define TM_KIT_BASIC_TRIVIAL_BOOST_LOGGING_COMPONENT_HPP_

#include <iostream>
#include <chrono>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes.hpp>

#include <tm_kit/infra/LogLevel.hpp>

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

} } } }

#endif