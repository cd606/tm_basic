#ifndef TM_KIT_BASIC_TRANSACTION_V2_TI_TRANSACTION_DELTA_MERGER_HPP_
#define TM_KIT_BASIC_TRANSACTION_V2_TI_TRANSACTION_DELTA_MERGER_HPP_

#include <tm_kit/basic/transaction/v2/DataStreamInterface.hpp>
#include <tm_kit/basic/transaction/v2/TransactionDataStore.hpp>
#include <tm_kit/basic/transaction/v2/DefaultUtilities.hpp>
#include <unordered_map>
#include <mutex>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace transaction { namespace v2 {

    template <
        class DI
        , bool NeedOutput
        , bool MutexProtected = true
        , class VersionDeltaMerger = TriviallyMerge<typename DI::Version, typename DI::VersionDelta>
        , class DataDeltaMerger = TriviallyMerge<typename DI::Data, typename DI::DataDelta>
        , class KeyHashType = std::hash<typename DI::Key>
    >
    class TransactionDeltaMerger {
    private:
        using TransactionDataStorePtr = transaction::v2::TransactionDataStorePtr<DI,KeyHashType,MutexProtected>;
        using TransactionDataStore = typename TransactionDataStorePtr::element_type;

        TransactionDataStorePtr dataStore_;
        VersionDeltaMerger versionDeltaMerger_;
        DataDeltaMerger dataDeltaMerger_;

        using Lock = typename TransactionDataStore::Lock;
        using SafeGV = typename TransactionDataStore::GlobalVersion;

    public:
        static constexpr bool IsMutexProtected = MutexProtected;
        using DataStreamInterfaceType = DI;
        using KeyHash = KeyHashType;
        using RetType = std::conditional_t<NeedOutput, typename DI::FullUpdate, void>;

    private:
        void handleFullUpdate(typename DI::OneFullUpdateItem &&update) const {
            Lock _(dataStore_->mutex_);
            auto iter = dataStore_->dataMap_.find(update.groupID);
            if (iter == dataStore_->dataMap_.end()) {
                dataStore_->dataMap_.insert(typename TransactionDataStore::DataMap::value_type {
                    std::move(update.groupID)
                    , typename TransactionDataStore::DataMap::mapped_type {std::move(update.version), { std::move(update.data) }}
                });
            } else {
                infra::withtime_utils::updateVersionedData(iter->second, {
                    std::move(update.version), std::move(update.data)
                });
            }
        }
        void handleDeltaUpdate(typename DI::OneDeltaUpdateItem &&update) const {
            Lock _(dataStore_->mutex_);
            auto iter = dataStore_->dataMap_.find(std::get<0>(update));
            if (iter == dataStore_->dataMap_.end() || !iter->second.data) {
                return;
            }
            versionDeltaMerger_(iter->second.version, std::get<1>(update));
            dataDeltaMerger_(*(iter->second.data), std::get<2>(update));
        }

        struct SetGlobalVersion {
            SafeGV *p_;
            typename DI::GlobalVersion v_;
            SetGlobalVersion(SafeGV *p, typename DI::GlobalVersion v) 
                : p_(p), v_(v)
            {}
            ~SetGlobalVersion() {
                *p_ = v_;
            }
        };

    public:
        TransactionDeltaMerger(TransactionDataStorePtr const &dataStore)
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
            std::vector<typename DI::Key> keys;
            {
                SetGlobalVersion _(&(dataStore_->globalVersion_), update.version);
                std::unordered_set<typename DI::Key, KeyHash> keySet;
                for (auto &&item : update.data) {
                    std::visit([this,&keys,&keySet](auto &&u) {
                        using T = std::decay_t<decltype(u)>;
                        if constexpr (std::is_same_v<T, typename DI::OneFullUpdateItem>) {
                            if constexpr (NeedOutput) {
                                if (keySet.find(u.groupID) == keySet.end()) {
                                    keys.push_back(u.groupID);
                                    keySet.insert(u.groupID);
                                }
                            }
                            handleFullUpdate(std::move(u));
                        } else if constexpr (std::is_same_v<T, typename DI::OneDeltaUpdateItem>) {
                            if constexpr (NeedOutput) {
                                if (keySet.find(std::get<0>(u)) == keySet.end()) {
                                    keys.push_back(std::get<0>(u));
                                    keySet.insert(std::get<0>(u));
                                }
                            }
                            handleDeltaUpdate(std::move(u));
                        }
                    }, std::move(item.value));
                }
            }
            if constexpr (NeedOutput) {
                return dataStore_->createFullUpdateNotificationWithoutVariant(keys);
            }
        }
    };

} } } } } }

#endif