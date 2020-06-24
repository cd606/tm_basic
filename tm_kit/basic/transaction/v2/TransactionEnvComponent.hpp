#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_ENV_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_ENV_COMPONENT_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { 

namespace transaction { namespace v2 {

    template <class TI>
    class TransactionEnvComponent {
    public:
        virtual typename TI::TransactionResponse handleInsert(std::string const &account, typename TI::InsertAction const &insertAction) = 0;
        virtual typename TI::TransactionResponse handleUpdate(std::string const &account, typename TI::UpdateAction const &updateAction) = 0;
        virtual typename TI::TransactionResponse handleDelete(std::string const &account, typename TI::DeleteAction const &deleteAction) = 0;
    };

} }

} } } } 

#endif