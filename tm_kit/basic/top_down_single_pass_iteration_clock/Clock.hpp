#ifndef TM_KIT_BASIC_TOP_DOWN_SINGLE_PASS_ITERATION_CLOCK_CLOCK_HPP_
#define TM_KIT_BASIC_TOP_DOWN_SINGLE_PASS_ITERATION_CLOCK_CLOCK_HPP_

#include <tm_kit/single_pass_iteration_clock/Clock.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace top_down_single_pass_iteration_clock {
    
    template <class TimePoint, bool AllowLocalTimeOverride=true>
    using Clock = dev::cd606::tm::basic::single_pass_iteration_clock::Clock<TimePoint,AllowLocalTimeOverride>;
    
} } } } }

#endif