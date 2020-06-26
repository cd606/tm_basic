#ifndef TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_ENV_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TRANSACTION_ENV_COMPONENT_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { 

namespace transaction { namespace v2 {

    template <class TI>
    class TransactionEnvComponent {
    public:
        virtual ~TransactionEnvComponent() {}
        virtual typename TI::GlobalVersion acquireLock(std::string const &account, std::string const &name) = 0;
        virtual typename TI::GlobalVersion releaseLock(std::string const &account, std::string const &name) = 0;
        virtual typename TI::TransactionResponse handleInsert(std::string const &account, typename TI::Key const &key, typename TI::Data const &data) = 0;
        virtual typename TI::TransactionResponse handleUpdate(std::string const &account, typename TI::Key const &key, std::optional<typename TI::VersionSlice> const &updateVersionSlice, typename TI::ProcessedUpdate const &processedUpdate) = 0;
        virtual typename TI::TransactionResponse handleDelete(std::string const &account, typename TI::Key const &key, std::optional<typename TI::Version> const &versionToDelete) = 0;
    };

} }

} } } } 

#endif