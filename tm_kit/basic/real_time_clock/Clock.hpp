#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_HPP_

#include <chrono>
#include <thread>
#include <optional>
#include <ctime>
#include <string>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {

    //This code DELIBERATELY made settings immutable
    //The only possibly to change settings is through operator=,
    //and it is expected that users will only use that in the initialization
    //of an environment
    class Clock {
    public:
        struct ClockSettings {
            std::chrono::system_clock::time_point synchronizationPointActual;
            std::chrono::system_clock::time_point synchronizationPointVirtual;
            double speed; //1.0 means actual speed, 2.0 means clock goes at twice speed, and so on
        };
    private:
        //If settings is not given, the clock is the actual clock
        std::optional<ClockSettings> settings_; 

    public:
        Clock();
        Clock(ClockSettings const &);
        Clock(Clock const &) = default;
        Clock &operator=(Clock const &) = default;
        Clock(Clock &&) = default;
        Clock &operator=(Clock &&) = default;
        ~Clock() = default;

        /* An example of the JSON setting content is like:
         * {
         *  "clock_settings": {
         *    "actual" : {"time" : "10:00"}
         *    , "virtual" : {"date" : "2020-04-10", "time" : "09:30"}  
         *    , "speed" : "2.0"
         *  }
         * }
         * The date and time formats must be exact
         */
        static ClockSettings loadClockSettingsFromJSONFile(std::string const &fileName);
        static ClockSettings loadClockSettingsFromJSONString(std::string const &jsonContent);
        //this simulates the JSON parsing, but with actual time given as an int and virtual time given as yyyy-MM-ddTHH:mmss
        static ClockSettings clockSettingsWithStartPoint(int actualHHMM, std::string const &virtualStartPoint, double speed);
        //here the virtual time is still given in string, but the actual time is given as an "align minute",
        //how it works is: if actualTimeAlign is 5, then the next even-5-minute-point (on actual clock)
        //will be synchronized with the virtual start point. If the current call is exactly on an aligned minute
        //point, the actual alignment time will be the next align point.
        static ClockSettings clockSettingsWithStartPointCorrespondingToNextAlignment(int actualTimeAlignMinute, std::string const &virtualStartPoint, double speed=1.0);

        //these are virtual times
        std::chrono::system_clock::time_point now();
        uint64_t currentTimeMillis();
        void sleepFor(std::chrono::system_clock::duration const &virtualDuration);
        
        //converters
        std::chrono::system_clock::time_point actualTime(std::chrono::system_clock::time_point const &virtualT);
        std::chrono::system_clock::time_point virtualTime(std::chrono::system_clock::time_point const &actualT);
        std::chrono::system_clock::duration actualDuration(std::chrono::system_clock::duration const &virtualD);
        std::chrono::system_clock::duration virtualDuration(std::chrono::system_clock::duration const &actualD);
    };

} } } } }

#endif