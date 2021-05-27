#ifndef TM_KIT_BASIC_TOP_DOWN_SINGLE_PASS_ITERATION_CLOCK_CLOCK_COMPONENT_HPP_
#define TM_KIT_BASIC_TOP_DOWN_SINGLE_PASS_ITERATION_CLOCK_CLOCK_COMPONENT_HPP_

#include <tm_kit/basic/single_pass_iteration_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace top_down_single_pass_iteration_clock {
    
    template <class TimePoint, bool AllowLocalTimeOverride=false>
    using ClockComponent = dev::cd606::tm::basic::single_pass_iteration_clock::ClockComponent<TimePoint,AllowLocalTimeOverride>;

} } } } }

#endif