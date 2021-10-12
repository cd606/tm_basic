#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>
#include <iostream>

#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {
    class ClockComponentImpl {
    private:
        std::shared_ptr<boost::asio::io_service> service_;
    public:
        ClockComponentImpl() : service_(std::make_shared<boost::asio::io_service>()) {
            auto s = service_;
            std::thread th([s]() {
                boost::asio::io_service::work w(*s);
                s->run();
            });
            th.detach();
        }
        ~ClockComponentImpl() {
            service_->stop();
        }
        void scheduleOneTimeCallback(std::chrono::system_clock::time_point const &t, std::function<void()> callback) {
            auto timer = std::make_shared<boost::asio::system_timer>(*service_, t);
            timer->async_wait([timer,callback](boost::system::error_code const &err) {
                if (!err) {
                    callback();
                }
            });
        }
        void scheduleRecurringCallback(std::chrono::system_clock::time_point const &start, std::chrono::system_clock::time_point const &end, std::chrono::system_clock::duration const &period, std::function<void()> callback) {
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
        }
        void scheduleVariableDurationRecurringCallback(std::chrono::system_clock::time_point const &start, std::chrono::system_clock::time_point const &end, std::function<std::chrono::system_clock::duration(std::chrono::system_clock::time_point const &)> periodCalc, std::function<void()> callback) {
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
        }
    };

    ClockComponent::ClockComponent() : Clock(), impl_(std::make_unique<ClockComponentImpl>()) {}
    ClockComponent::ClockComponent(Clock::ClockSettings const &s) : Clock(s), impl_(std::make_unique<ClockComponentImpl>()) {}
    ClockComponent::ClockComponent(ClockComponent &&) = default;
    ClockComponent &ClockComponent::operator=(ClockComponent &&) = default;
    ClockComponent::~ClockComponent() = default;

    void ClockComponent::createOneShotTimer(ClockComponent::TimePointType const &fireAtTime, std::function<void()> callback) {
        auto t = Clock::actualTime(fireAtTime);
        if (t >= std::chrono::system_clock::now()) {
            impl_->scheduleOneTimeCallback(t, callback);
        }
    }
    void ClockComponent::createOneShotDurationTimer(ClockComponent::DurationType const &fireAfterDuration, std::function<void()> callback) {
        if (fireAfterDuration >= ClockComponent::DurationType(0)) {
            auto t = std::chrono::system_clock::now()+Clock::actualDuration(fireAfterDuration);
            impl_->scheduleOneTimeCallback(t, callback);
        }
    }
    void ClockComponent::createRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, ClockComponent::DurationType const &period, std::function<void()> callback) {
        auto t1 = Clock::actualTime(firstFireAtTime);
        auto t2 = Clock::actualTime(lastFireAtTime);
        auto d = Clock::actualDuration(period);
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            impl_->scheduleRecurringCallback(t1,t2,d,callback);
        }
    }
    void ClockComponent::createVariableDurationRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, std::function<ClockComponent::DurationType(ClockComponent::TimePointType const &)> periodCalc, std::function<void()> callback) {
        auto t1 = Clock::actualTime(firstFireAtTime);
        auto t2 = Clock::actualTime(lastFireAtTime);
        auto d = Clock::actualDuration(periodCalc(firstFireAtTime));
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            impl_->scheduleVariableDurationRecurringCallback(t1,t2,[this,periodCalc](ClockComponent::TimePointType const &tp) {
                return this->Clock::actualDuration(periodCalc(tp));
            },callback);
        }
    }

    SimplifiedClockComponent::SimplifiedClockComponent() : Clock(), impl_(std::make_unique<ClockComponentImpl>()) {}
    SimplifiedClockComponent::SimplifiedClockComponent(SimplifiedClockComponent &&) = default;
    SimplifiedClockComponent &SimplifiedClockComponent::operator=(SimplifiedClockComponent &&) = default;
    SimplifiedClockComponent::~SimplifiedClockComponent() = default;

    void SimplifiedClockComponent::createOneShotTimer(ClockComponent::TimePointType const &fireAtTime, std::function<void()> callback) {
        if (fireAtTime >= std::chrono::system_clock::now()) {
            impl_->scheduleOneTimeCallback(fireAtTime, callback);
        }
    }
    void SimplifiedClockComponent::createOneShotDurationTimer(ClockComponent::DurationType const &fireAfterDuration, std::function<void()> callback) {
        if (fireAfterDuration >= ClockComponent::DurationType(0)) {
            auto t = std::chrono::system_clock::now()+fireAfterDuration;
            impl_->scheduleOneTimeCallback(t, callback);
        }
    }
    void SimplifiedClockComponent::createRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, ClockComponent::DurationType const &period, std::function<void()> callback) {
        auto t1 = firstFireAtTime;
        auto t2 = lastFireAtTime;
        auto d = period;
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            impl_->scheduleRecurringCallback(t1,t2,d,callback);
        }
    }
    void SimplifiedClockComponent::createVariableDurationRecurringTimer(ClockComponent::TimePointType const &firstFireAtTime, ClockComponent::TimePointType const &lastFireAtTime, std::function<ClockComponent::DurationType(ClockComponent::TimePointType const &)> periodCalc, std::function<void()> callback) {
        auto t1 = firstFireAtTime;
        auto t2 = lastFireAtTime;
        auto d = periodCalc(firstFireAtTime);
        auto n = std::chrono::system_clock::now();
        while (t1 < n) {
            t1 += d;
        }
        if (t1 <= t2) {
            impl_->scheduleVariableDurationRecurringCallback(t1,t2,periodCalc,callback);
        }
    }

} } } } }