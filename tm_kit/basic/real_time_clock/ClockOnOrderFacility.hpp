#ifndef TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_REAL_TIME_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/real_time_clock/ClockComponent.hpp>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace real_time_clock {

    template <class Env, std::enable_if_t<std::is_base_of_v<ClockComponent, Env>, int> = 0>
    class ClockOnOrderFacility {
    public:
        using M = infra::RealTimeApp<Env>;
        using Duration = typename M::Duration;
        template <class S>
        struct FacilityInput {
            S inputData;
            std::vector<Duration> callbackDurations;
        };
        template <class S, class T>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createClockCallback(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter) {
            class LocalF final : public M::template AbstractOnOrderFacility<FacilityInput<S>,T>, public virtual infra::IControllableNode<Env> {
            private:
                std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter_;
                std::unordered_map<uint64_t, std::function<void()>> cancellors_;
                std::recursive_mutex cancellorsMutex_;
                uint64_t cancellorCount_;
            public:
                LocalF(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter)
                    : converter_(converter), cancellors_(), cancellorsMutex_(), cancellorCount_(0)
                {
                }
                virtual void handle(typename M::template InnerData<typename M::template Key<FacilityInput<S>>> &&input) override final {
                    TM_INFRA_FACILITY_TRACER(input.environment);
                    std::vector<Duration> filteredDurations;
                    for (auto const &d : input.timedData.value.key().callbackDurations) {
                        if (d >= Duration(0)) {
                            filteredDurations.push_back(d);
                        }
                    }
                    Env *env = input.environment;
                    typename Env::IDType id = input.timedData.value.id();
                    if (filteredDurations.empty()) {
                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                        ++cancellorCount_;
                        cancellors_[cancellorCount_] = env->createOneShotDurationTimer(Duration(0), [this,env,id,cancellorID=cancellorCount_]() {
                            auto now = env->now();
                            T t = converter_(now, 0, 0);
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                env, {now, {id, std::move(t)}, true}
                            });
                            {
                                std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                                cancellors_.erase(cancellorID);
                            }
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                        for (size_t ii=0; ii<count; ++ii) {
                            bool isFinal = (ii == count-1);
                            ++cancellorCount_;
                            cancellors_[cancellorCount_] = env->createOneShotDurationTimer(filteredDurations[ii], [this,env,id,isFinal,ii,count,cancellorID=cancellorCount_]() {
                                auto now = env->now();
                                T t = converter_(now, ii, count);
                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                    env, {now, {id, std::move(t)}, isFinal}
                                });
                                {
                                    std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                                    cancellors_.erase(cancellorID);
                                }
                            });
                        }
                    }
                }
                void control(Env */*env*/, std::string const &command, std::vector<std::string> const &params) override final {
                    if (command == "stop") {
                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                        for (auto const &item : cancellors_) {
                            item.second();
                        }
                        cancellors_.clear();
                    }
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF(converter));
        }
        template <class S, class T, class F>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createGenericClockCallback(F &&converter) {
            class LocalF final : public M::template AbstractOnOrderFacility<FacilityInput<S>,T>, public virtual infra::IControllableNode<Env> {
            private:
                F converter_;
                std::unordered_map<uint64_t, std::function<void()>> cancellors_;
                std::recursive_mutex cancellorsMutex_;
                uint64_t cancellorCount_;
            public:
                LocalF(F &&converter)
                    : converter_(std::move(converter)), cancellors_(), cancellorsMutex_(), cancellorCount_(0)
                {
                }
                virtual void handle(typename M::template InnerData<typename M::template Key<FacilityInput<S>>> &&input) override final {
                    static const auto simple_callback_checker = boost::hana::is_valid(
                        [](auto *c) -> decltype((void) (*c)(
                            std::declval<typename Env::TimePointType>()
                            , (std::size_t) 0
                            , (std::size_t) 0
                        )) {}
                    );
                    static const auto complex_callback_checker = boost::hana::is_valid(
                        [](auto *c) -> decltype((void) (*c)(
                            std::declval<typename Env::TimePointType>()
                            , std::declval<Duration>()
                            , (std::size_t) 0
                            , (std::size_t) 0
                        )) {}
                    );
#ifdef _MSC_VER
                    static const auto full_callback_checker = boost::hana::is_valid(
                        [](auto *c, auto *s) -> decltype((void) (*c)(
                            std::declval<typename Env::TimePointType>()
                            , std::declval<Duration>()
                            , (std::size_t) 0
                            , (std::size_t) 0
                            , std::move(*s)
                        )) {}
                    );
#else
                    static const auto full_callback_checker = boost::hana::is_valid(
                        [](auto *c) -> decltype((void) (*c)(
                            std::declval<typename Env::TimePointType>()
                            , std::declval<Duration>()
                            , (std::size_t) 0
                            , (std::size_t) 0
                            , std::declval<S>()
                        )) {}
                    );
#endif
                    TM_INFRA_FACILITY_TRACER(input.environment);
                    std::vector<Duration> filteredDurations;
                    for (auto const &d : input.timedData.value.key().callbackDurations) {
                        if (d >= Duration(0)) {
                            filteredDurations.push_back(d);
                        }
                    }
                    Env *env = input.environment;
                    typename Env::IDType id = input.timedData.value.id();
                    auto val = std::move(input.timedData.value.key().inputData);
                    if (filteredDurations.empty()) {
                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                        ++cancellorCount_;
                        cancellors_[cancellorCount_] = env->createOneShotDurationTimer(Duration(0), [this,env,id,val=std::move(val),cancellorID=cancellorCount_]() {
                            auto now = env->now();
                            if constexpr (simple_callback_checker((F *) nullptr)) {
                                auto t = converter_(now, 0, 0);
                                if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        env, {now, {id, std::move(t)}, true}
                                    });
                                } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                    if (t.empty()) {
                                        this->markEndHandlingRequest(id);
                                    } else {
                                        std::size_t kk = t.size()-1;
                                        for (auto &&item : t) {
                                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                                env, {now, {id, std::move(item)}, (kk==0)}
                                            });
                                            --kk;
                                        }
                                    }
                                }
                            } else if constexpr (complex_callback_checker((F *) nullptr)) {
                                auto t = converter_(now, Duration {}, 0, 0);
                                if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        env, {now, {id, std::move(t)}, true}
                                    });
                                } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                    if (t.empty()) {
                                        this->markEndHandlingRequest(id);
                                    } else {
                                        std::size_t kk = t.size()-1;
                                        for (auto &&item : t) {
                                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                                env, {now, {id, std::move(item)}, (kk==0)}
                                            });
                                            --kk;
                                        }
                                    }
                                }
