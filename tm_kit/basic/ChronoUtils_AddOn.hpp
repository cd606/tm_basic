#ifndef TM_KIT_BASIC_CHRONO_UTILS_ADD_ON_HPP_
#define TM_KIT_BASIC_CHRONO_UTILS_ADD_ON_HPP_

#include <tm_kit/infra/ChronoUtils.hpp>

//This is "add on" to the infra withtime_utils files, so the functions
//stay in the tm::infra namespace
namespace dev { namespace cd606 { namespace tm { namespace infra {
    namespace withtime_utils {
        extern std::chrono::system_clock::time_point parseZonedTime(int year, int month, int day, int hour, int minute, int second, int microseconds, std::string_view const &timeZoneName);
    }
} } } }

#endif