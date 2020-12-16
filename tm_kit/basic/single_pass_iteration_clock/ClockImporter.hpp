#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_IMPORTER_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_IMPORTER_HPP_

#include <type_traits>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent<typename Env::TimePointType>, Env>, int> = 0>
    class ClockImporter {
    public:
        using M = infra::SinglePassIterationApp<Env>;
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createOneShotClockImporter(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator) {
            class LocalI final : public M::template AbstractImporterCore<T> {
            private:
                typename Env::TimePointType tp_;
                std::function<T(typename Env::TimePointType const &)> generator_;
                Env *env_;
            public:
                LocalI(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator)
                    : tp_(tp), generator_(generator), env_(nullptr)
                {
                }
                virtual void start(Env *env) override final {
                    env_ = env;
                }
                virtual typename M::template Data<T> generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    return { typename M::template InnerData<T> {
                        env_, {tp_, generator_(tp_), true}
                    } };
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
            class LocalI final : public M::template AbstractImporterCore<T> {
            private:
                typename Env::TimePointType start_, end_;
                typename Env::DurationType period_;
                typename Env::TimePointType current_;
                std::function<T(typename Env::TimePointType const &)> generator_;
                Env *env_;
            public:
                LocalI(typename Env::TimePointType const &start, typename Env::TimePointType const &end, typename Env::DurationType const &period, std::function<T(typename Env::TimePointType const &)> generator)
                    : start_(start), end_(end), period_(period), current_(start_), generator_(generator), env_(nullptr)
                {
                }
                virtual typename M::template Data<T> generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    typename M::template InnerData<T> ret = typename M::template InnerData<T> {
                        env_, {current_, generator_(current_), (current_+period_ > end_)}
                    };
                    if (current_ < end_) {
                        current_ += period_;
                    }
                    return ret;
                }
                virtual void start(Env *env) override final {
                    env_ = env;
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
            class LocalI final : public M::template AbstractImporterCore<T> {
            private:
                typename Env::TimePointType start_, end_;
                std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc_;
                typename Env::TimePointType current_;
                std::function<T(typename Env::TimePointType const &)> generator_;
                Env *env_;
            public:
                LocalI(typename Env::TimePointType const &start, typename Env::TimePointType const &end, std::function<typename Env::DurationType(typename Env::TimePointType const &)> periodCalc, std::function<T(typename Env::TimePointType const &)> generator)
                    : start_(start), end_(end), periodCalc_(periodCalc), current_(start_), generator_(generator), env_(nullptr)
                {
                }
                virtual typename M::template Data<T> generate(T const *notUsed=nullptr) override final {
                    TM_INFRA_IMPORTER_TRACER(env_);
                    auto period = periodCalc_(current_);
                    typename M::template InnerData<T> ret = typename M::template InnerData<T> {
                        env_, {current_, generator_(current_), (current_+period > end_)}
                    };
                    if (current_ < end_) {
                        current_ += period;
                    }
                    return ret;
                }
                virtual void start(Env *env) override final {
                    env_ = env;
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