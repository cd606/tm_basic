#ifndef TM_KIT_BASIC_TRANSACTION_V2_SINGLE_PASS_ITERATION_DATA_STREAM_IMPORTER_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_SINGLE_PASS_ITERATION_DATA_STREAM_IMPORTER_HPP_

#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamEnvComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 { namespace single_pass_iteration {

    template <class TheEnvironment, class DI>
    class DataStreamImporter 
        :
        public infra::SinglePassIterationApp<TheEnvironment>::template AbstractImporter<
            typename DI::Update
        >
        , public DataStreamEnvComponent<DI>::Callback
    {
    private:
        using DataItem = typename infra::SinglePassIterationApp<TheEnvironment>::template InnerData<
            typename DI::Update
        >;
        TheEnvironment *env_;
        std::deque<DataItem> updates_;
    public:
        virtual ~DataStreamImporter() {}
        virtual void start(TheEnvironment *env) override final {
            env_ = env;
            static_cast<DataStreamEnvComponent<DI> *>(env)->initialize(this);
        }
        virtual void onUpdate(typename DI::Update &&u) override final {
            updates_.push_back(
                infra::withtime_utils::pureTimedDataWithEnvironment<
                    typename DI::Update
                    , TheEnvironment
                    , typename TheEnvironment::TimePointType
                >(env_, std::move(u))
            );
        }
        virtual typename infra::SinglePassIterationApp<TheEnvironment>::template Data<
            typename DI::Update
        > generate(typename DI::Update const * = nullptr) override final {
            if (updates_.empty()) {
                return std::nullopt;
            }
            DataItem d = std::move(updates_.front());
            updates_.pop_front();
            return d;
        }
    };

} } }

} } } }

#endif