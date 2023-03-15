#ifndef TM_KIT_BASIC_TIMESTAMPED_COLLECTION_IMPORTER_HPP_
#define TM_KIT_BASIC_TIMESTAMPED_COLLECTION_IMPORTER_HPP_

#include <tm_kit/infra/BasicWithTimeApp.hpp>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TopDownSinglePassIterationApp.hpp>
#include <tm_kit/infra/WithTimeData.hpp>
#include <tm_kit/infra/GenericLift.hpp>
#include <tm_kit/infra/AppClassifier.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class M>
    class TimestampedCollectionImporterFactory {
    private:
        template <class Collection, class TimeExtractorF>
        class ImporterF {
        private:
            Collection data_;
            TimeExtractorF timeExtractorF_;
            typename Collection::iterator iter;
        public:
            ImporterF(Collection &&data, TimeExtractorF &&timeExtractorF)
                : data_(std::move(data)), timeExtractorF_(std::move(timeExtractorF)), iter(data_.begin())
            {}
            std::tuple<bool, typename M::template Data<typename Collection::value_type>> operator()(typename M::EnvironmentType *env) {
                if (iter == data_.end()) {
                    return {false, std::nullopt};
                }
                auto t = timeExtractorF_(*iter);
                if constexpr (infra::AppClassifier<M>::TheClassification == infra::AppClassification::RealTime) {
                    auto n = env->resolveTime();
                    if (t >= n) {
                        std::this_thread::sleep_for(env->actualDuration(t-n));
                    }
                }
                auto iter1 = iter;
                ++iter1;
                typename M::template Data<typename Collection::value_type> d {{
                    env
                    , {
                        env->resolveTime(t)
                        , std::move(*iter)
                        , (iter1 == data_.end())
                    }
                }};
                iter = iter1;
                return {
                    (iter1 != data_.end())
                    , std::move(d)
                };
            }
        };
    public:
        template <class Collection, class TimeExtractorF>
        static std::shared_ptr<typename M::template Importer<typename Collection::value_type>> createImporter(
            Collection &&data
            , TimeExtractorF &&timeExtractorF
        ) {
            return infra::GenericLift<M>::lift(
                ImporterF<Collection, TimeExtractorF>(std::move(data), std::move(timeExtractorF))
                , infra::LiftParameters<typename M::TimePoint>().SuggestThreaded(true)
            );
        }
    };
} } } }

#endif
