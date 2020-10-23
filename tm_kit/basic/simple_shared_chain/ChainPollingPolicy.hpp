#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_POLLING_POLICY_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_POLLING_POLICY_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    struct ChainPollingPolicy {
        bool callbackPerUpdate = false;
        bool busyLoop = false;
        bool noYield = false;

        ChainPollingPolicy &CallbackPerUpdate(bool b) {
            callbackPerUpdate = b;
            return *this;
        }
        ChainPollingPolicy &BusyLoop(bool b) {
            busyLoop = b;
            return *this;
        }
        ChainPollingPolicy &NoYield(bool b) {
            noYield = b;
            if (b) {
                busyLoop = true;
            }
            return *this;
        }
    };
} } } } }

#endif