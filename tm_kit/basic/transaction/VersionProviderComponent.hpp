#ifndef TM_KIT_BASIC_TRANSACTION_VERSION_PROVIDER_COMPONENT_HPP_
#define TM_KIT_BASIC_TRANSACTION_VERSION_PROVIDER_COMPONENT_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <class KeyType, class DataType, class VersionType>
    class VersionProviderComponent {
    public:
        virtual ~VersionProviderComponent() {}
        virtual VersionType getNextVersionForKey(KeyType const &) = 0;
    };

} } } } }

#endif