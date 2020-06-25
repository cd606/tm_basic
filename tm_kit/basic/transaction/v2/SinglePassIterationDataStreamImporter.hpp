#ifndef TM_KIT_BASIC_TRANSACTION_V2_SINGLE_PASS_ITERATION_DATA_STREAM_IMPORTER_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_SINGLE_PASS_ITERATION_DATA_STREAM_IMPORTER_HPP_

#include <tm_kit/infra/SinglePassIterationMonad.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamEnvComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 { namespace single_pass_iteration {

    template <class TheEnvironment, class DI>
    class DataStreamImporter 
        :
        public infra::SinglePassIterationMonad<TheEnvironment>::AbstractImporter<
            typename DI::Update
        >
        , public DataStreamEnvComponent<DI>::Callback
    {
    private:
        using DataItem = typename infra::SinglePassIterationMonad<TheEnvironment>::template InnerData<
            DI::Update
        >;
        TheEnvironment *env_;
        std::deque<DataItem> updates_;
    public:
        virtual ~DataStreamImporter() {}
        virtual void start(TheEnvironment *env) override final {
            env_ = env;
            static_cast<DataStreamEnvComponent<DI> *>(env)->initialize(this);
        }
        virtual void onUpdate(DI::Update &&u) override final {
            updates_.push_back(
                infra::withtime_utils::pureTimedDataWithEnvironment<
                    DI::Update
                    , TheEnvironment
                    , typename TheEnvironment::TimePointType
                >(env_, std::move(u));
            );
        }
        virtual typename infra::SinglePassIterationMonad<TheEnvironment>::template Data<
            DI::update
        > generate() override final {
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