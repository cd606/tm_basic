#ifndef TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_
#define TM_KIT_BASIC_SIMPLE_SHARED_CHAIN_CHAIN_READER_HPP_

#include <tm_kit/basic/VoidStruct.hpp>
#include <optional>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace simple_shared_chain {
    template <class App, class Chain, class ChainItemFolder, class TriggerT=VoidStruct>
    class ChainReader {
    private:
        using Env = typename App::EnvironmentType;
        Chain *chain_;
        typename Chain::ItemType currentItem_;
        ChainItemFolder folder_;
        typename ChainItemFolder::ResultType currentValue_;
    public:
        ChainReader(Env *env, Chain *chain) : 
            chain_(chain)
            , currentItem_()
            , folder_()
            , currentValue_(folder_.initialize(env)) 
        {
            auto checker = boost::hana::is_valid(
                [](auto *c, auto *f, auto const *v) -> decltype((void) (c->loadUntil((Env *) nullptr, f->chainIDForValue(*v)))) {}
            );
            if constexpr (checker(
                (Chain *) nullptr
                , (ChainItemFolder *) nullptr
                , (typename ChainItemFolder::ResultType const *) nullptr
            )) {
                currentItem_ = chain_->loadUntil(env, folder_.chainIDForValue(currentValue_));
            } else {
                currentItem_ = chain_->head(env);
            }
        }
        ~ChainReader() = default;
        ChainReader(ChainReader const &) = delete;
        ChainReader &operator=(ChainReader const &) = delete;
        ChainReader(ChainReader &&) = default;
        ChainReader &operator=(ChainReader &&) = default;
        typename std::optional<typename ChainItemFolder::ResultType> operator()(TriggerT &&) {
            bool hasNew = false;
            while (true) {
                std::optional<typename Chain::ItemType> nextItem = chain_->fetchNext(currentItem_);
                if (!nextItem) {
                    break;
                }
                currentItem_ = std::move(*nextItem);
                currentValue_ = folder_.fold(currentValue_, currentItem_);
                hasNew = true;
            }
            if (hasNew) {
                return {currentValue_};
            } else {
                return std::nullopt;
            }
        }
    };
} } } } }

#endif