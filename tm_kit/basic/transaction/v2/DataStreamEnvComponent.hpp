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
            virtual void onFullUpdate(typename DI::FullUpdate const &) = 0;
            virtual void onDeltaUpdate(typename DI::DeltaUpdate const &) = 0;
        };
        virtual bool exclusivelyBindTo(Callback *cb) = 0;
        virtual void initialize() = 0;
        virtual void startWatchIfNecessary(std::string const &account, KeyType const &key) = 0;
        virtual void discretionallyStopWatch(std::string const &account, KeyType const &key) = 0;
    };

    template <class DI>
    class SynchronizedDataStreamComponent : public DataStreamComponent<DI> {
    public:
        virtual void triggerFullUpdateCallback(typename DI::FullUpdate const &) = 0;
        virtual void triggerDeltaUpdateCallback(typename DI::DeltaUpdate const &) = 0;
    };

} }

} } } } 

#endif