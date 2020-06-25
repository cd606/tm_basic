#ifndef TM_KIT_BASIC_TRANSACTION_V2_TI_TRANSACTION_DELTA_MERGER_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TI_TRANSACTION_DELTA_MERGER_HPP_

#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/DefaultUtilities.hpp>
#include <unordered_map>
#include <mutex>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction {

    template <
        class DI
        , bool NeedOutput
        , class VersionDeltaMerger = TriviallyMerge<typename DI::Version, typename DI::VersionDelta>
        , class DataDeltaMerger = TriviallyMerge<typename DI::Data, typename DI::DataDelta>
        , class KeyHash = std::hash<typename DI::Key>
    >
    class TransactionDeltaMerger {
    private:
        TransactionDataStorePtr<DI,KeyHash> dataStore_;
        VersionDeltaMerger versionDeltaMerger_;
        DataDeltaMerger dataDeltaMerger_;

    public:
        using RetType = std::conditional_t<NeedOutput, std::optional<typename DI::FullUpdate>, void>;

    private:
        RetType handleFullUpdate(typename DI::FullUpdate &&update) {
            std::lock_guard<std::mutex> _(dataStore_->mutex_);
            auto iter = dataStore_->dataMap_.find(update.groupID);
            if (iter == dataStore_->dataMap_.end()) {
                iter = dataStore_->dataMap_.insert(std::make_pair(
                    std::move(update.groupID)
                    , {std::move(update.version), std::move(update.data)}
                )).first;
            } else {
                infra::withtime_utils::updateVersionedData(iter->second, {
                    std::move(update.version), std::move(update.data)
                });
            }
            if constexpr (NeedOutput) {
                return typename DI::FullUpdate {
                    infra::withtime_utils::make_value_copy(iter->first)
                    , infra::withtime_utils::make_value_copy(update.version)
                    infra::withtime_utils::make_value_copy(iter->second)
                };
            }
        }
        RetType handleDeltaUpdate(typename DI::DeltaUpdate &&update) {
            std::lock_guard<std::mutex> _(dataStore_->mutex_);
            auto iter = dataStore_->dataMap_.find(std::get<0>(update));
            if (iter == dataStore_->dataMap_.end() || !iter->second.data) {
                if constexpr (NeedOutput) {
                    return std::nullopt;
                } else {
                    return;
                }
            }
            versionDeltaMerger_(iter->second.version, std::get<1>(update));
            dataDeltaMerger_(*(iter->second.data), std::get<2>(update));
            if constexpr (NeedOutput) {
                return typename DI::FullUpdate {
                    infra::withtime_utils::make_value_copy(iter->first)
                    , infra::withtime_utils::make_value_copy(update.version)
                    infra::withtime_utils::make_value_copy(iter->second)
                };
            }
        }

        struct SetGlobalVersion {
            SetGlobalVersion(std::atomic<typename DI::GlobalVersion> *p, typename DI::GlobalVersion v) 
                : p_(p), v_(v)
            {}
            ~SetGlobalVersion() {
                p_ = v;
            }
        };

    public:
        TransactionDeltaMerger(TransactionDataStorePtr<DI,KeyHash> const &dataStore)
            : dataStore_(dataStore), versionDeltaMerger_(), dataDeltaMerger_() {}
        TransactionDeltaMerger &setVersionDeltaMerger(VersionDeltaMerger &&vm) {
            versionDeltaMerger_ = std::move(vm);
            return *this;
        }
        TransactionDeltaMerger &setDataDeltaMerger(DataDeltaMerger &&dm) {
            dataDeltaMerger_ = std::move(dm);
            return *this;
        }
        RetType operator()(typename DI::Update &&update) const {
            SetGlobalVersion _(&(dataStore_->globalVersion_), update.globalVersion);
            if constexpr (NeedOutput) {
                std::visit([](auto &&u) -> RetType {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, typename DI::FullUpdate>) {
                        return handleFullUpdate(std::move(u));
                    } else if constexpr (std::is_same_v<T, typename DI::DeltaUpdate>) {
                        return handleDeltaUpdate(std::move(u));
                    } else {
                        return std::nullopt;
                    }
                }, std::move(update.data));
            } else {
                std::visit([](auto &&u) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, typename DI::FullUpdate>) {
                        handleFullUpdate(std::move(u));
                    } else if constexpr (std::is_same_v<T, typename DI::DeltaUpdate>) {
                        handleDeltaUpdate(std::move(u));
                    }
                }, std::move(update.data));
            }
            
        }
    };

} } } } }

#endif