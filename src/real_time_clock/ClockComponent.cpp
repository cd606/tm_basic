#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>
#include <iostream>

#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {
    class ClockComponentImpl {
    private:
#if BOOST_VERSION >= 108700    
        std::shared_ptr<boost::asio::io_context> service_;
#else
        std::shared_ptr<boost::asio::io_service> service_;
#endif
    public:
#if BOOST_VERSION >= 108700       
        ClockComponentImpl() : service_(std::make_shared<boost::asio::io_context>()) {
#else
        ClockComponentImpl() : service_(std::make_shared<boost::asio::io_service>()) {
#endif
            auto s = service_;
            std::thread th([s]() {
#if BOOST_VERSION >= 108700                
                auto work_guard = boost::asio::make_work_guard(*s);
#else
                boost::asio::io_service::work w(*s);
#endif
                s->run();
            });
            th.detach();
        }
        ~ClockComponentImpl() {
            service_->stop();
        }
        std::function<void()> scheduleOneTimeCallback(std::chrono::system_clock::time_point const &t, std::function<void()> callback) {
            auto timer = std::make_shared<boost::asio::system_timer>(*service_, t);
            timer->async_wait([timer,callback](boost::system::error_code const &err) {
                if (!err) {
                    callback();
                }
            });
            return [timer]() {
                timer->cancel();
            };
        }
        std::function<void()> scheduleRecurringCallback(std::chrono::system_clock::time_point const &start, std::chrono::system_clock::time_point const &end, std::chrono::system_clock::duration const &period, std::function<void()> callback) {
            auto timer = std::make_shared<boost::asio::system_timer>(*service_, start);
            timer->async_wait([this,timer,start,end,period,callback](boost::system::error_code const &err) {
                if (!err) {
                    std::chrono::system_clock::time_point t = start+period;
                    if (t <= end) {
                        scheduleRecurringCallback(t,end,period,callback);
                    }
                    callback();
                }
            });
            return [timer]() {
                timer->cancel();
            };
        }
        std::function<void()> scheduleVariableDurationRecurringCallback(std::chrono::system_clock::time_point const &start, std::chrono::system_clock::time_point const &end, std::function<std::chrono::system_clock::duration(std::chrono::system_clock::time_point const &)> periodCalc, std::function<void()> callback) {
            auto timer = std::make_shared<boost::asio::system_timer>(*service_, start);
            timer->async_wait([this,timer,start,end,periodCalc,callback](boost::system::error_code const &err) {
                if (!err) {
                    std::chrono::system_clock::time_point t = start+periodCalc(start);
                    if (t <= end) {
                        scheduleVariableDurationRecurringCallback(t,end,periodCalc,callback);
                    }
                    callback();
                }
            });
            return [timer]() {
                timer->cancel();
            };
        }
    };

    ClockComponent::ClockComponent() : Clock(), impl_(std::make_unique<ClockComponentImpl>()) {}
    ClockComponent::ClockComponent(Clock::ClockSettings const &s) : Clock(s), impl_(std::make_unique<ClockComponentImpl>()) {}
    ClockComponent::ClockComponent(ClockComponent &&) = default;
    ClockComponent &ClockComponent::operator=(ClockComponent &&) = default;
    ClockComponent::~ClockComponent() = default;

    std::function<void()> ClockComponent::createOneShotTimer(ClockComponent::TimePointType const &fireAtTime, std::function<void()> callback) {
        auto t = Clock::actualTime(fireAtTime);
        if (t >= std::chrono::system_clock::now()) {
            return impl_->scheduleOneTimeCallback(t, callback);
        } else {
            return {};
        }
    }
    std::function<void()> ClockComponent::createOneShotDurationTimer(ClockComponent::DurationType const &fireAfterDuration, std::function<void()> callback) {
        if (fireAfterDuration >= ClockComponent::DurationType(0)) {
            auto t = std::chrono::system_clock::now()+Clock::actualDuration(fireAfterDuration);
            return impl_->scheduleOneTimeCallback(t, callback);
        } else {
            return {};
        }
    }
    std::function<void()> ClockComponent::createRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, ClockComponent::DurationType const &period, std::function<void()> callback) {
        auto t1 = Clock::actualTime(firstFireAtTime);
        auto t2 = Clock::actualTime(lastFireAtTime);
        auto d = Clock::actualDuration(period);
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            return impl_->scheduleRecurringCallback(t1,t2,d,callback);
        } else {
            return {};
        }
    }
    std::function<void()> ClockComponent::createVariableDurationRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, std::function<ClockComponent::DurationType(ClockComponent::TimePointType const &)> periodCalc, std::function<void()> callback) {
        auto t1 = Clock::actualTime(firstFireAtTime);
        auto t2 = Clock::actualTime(lastFireAtTime);
        auto d = Clock::actualDuration(periodCalc(firstFireAtTime));
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            return impl_->scheduleVariableDurationRecurringCallback(t1,t2,[this,periodCalc](ClockComponent::TimePointType const &tp) {
                return this->Clock::actualDuration(periodCalc(tp));
            },callback);
        } else {
            return {};
        }
    }

    SimplifiedClockComponent::SimplifiedClockComponent() : Clock(), impl_(std::make_unique<ClockComponentImpl>()) {}
    SimplifiedClockComponent::SimplifiedClockComponent(SimplifiedClockComponent &&) = default;
    SimplifiedClockComponent &SimplifiedClockComponent::operator=(SimplifiedClockComponent &&) = default;
    SimplifiedClockComponent::~SimplifiedClockComponent() = default;

    std::function<void()> SimplifiedClockComponent::createOneShotTimer(ClockComponent::TimePointType const &fireAtTime, std::function<void()> callback) {
        if (fireAtTime >= std::chrono::system_clock::now()) {
            return impl_->scheduleOneTimeCallback(fireAtTime, callback);
        } else {
            return {};
        }
    }
    std::function<void()> SimplifiedClockComponent::createOneShotDurationTimer(ClockComponent::DurationType const &fireAfterDuration, std::function<void()> callback) {
        if (fireAfterDuration >= ClockComponent::DurationType(0)) {
            auto t = std::chrono::system_clock::now()+fireAfterDuration;
            return impl_->scheduleOneTimeCallback(t, callback);
        } else {
            return {};
        }
    }
    std::function<void()> SimplifiedClockComponent::createRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, ClockComponent::DurationType const &period, std::function<void()> callback) {
        auto t1 = firstFireAtTime;
        auto t2 = lastFireAtTime;
        auto d = period;
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            return impl_->scheduleRecurringCallback(t1,t2,d,callback);
        } else {
            return {};
        }
    }
    std::function<void()> SimplifiedClockComponent::createVariableDurationRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, std::function<ClockComponent::DurationType(ClockComponent::TimePointType const &)> periodCalc, std::function<void()> callback) {
        auto t1 = firstFireAtTime;
        auto t2 = lastFireAtTime;
        auto d = periodCalc(firstFireAtTime);
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            return impl_->scheduleVariableDurationRecurringCallback(t1,t2,periodCalc,callback);
        } else {
            return {};
        }
    }

} } } } }