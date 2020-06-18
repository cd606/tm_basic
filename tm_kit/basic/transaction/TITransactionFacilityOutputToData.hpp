#ifndef TM_KIT_BASIC_TRANSACTION_TI_TRANSACTION_FACILITY_OUTPUT_TO_DATA_HPP_
#define TM_KIT_BASIC_TRANSACTION_TI_TRANSACTION_FACILITY_OUTPUT_TO_DATA_HPP_

#include <tm_kit/basic/transaction/SingleKeyTransactionInterface.hpp>
#include <unordered_map>
#include <mutex>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class M
        , class KeyType
        , class DataType
        , class VersionType
        , bool MutexProtected = false
        , class DataSummaryType = DataType
        , class CheckSummary = std::equal_to<DataType>
        , class DataDeltaType = DataType
        , class ApplyDelta = CopyApplier<DataType>
        , class Cmp = std::less<VersionType>
        , class KeyHash = std::hash<KeyType>
    >
    class TITransactionFacilityOutputToData {
    private:
        Cmp cmp_;
        ApplyDelta applyDelta_;
        std::unordered_map<KeyType, std::tuple<VersionType, std::optional<DataType>>, KeyHash> store_;
        std::mutex mutex_;

        using TI = SingleKeyTransactionInterface<KeyType,DataType,VersionType,typename M::EnvironmentType::IDType,DataSummaryType,DataDeltaType,Cmp>;
        
        std::optional<typename TI::OneValue> handle(typename TI::FacilityOutput &&x) {
            return std::visit([this](auto &&arg) -> std::optional<typename TI::OneValue> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<typename TI::OneValue, T>) {
                    typename TI::OneValue v = std::move(arg);
                    auto iter = store_.find(v.groupID);
                    if (!v.data) {
                        if (iter != store_.end()) {
                            if (cmp_(std::get<0>(iter->second), v.version)) {
                                std::get<0>(iter->second) = v.version;
                                std::get<1>(iter->second) = std::nullopt;
                                return v;
                            } else {
                                return std::nullopt;
                            }
                        } else {
                            return v;
                        }
                    }
                    if (iter == store_.end()) {
                        store_.insert(std::make_pair(v.groupID, std::tuple<VersionType, std::optional<DataType>>(v.version, v.data)));
                        return v;
                    }
                    if (cmp_(std::get<0>(iter->second), v.version)) {
                        iter->second = std::tuple<VersionType, std::optional<DataType>>(v.version, v.data);
                        return v;
                    }
                    return std::nullopt;
                } else if constexpr (std::is_same_v<typename TI::OneDelta, T>) {
                    typename TI::OneDelta v = std::move(arg);
                    auto iter = store_.find(v.groupID);
                    if (iter == store_.end()) {
                        return std::nullopt;
                    }
                    if (!std::get<1>(iter->second)) {
                        return std::nullopt;
                    }
                    if (!cmp_(std::get<0>(iter->second), v.version)) {
                        return std::nullopt;
                    }
                    std::get<0>(iter->second) = v.version;
                    applyDelta_(*(std::get<1>(iter->second)), v.data);
                    return typename TI::OneValue {
                        iter->first
                        , std::get<0>(iter->second)
                        , std::get<1>(iter->second)
                    };
                } else {
                    return std::nullopt;
                }
            }, std::move(x.value));
        }
    public:
        TITransactionFacilityOutputToData() : cmp_(), applyDelta_(), store_(), mutex_() {
        }
        TITransactionFacilityOutputToData(TITransactionFacilityOutputToData &&d) :
            cmp_(std::move(d.cmp_)), applyDelta_(std::move(d.applyDelta_)), store_(std::move(d.store_)), mutex_() {}
        TITransactionFacilityOutputToData &operator=(TITransactionFacilityOutputToData &&d) = delete;
        TITransactionFacilityOutputToData(TITransactionFacilityOutputToData const &d) :
            cmp_(d.cmp_), applyDelta_(d.applyDelta_), store_(d.store_), mutex_() {}
        TITransactionFacilityOutputToData &operator=(TITransactionFacilityOutputToData const &d) = delete;    
        std::optional<typename TI::OneValue> operator()(typename TI::FacilityOutput &&x) {
            if constexpr (MutexProtected) {
                std::lock_guard<std::mutex> _(mutex_);
                return handle(std::move(x));
            } else {
                return handle(std::move(x));
            }
        }
    };

} } } } }

#endif