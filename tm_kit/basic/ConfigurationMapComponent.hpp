#ifndef TM_KIT_BASIC_CONFIGURATION_MAP_COMPONENT_HPP_
#define TM_KIT_BASIC_CONFIGURATION_MAP_COMPONENT_HPP_

#include <unordered_map>
#include <mutex>
#include <memory>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    
    template <class ConfigurationKey, class ConfigurationData, class ConfigurationKeyHash=std::hash<ConfigurationKey>>
    class ConfigurationMapComponent {
    private:
        std::unordered_map<ConfigurationKey, ConfigurationData, ConfigurationKeyHash> data_;
        std::unique_ptr<std::mutex> mutex_;
    public:
        ConfigurationMapComponent() : data_(), mutex_(std::make_unique<std::mutex>()) {}
        virtual ~ConfigurationMapComponent() = default;
        ConfigurationMapComponent(ConfigurationMapComponent const &) = delete;
        ConfigurationMapComponent &operator=(ConfigurationMapComponent const &) = delete;
        ConfigurationMapComponent(ConfigurationMapComponent &&) = default;
        ConfigurationMapComponent &operator=(ConfigurationMapComponent &&) = default;

        void setConfigurationItem(ConfigurationKey const &key, ConfigurationData const &data) {
            std::lock_guard<std::mutex> _(*mutex_);
            data_[key] = data;
        }
        std::optional<ConfigurationData> getConfigurationItem(ConfigurationKey const &key) const {
            std::lock_guard<std::mutex> _(*mutex_);
            auto iter = data_.find(key);
            if (iter == data_.end()) {
                return std::nullopt;
            }
            return iter->second;
        }
    };

}}}}

#endif
