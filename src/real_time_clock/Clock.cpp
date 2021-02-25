#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <tm_kit/infra/ChronoUtils.hpp>
#include <tm_kit/basic/real_time_clock/Clock.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {
    Clock::Clock() : settings_(std::nullopt) {}
    Clock::Clock(Clock::ClockSettings const &s) : settings_(s) {}

    namespace {
        Clock::ClockSettings createClockSettings(boost::property_tree::ptree &tree) {
            std::string hhmmActual = tree.get<std::string>("clock_settings.actual.time");
            std::string yyyymmddVirtual = tree.get<std::string>("clock_settings.virtual.date");
            std::string hhmmVirtual = tree.get<std::string>("clock_settings.virtual.time");
            double speed = tree.get<double>("clock_settings.speed");

            Clock::ClockSettings ret;

            std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm *m = std::localtime(&t);
            std::vector<std::string> parts;
            boost::trim(hhmmActual);
            boost::split(parts, hhmmActual, boost::is_any_of(":"));
            m->tm_hour = boost::lexical_cast<int>(parts[0]);
            m->tm_min = (parts.size()>1)?boost::lexical_cast<int>(parts[1]):0;
            m->tm_sec = 0;
            t = std::mktime(m);
            ret.synchronizationPointActual = std::chrono::system_clock::from_time_t(t);

            parts.clear();
            boost::trim(yyyymmddVirtual);
            if (yyyymmddVirtual != "") {
                boost::split(parts, yyyymmddVirtual, boost::is_any_of("-"));
                m->tm_year = boost::lexical_cast<int>(parts[0])-1900;
                m->tm_mon = boost::lexical_cast<int>(parts[1])-1;
                m->tm_mday = boost::lexical_cast<int>(parts[2]);
                parts.clear();
            }
            boost::trim(hhmmVirtual);
            boost::split(parts, hhmmVirtual, boost::is_any_of(":"));
            m->tm_hour = boost::lexical_cast<int>(parts[0]);
            m->tm_min = (parts.size()>1)?boost::lexical_cast<int>(parts[1]):0;
            m->tm_sec = 0;
            m->tm_isdst = -1;
            t = std::mktime(m);
            ret.synchronizationPointVirtual = std::chrono::system_clock::from_time_t(t);

            ret.speed = speed;

            return ret;
        }
    }

    Clock::ClockSettings Clock::loadClockSettingsFromJSONFile(std::string const &fileName) {
        boost::property_tree::ptree tree;
        boost::property_tree::read_json(fileName, tree);
        return createClockSettings(tree);
    }
    Clock::ClockSettings Clock::loadClockSettingsFromJSONString(std::string const &jsonContent) {
        boost::property_tree::ptree tree;
        std::stringstream ss;
        ss << jsonContent;
        boost::property_tree::read_json(ss, tree);
        return createClockSettings(tree);
    }

    Clock::ClockSettings Clock::clockSettingsWithStartPoint(int actualHHMM, std::string const &virtualStartPoint, double speed) {
        Clock::ClockSettings ret;

        std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm *m = std::localtime(&t);
        m->tm_hour = actualHHMM/100;
        m->tm_min = actualHHMM%100;
        m->tm_sec = 0;
        t = std::mktime(m);
        ret.synchronizationPointActual = std::chrono::system_clock::from_time_t(t);

        ret.synchronizationPointVirtual = infra::withtime_utils::parseLocalTime(virtualStartPoint);
        ret.speed = speed;

        return ret;
    }
    Clock::ClockSettings Clock::clockSettingsWithStartPointCorrespondingToNextAlignment(int actualTimeAlignMinute, std::string const &virtualStartPoint, double speed) {
        Clock::ClockSettings ret;

        std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm *m = std::localtime(&t);
        int min = m->tm_min;
        if (min % actualTimeAlignMinute == 0) {
            min += actualTimeAlignMinute;
        } else {
            min += (min % actualTimeAlignMinute);
        }
        if (min >= 60) {
            m->tm_hour = m->tm_hour+1;
            m->tm_min = min%60;
            if (m->tm_hour >= 24) {
                t += 24*3600;
                m = std::localtime(&t);
                m->tm_hour = 0;
                m->tm_min = min%60;
            }
        } else {
            m->tm_min = min;
        }
        m->tm_sec = 0;
        t = std::mktime(m);
        ret.synchronizationPointActual = std::chrono::system_clock::from_time_t(t);

        ret.synchronizationPointVirtual = infra::withtime_utils::parseLocalTime(virtualStartPoint);
        ret.speed = speed;

        return ret;
    }

    namespace {
        std::chrono::system_clock::time_point actualToVirtual(Clock::ClockSettings const &s, std::chrono::system_clock::time_point const &t) {
            auto d = t-s.synchronizationPointActual;
            d *= s.speed;
            return s.synchronizationPointVirtual+d;
        }
        std::chrono::system_clock::time_point virtualToActual(Clock::ClockSettings const &s, std::chrono::system_clock::time_point const &t) {
            auto d = t-s.synchronizationPointVirtual;
            d /= s.speed;
            return s.synchronizationPointActual+d;
        }
    }

    std::chrono::system_clock::time_point Clock::now() {
        if (settings_) {
            return actualToVirtual(*settings_, std::chrono::system_clock::now());
        } else {
            return std::chrono::system_clock::now();
        }
    }

    uint64_t Clock::currentTimeMillis() {
        return infra::withtime_utils::sinceEpoch<std::chrono::milliseconds>(now());
    }

    void Clock::sleepFor(std::chrono::system_clock::duration const &virtualDuration) {
        std::this_thread::sleep_for(actualDuration(virtualDuration));
    }

    std::chrono::system_clock::time_point Clock::actualTime(std::chrono::system_clock::time_point const &virtualT) {
        if (settings_) {
            return virtualToActual(*settings_, virtualT);
        } else {
            return virtualT;
        }
    }
    std::chrono::system_clock::time_point Clock::virtualTime(std::chrono::system_clock::time_point const &actualT) {
        if (settings_) {
            return actualToVirtual(*settings_, actualT);
        } else {
            return actualT;
        }        
    }
    std::chrono::system_clock::duration Clock::actualDuration(std::chrono::system_clock::duration const &virtualD) {
        if (settings_) {
            return std::chrono::duration_cast<std::chrono::system_clock::duration>(virtualD/settings_->speed);
        } else {
            return virtualD;
        }          
    }
    std::chrono::system_clock::duration Clock::virtualDuration(std::chrono::system_clock::duration const &actualD) {
        if (settings_) {
            return std::chrono::duration_cast<std::chrono::system_clock::duration>(actualD*settings_->speed);
        } else {
            return actualD;
        } 
    }
} } } } }