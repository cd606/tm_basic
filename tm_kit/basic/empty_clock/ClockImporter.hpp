#ifndef TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_IMPORTER_HPP_
#define TM_KIT_BASIC_EMPTY_CLOCK_CLOCK_IMPORTER_HPP_

#include <type_traits>
#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/empty_clock/ClockComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace empty_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent<typename Env::TimePointType>, Env>, int> = 0>
    class ClockImporter {
    public:
        using M = infra::BasicWithTimeApp<Env>;
        template <class T>
        static std::shared_ptr<typename M::template Importer<T>> createOneShotClockImporter(typename Env::TimePointType const &tp, std::function<T(typename Env::TimePointType const &)> generator) {
            return M::template vacuousImporter<T>();
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
            return M::template vacuousImporter<T>();
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
            return M::template vacuousImporter<T>();
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