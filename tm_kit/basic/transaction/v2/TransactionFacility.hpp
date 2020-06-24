#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_FACILITY_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_FACILITY_HPP_

#include <tm_kit/basic/transaction/v2/TransactionInterface.hpp>
#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    
namespace transaction { namespace v2 {

    template <
        class M, class TI, class DI, class Enable=void
    >
    class TransactionFacility {}

    template <
        class M, class TI, class DI
    >
    class TransactionFacility<
        M, TI, DI
        , std::enable_if_t<
            std::is_same_v<typename TI::GlobalVersion, typename DI::GlobalVersion>
            && std::is_same_v<typename TI::Key, typename DI::Key>
            && std::is_same_v<typename TI::Version, typename DI::Version>
            && std::is_same_v<typename TI::Data, typename DI::Data>
        >
    >
        : 
        public M::template AbstractOnOrderFacility<
            typename TI::TransactionWithAccountInfo
            , typename TI::TransactionResponse
        > 
    {
    public:
        void handle(typename M::template InnerData<typename M::template Key<
            typename TI::TransactionWithAccountInfo
        >> &&transactionWithAccountInfo) override final {

        }
    };

} } 

} } } }

#endif