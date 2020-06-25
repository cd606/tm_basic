#ifndef TM_KIT_BASIC_TRANSACTION_V2_DEFAULT_UTILITIES_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_DEFAULT_UTILITIES_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    template <bool V, class A, class B>
    struct ConstChecker {
        bool operator()(A const &, B const &) const {
            return V;
        }
    };

    template <class Data, class DataDelta>
    struct TriviallyMerge {
        void operator()(Data &d, DataDelta const &delta) const {
            d = delta;
        }
    };

    template <class Data, class DataDelta, class ProcessedUpdate>
    struct TriviallyProcess {
        std::optional<ProcessedUpdate> operator()(Data &d, DataDelta const &delta) const {
            d = delta;
            return ProcessedUpdate {delta};
        }
    };
} } 

} } } } 

#endif