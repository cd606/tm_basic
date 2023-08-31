#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_IMPORTER_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_IMPORTER_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent, Env>, int> = 0>
    class ClockImporter {
    public:
        using M = infra::RealTimeApp<Env>;
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createOneShotClockImporter(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator) {
            class LocalI final : public M::template AbstractImporter<T>, public virtual infra::IControllableNode<Env>  {
            private:
                typename Env::TimePointType tp_;
                std::function<T(typename Env::TimePointType const &)> generator_;
                std::function<void()> cancellor_;
                std::recursive_mutex cancellorMutex_;
            public:
                LocalI(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator)
                    : tp_(tp), generator_(generator), cancellor_(), cancellorMutex_()
                {
                }
                //because we want to enforce the consistency of the generator
                //input and the data time point, we create the data by hand
                virtual void start(Env *env) override final {
                    std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                    cancellor_ = env->createOneShotTimer(tp_, [this,env]() {
                        TM_INFRA_IMPORTER_TRACER(env);
                        {
                            std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                            cancellor_ = {};
                        }
                        auto now = env->now();
                        T t = generator_(now);
                        this->publish(typename M::template InnerData<T> {
                            env, {now, std::move(t), true}
                        });
                    });
                }
                void control(Env */*env*/, std::string const &command, std::vector<std::string> const &params) override final {
                    if (command == "stop") {
                        std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                        if (cancellor_) {
                            cancellor_();
                        }
                    }
                }
            };
            return M::importer(new LocalI(tp, generator));
        }
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createOneShotClockConstImporter(typename Env::TimePointType const &tp, T const &value) {
            return createOneShotClockImporter<T>(tp, ConstGenerator<T,typename Env::TimePointType> {value});
        }
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createOneShotClockConstImporter(typename Env::TimePointType const &tp, T &&value) {
            return createOneShotClockImporter<T>(tp, ConstGenerator<T,typename Env::TimePointType> {std::move(value)});
        }

        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createRecurringClockImporter(typename Env::TimePointType start, typename Env::TimePointType end, typename Env::DurationType period, std::function<T(typename Env::TimePointType const &)> generator) {
            class LocalI final : public M::template AbstractImporter<T>, public virtual infra::IControllableNode<Env> {
            private:
                typename Env::TimePointType start_, end_;
                typename Env::DurationType period_;
                std::function<T(typename Env::TimePointType const &)> generator_;
                std::function<void()> cancellor_;
                std::recursive_mutex cancellorMutex_;
            public:
                LocalI(typename Env::TimePointType const &start, typename Env::TimePointType const &end, typename Env::DurationType const &period, std::function<T(typename Env::TimePointType const &)> generator)
                    : start_(start), end_(end), period_(period), generator_(generator), cancellor_(), cancellorMutex_()
                {
                }
                //because we want to enforce the consistency of the generator
                //input and the data time point, we create the data by hand
                virtual void start(Env *env) override final {
                    std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                    cancellor_ = env->createRecurringTimer(start_, end_, period_, [this,env]() {
                        TM_INFRA_IMPORTER_TRACER(env);
                        auto now = env->now();
                        T t = generator_(now);
                        this->publish(typename M::template InnerData<T> {
                            env, {now, std::move(t), now>=end_}
                        });
                    });
                }
                void control(Env */*env*/, std::string const &command, std::vector<std::string> const &params) override final {
                    if (command == "stop") {
                        std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                        if (cancellor_) {
                            cancellor_();
                        }
                    }
                }
            };
            return M::importer(new LocalI(start, end, period, generator));
        }

        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createRecurringClockConstImporter(typename Env::TimePointType start, typename Env::TimePointType end, typename Env::DurationType period, T const &value) {
            return createRecurringClockImporter<T>(start, end, period, ConstGenerator<T,typename Env::TimePointType> {value});
        }
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createRecurringClockConstImporter(typename Env::TimePointType start, typename Env::TimePointType end, typename Env::DurationType period, T &&value) {
            return createRecurringClockImporter<T>(start, end, period, ConstGenerator<T,typename Env::TimePointType> {std::move(value)});
        }

        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createVariableDurationRecurringClockImporter(typename Env::TimePointType start, typename Env::TimePointType end, std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc, std::function<T(typename Env::TimePointType const &)> generator) {
            class LocalI final : public M::template AbstractImporter<T>, public virtual infra::IControllableNode<Env> {
            private:
                typename Env::TimePointType start_, end_;
                std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc_;
                std::function<T(typename Env::TimePointType const &)> generator_;
                std::function<void()> cancellor_;
                std::recursive_mutex cancellorMutex_;
            public:
                LocalI(typename Env::TimePointType const &start, typename Env::TimePointType const &end, std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc, std::function<T(typename Env::TimePointType const &)> generator)
                    : start_(start), end_(end), periodCalc_(periodCalc), generator_(generator), cancellor_(), cancellorMutex_()
                {
                }
                //because we want to enforce the consistency of the generator
                //input and the data time point, we create the data by hand
                virtual void start(Env *env) override final {
                    std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                    cancellor_ = env->createVariableDurationRecurringTimer(start_, end_, periodCalc_, [this,env]() {
                        TM_INFRA_IMPORTER_TRACER(env);
                        auto now = env->now();
                        T t = generator_(now);
                        this->publish(typename M::template InnerData<T> {
                            env, {now, std::move(t), now>=end_}
                        });
                    });
                }
                void control(Env */*env*/, std::string const &command, std::vector<std::string> const &params) override final {
                    if (command == "stop") {
                        std::lock_guard<std::recursive_mutex> _(cancellorMutex_);
                        if (cancellor_) {
                            cancellor_();
                        }
                    }
                }
            };
            return M::importer(new LocalI(start, end, periodCalc, generator));
        }

        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createVariableDurationRecurringClockConstImporter(typename Env::TimePointType start, typename Env::TimePointType end, std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc, T const &value) {
            return createVariableDurationRecurringClockImporter<T>(start, end, periodCalc, ConstGenerator<T,typename Env::TimePointType> {value});
        }
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createVariableDurationRecurringClockConstImporter(typename Env::TimePointType start, typename Env::TimePointType end, std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc, T &&value) {
            return createVariableDurationRecurringClockImporter<T>(start, end, periodCalc, ConstGenerator<T,typename Env::TimePointType> {std::move(value)});
        }
    };

} } } } }

#endif