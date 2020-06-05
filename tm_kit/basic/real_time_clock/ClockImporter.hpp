#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_IMPORTER_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_IMPORTER_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeMonad.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent, Env>, int> = 0>
    class ClockImporter {
    public:
        using M = infra::RealTimeMonad<Env>;
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createOneShotClockImporter(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator) {
            class LocalI final : public M::template AbstractImporter<T> {
            private:
                typename Env::TimePointType tp_;
                std::function<T(typename Env::TimePointType const &)> generator_;
            public:
                LocalI(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator)
                    : tp_(tp), generator_(generator)
                {
                }
                //because we want to enforce the consistency of the generator
                //input and the data time point, we create the data by hand
                virtual void start(Env *env) override final {
                    env->createOneShotTimer(tp_, [this,env]() {
                        auto now = env->now();
                        T t = generator_(now);
                        this->publish(typename M::template InnerData<T> {
                            env, {now, std::move(t), true}
                        });
                    });
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
            class LocalI final : public M::template AbstractImporter<T> {
            private:
                typename Env::TimePointType start_, end_;
                typename Env::DurationType period_;
                std::function<T(typename Env::TimePointType const &)> generator_;
            public:
                LocalI(typename Env::TimePointType const &start, typename Env::TimePointType const &end, typename Env::DurationType const &period, std::function<T(typename Env::TimePointType const &)> generator)
                    : start_(start), end_(end), period_(period), generator_(generator)
                {
                }
                //because we want to enforce the consistency of the generator
                //input and the data time point, we create the data by hand
                virtual void start(Env *env) override final {
                    env->createRecurringTimer(start_, end_, period_, [this,env]() {
                        auto now = env->now();
                        T t = generator_(now);
                        this->publish(typename M::template InnerData<T> {
                            env, {now, std::move(t), now>=end_}
                        });
                    });
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
    };

} } } } }

#endif