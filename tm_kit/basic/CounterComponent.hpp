#ifndef TM_KIT_INFRA_COUNTER_COMPONENT_HPP_
#define TM_KIT_INFRA_COUNTER_COMPONENT_HPP_

#include <type_traits>
#include <atomic>
#include <tm_kit/basic/NotConstructibleStruct.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class MarkerType=NotConstructibleStruct, class IntType=uint64_t, typename=std::enable_if_t<std::is_unsigned_v<IntType>>>
    class CounterComponent {
    private:
        std::atomic<IntType> counter_;
    public:
        CounterComponent() : counter_(0) {}
        ~CounterComponent() {}
        IntType getCounterValue(MarkerType *notUsed) {
            return counter_.fetch_add(1);
        }
    };
} } } }

#endif