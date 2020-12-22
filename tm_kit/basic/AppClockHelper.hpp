#ifndef TM_KIT_BASIC_APP_CLOCK_HELPER_HPP_
#define TM_KIT_BASIC_APP_CLOCK_HELPER_HPP_

#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>

#include <tm_kit/basic/empty_clock/ClockImporter.hpp>
#include <tm_kit/basic/empty_clock/ClockOnOrderFacility.hpp>
#include <tm_kit/basic/real_time_clock/ClockImporter.hpp>
#include <tm_kit/basic/real_time_clock/ClockOnOrderFacility.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockImporter.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockOnOrderFacility.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class App>
    struct AppClockHelper {};

    template <class Env>
    struct AppClockHelper<infra::BasicWithTimeApp<Env>> {
        using Importer = empty_clock::ClockImporter<Env>;
        using Facility = empty_clock::ClockOnOrderFacility<Env>;
    };

    template <class Env>
    struct AppClockHelper<infra::RealTimeApp<Env>> {
        using Importer = real_time_clock::ClockImporter<Env>;
        using Facility = real_time_clock::ClockOnOrderFacility<Env>;
    };

    template <class Env>
    struct AppClockHelper<infra::SinglePassIterationApp<Env>> {
        using Importer = single_pass_iteration_clock::ClockImporter<Env>;   
        using Facility = single_pass_iteration_clock::ClockOnOrderFacility<Env>;   
    }; 
    
} } } }

#endif