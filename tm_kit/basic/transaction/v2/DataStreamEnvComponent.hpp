#ifndef TM_KIT_BASIC_TRANSACTION_V2_DATA_STREAM_ENV_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_DATA_STREAM_ENV_COMPONENT_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { 

namespace transaction { namespace v2 {

    template <class DI>
    class DataStreamEnvComponent {
    public:
        class Callback {
        public:
            virtual ~Callback() {}
            virtual void onUpdate(typename DI::Update &&) = 0;
        };
        virtual ~DataStreamEnvComponent() {}
        virtual void initialize(Callback *cb) = 0;
    };

} }

} } } } 

#endif