#ifdef _MSC_VER
                            } else if constexpr (full_callback_checker((F *) nullptr, (S *) nullptr)) {
#else
                            } else if constexpr (full_callback_checker((F *) nullptr)) {
#endif
                                auto val1 = std::move(val);
                                auto t = converter_(now, Duration {}, 0, 0, std::move(val1));
                                if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        env, {now, {id, std::move(t)}, true}
                                    });
                                } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                    if (t.empty()) {
                                        this->markEndHandlingRequest(id);
                                    } else {
                                        std::size_t kk = t.size()-1;
                                        for (auto &&item : t) {
                                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                                env, {now, {id, std::move(item)}, (kk==0)}
                                            });
                                            --kk;
                                        }
                                    }
                                }
                            } else {
                                throw std::runtime_error("real_time_clock::ClockFacility::createGenericClockCallback: bad function type");
                            }
                            {
                                std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                                cancellors_.erase(cancellorID);
                            }
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                        for (size_t ii=0; ii<count; ++ii) {
                            bool isFinal = (ii == count-1);
                            if constexpr (simple_callback_checker((F *) nullptr)) {
                                ++cancellorCount_;
                                cancellors_[cancellorCount_] = env->createOneShotDurationTimer(filteredDurations[ii], [this,env,id,isFinal,ii,count,cancellorID=cancellorCount_]() {
                                    auto now = env->now();                                
                                    auto t = converter_(now, ii, count);
                                    if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                            env, {now, {id, std::move(t)}, isFinal}
                                        });
                                    } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                        if (t.empty()) {
                                            if (isFinal) {
                                                this->markEndHandlingRequest(id);
                                            }
                                        } else {
                                            std::size_t kk = t.size()-1;
                                            for (auto &&item : t) {
                                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                                    env, {now, {id, std::move(item)}, (isFinal && (kk==0))}
                                                });
                                                --kk;
                                            }
                                        }
                                    }
                                    {
                                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                                        cancellors_.erase(cancellorID);
                                    }
                                });
                            } else if constexpr (complex_callback_checker((F *) nullptr)) {
                                auto d = filteredDurations[ii];
                                ++cancellorCount_;
                                cancellors_[cancellorCount_] = env->createOneShotDurationTimer(d, [this,env,id,isFinal,ii,count,d,cancellorID=cancellorCount_]() {
                                    auto now = env->now();                                
                                    auto t = converter_(now, d, ii, count);
                                    if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                            env, {now, {id, std::move(t)}, isFinal}
                                        });
                                    } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                        if (t.empty()) {
                                            if (isFinal) {
                                                this->markEndHandlingRequest(id);
                                            }
                                        } else {
                                            std::size_t kk = t.size()-1;
                                            for (auto &&item : t) {
                                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                                    env, {now, {id, std::move(item)}, (isFinal && (kk==0))}
                                                });
                                                --kk;
                                            }
                                        }
                                    }
                                    {
                                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                                        cancellors_.erase(cancellorID);
                                    }
                                });
#ifdef _MSC_VER
                            } else if constexpr (full_callback_checker((F *) nullptr, (S *) nullptr)) {
#else
                            } else if constexpr (full_callback_checker((F *) nullptr)) {
#endif
                                auto d = filteredDurations[ii];                                
                                ++cancellorCount_;
                                cancellors_[cancellorCount_] = env->createOneShotDurationTimer(d, [this,env,id,isFinal,ii,count,d,val=(isFinal?std::move(val):infra::withtime_utils::makeValueCopy<S>(val)),cancellorID=cancellorCount_]() {
                                    auto now = env->now();  
                                    auto val1 = std::move(val);
                                    auto t = converter_(now, d, ii, count, std::move(val1));
                                    if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                            env, {now, {id, std::move(t)}, isFinal}
                                        });
                                    } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                        if (t.empty()) {
                                            if (isFinal) {
                                                this->markEndHandlingRequest(id);
                                            }
                                        } else {
                                            std::size_t kk = t.size()-1;
                                            for (auto &&item : t) {
                                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                                    env, {now, {id, std::move(item)}, (isFinal && (kk==0))}
                                                });
                                                --kk;
                                            }
                                        }
                                    }
                                    {
                                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                                        cancellors_.erase(cancellorID);
                                    }
                                });
                            } else {
                                throw std::runtime_error("real_time_clock::ClockFacility::createGenericClockCallback: bad function type");
                            }
                        }
                    }
                }
                void control(Env */*env*/, std::string const &command, std::vector<std::string> const &params) override final {
                    if (command == "stop") {
                        std::lock_guard<std::recursive_mutex> _(cancellorsMutex_);
                        for (auto const &item : cancellors_) {
                            item.second();
                        }
                        cancellors_.clear();
                    }
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF(std::move(converter)));
        }
    };

} } } } }

#endif