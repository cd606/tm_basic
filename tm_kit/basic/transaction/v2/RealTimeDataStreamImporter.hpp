#ifndef TM_KIT_BASIC_TRANSACTION_V2_REAL_TIME_DATA_STREAM_IMPORTER_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_REAL_TIME_DATA_STREAM_IMPORTER_HPP_

#include <tm_kit/infra/RealTimeMonad.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamEnvComponent.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 { namespace real_time {

    template <class TheEnvironment, class DI>
    class DataStreamImporter 
        :
        public infra::RealTimeMonad<TheEnvironment>::AbstractImporter<
            typename DI::Update
        >
        , public DataStreamEnvComponent<DI>::Callback
    {
    private:
        TheEnvironment *env_;
    public:
        virtual ~DataStreamImporter() {}
        virtual void start(TheEnvironment *env) override final {
            env_ = env;
            static_cast<DataStreamEnvComponent<DI> *>(env)->initialize(this);
        }
        virtual void onUpdate(typename DI::Update &&u) override final {
            this->publish(env_, std::move(u));
        }
    };

} } }

} } } }

#endif