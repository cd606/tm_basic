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
        using Duration = typename Env::DurationType;
        template <class S>
        struct FacilityInput {
            S inputData;
            std::vector<Duration> callbackDurations;
        };
        template <class S, class T>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createClockCallback(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter) {
            class LocalF final : public M::template AbstractOnOrderFacility<FacilityInput<S>,T> {
            private:
                std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter_;
            public:
                LocalF(std::function<T(typename Env::TimePointType const &, std::size_t, std::size_t)> converter)
                    : converter_(converter)
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
                        env->createOneShotDurationTimer(Duration(0), [this,env,id]() {
                            auto now = env->now();
                            T t = converter_(now, 0, 0);
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                env, {now, {id, std::move(t)}, true}
                            });
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        for (size_t ii=0; ii<count; ++ii) {
                            bool isFinal = (ii == count-1);
                            env->createOneShotDurationTimer(filteredDurations[ii], [this,env,id,isFinal,ii,count]() {
                                auto now = env->now();
                                T t = converter_(now, ii, count);
                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                    env, {now, {id, std::move(t)}, isFinal}
                                });
                            });
                        }
                    }
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF(converter));
        }
        template <class S, class T, class F>
        static std::shared_ptr<typename M::template OnOrderFacility<FacilityInput<S>,T>> createGenericClockCallback(F &&converter) {
            class LocalF final : public M::template AbstractOnOrderFacility<FacilityInput<S>,T> {
            private:
                F converter_;
            public:
                LocalF(F &&converter)
                    : converter_(std::move(converter))
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
                    static const auto full_callback_checker = boost::hana::is_valid(
                        [](auto *c) -> decltype((void) (*c)(
                            std::declval<typename Env::TimePointType>()
                            , std::declval<Duration>()
                            , (std::size_t) 0
                            , (std::size_t) 0
                            , std::declval<S>()
                        )) {}
                    );
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
                        env->createOneShotDurationTimer(Duration(0), [this,env,id,val=std::move(val)]() {
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
                            } else if constexpr (full_callback_checker((F *) nullptr)) {
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
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        for (size_t ii=0; ii<count; ++ii) {
                            bool isFinal = (ii == count-1);
                            if constexpr (simple_callback_checker((F *) nullptr)) {
                                env->createOneShotDurationTimer(filteredDurations[ii], [this,env,id,isFinal,ii,count]() {
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
                                });
                            } else if constexpr (complex_callback_checker((F *) nullptr)) {
                                auto d = filteredDurations[ii];
                                env->createOneShotDurationTimer(d, [this,env,id,isFinal,ii,count,d]() {
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
                                });
                            } else if constexpr (full_callback_checker((F *) nullptr)) {
                                auto d = filteredDurations[ii];                                
                                env->createOneShotDurationTimer(d, [this,env,id,isFinal,ii,count,d,val=(isFinal?std::move(val):infra::withtime_utils::makeValueCopy<S>(val))]() {
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
                                });
                            } else {
                                throw std::runtime_error("real_time_clock::ClockFacility::createGenericClockCallback: bad function type");
                            }
                        }
                    }
                }
            };
            return M::fromAbstractOnOrderFacility(new LocalF(std::move(converter)));
        }
    };

} } } } }

#endif