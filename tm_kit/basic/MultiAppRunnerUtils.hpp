#ifndef TM_KIT_BASIC_MULTI_APP_RUNNER_UTILS_HPP_
#define TM_KIT_BASIC_MULTI_APP_RUNNER_UTILS_HPP_

#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/infra/TupleVariantHelper.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    class MultiAppRunnerUtilComponents {
    private:
        template <class FirstR, class... RemainingRs>
        static void get_steppers_internal_(FirstR &firstR, RemainingRs &... remainingRs, std::vector<std::function<bool()>> &output) {
            output.push_back(
                firstR.finalizeForInterleaving()
            );
            if constexpr (sizeof...(RemainingRs) == 0) {
                return;
            } else {
                get_steppers_internal_<RemainingRs...>(remainingRs..., output);
            }
        }
        template <class... Rs>
        static std::vector<std::function<bool()>> get_steppers(Rs &... rs) {
            if constexpr (sizeof...(Rs) == 0) {
                return {};
            } else {
                std::vector<std::function<bool()>> ret;
                get_steppers_internal_<Rs...>(rs..., ret);
                return ret;
            }
        }
    public:
        template <class... Rs>
        static void run(Rs &... rs) {
            auto steppers = get_steppers(rs...);
            while (true) {
                bool running = false;
                for (auto const &f : steppers) {
                    if (f()) {
                        running = true;
                    }
                }
                if (!running) {
                    break;
                }
            }
        }
    };

} } } }

#endif