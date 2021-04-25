#ifndef TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_
#define TM_KIT_BASIC_SINGLE_PASS_ITERATION_CLOCK_CLOCK_ON_ORDER_FACILITY_HPP_

#include <type_traits>
#include <vector>
#include <queue>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/TraceNodesComponent.hpp>
#include <tm_kit/basic/ConstGenerator.hpp>
#include <tm_kit/basic/VoidStruct.hpp>
#include <tm_kit/basic/single_pass_iteration_clock/ClockComponent.hpp>

#include <boost/hana/type.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace single_pass_iteration_clock {

    template <class Env>
    class ClockOnOrderFacility {
    public:
        using M = infra::SinglePassIterationApp<Env>;
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
                    auto now_tp = input.timedData.timePoint;
                    std::vector<Duration> filteredDurations;
                    for (auto const &d : input.timedData.value.key().callbackDurations) {
                        if (d >= Duration(0)) {
                            filteredDurations.push_back(d);
                        }
                    }
                    auto id = input.timedData.value.id();
                    if (filteredDurations.empty()) {
                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                            input.environment
                            , {
                                now_tp
                                , {id, converter_(now_tp, 0, 0)}
                                , true
                            }
                        });
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        for (size_t ii=0; ii<count; ++ii) {
                            auto fire_tp = now_tp + filteredDurations[ii];
                            this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                input.environment
                                , {
                                    fire_tp
                                    , {id, converter_(fire_tp, ii, count)}
                                    , ii == count-1
                                }
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
                    : converter_(converter)
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
                    auto now_tp = input.timedData.timePoint;
                    std::vector<Duration> filteredDurations;
                    for (auto const &d : input.timedData.value.key().callbackDurations) {
                        if (d >= Duration(0)) {
                            filteredDurations.push_back(d);
                        }
                    }
                    auto id = input.timedData.value.id();
                    auto val = std::move(input.timedData.value.key().inputData);
                    if (filteredDurations.empty()) {
                        if constexpr (simple_callback_checker((F *) nullptr)) {
                            auto t  = converter_(now_tp, 0, 0);
                            if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                    input.environment
                                    , {
                                        now_tp
                                        , {id, std::move(t)}
                                        , true
                                    }
                                });
                            } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                std::size_t kk = t.size()-1;
                                for (auto &&item : t) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        input.environment
                                        , {
                                            now_tp
                                            , {id, std::move(item)}
                                            , (kk == 0)
                                        }
                                    });
                                    --kk;
                                }
                            }
                        } else if constexpr (complex_callback_checker((F *) nullptr)) {
                            auto t  = converter_(now_tp, Duration {}, 0, 0);
                            if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                    input.environment
                                    , {
                                        now_tp
                                        , {id, std::move(t)}
                                        , true
                                    }
                                });
                            } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                std::size_t kk = t.size()-1;
                                for (auto &&item : t) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        input.environment
                                        , {
                                            now_tp
                                            , {id, std::move(item)}
                                            , (kk == 0)
                                        }
                                    });
                                    --kk;
                                }
                            }
                        } else if constexpr (full_callback_checker((F *) nullptr)) {
                            auto t  = converter_(now_tp, Duration {}, 0, 0, std::move(val));
                            if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                    input.environment
                                    , {
                                        now_tp
                                        , {id, std::move(t)}
                                        , true
                                    }
                                });
                            } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                std::size_t kk = t.size()-1;
                                for (auto &&item : t) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        input.environment
                                        , {
                                            now_tp
                                            , {id, std::move(item)}
                                            , (kk == 0)
                                        }
                                    });
                                    --kk;
                                }
                            }
                        }                        
                    } else {
                        std::sort(filteredDurations.begin(), filteredDurations.end());
                        std::size_t count = filteredDurations.size();
                        for (size_t ii=0; ii<count; ++ii) {
                            auto fire_tp = now_tp + filteredDurations[ii];
                            bool isFinal = (ii == count-1);
                            if constexpr (simple_callback_checker((F *) nullptr)) {
                                auto t  = converter_(fire_tp, ii, count);
                                if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        input.environment
                                        , {
                                            fire_tp
                                            , {id, std::move(t)}
                                            , isFinal
                                        }
                                    });
                                } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                    std::size_t kk = t.size()-1;
                                    for (auto &&item : t) {
                                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                            input.environment
                                            , {
                                                fire_tp
                                                , {id, std::move(item)}
                                                , (isFinal && (kk == 0))
                                            }
                                        });
                                        --kk;
                                    }
                                }
                            } else if constexpr (complex_callback_checker((F *) nullptr)) {
                                auto t  = converter_(fire_tp, filteredDurations[ii], ii, count);
                                if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        input.environment
                                        , {
                                            fire_tp
                                            , {id, std::move(t)}
                                            , isFinal
                                        }
                                    });
                                } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                    std::size_t kk = t.size()-1;
                                    for (auto &&item : t) {
                                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                            input.environment
                                            , {
                                                fire_tp
                                                , {id, std::move(item)}
                                                , (isFinal && (kk == 0))
                                            }
                                        });
                                        --kk;
                                    }
                                }
                            } else if constexpr (full_callback_checker((F *) nullptr)) {
                                auto t  = converter_(fire_tp, filteredDurations[ii], ii, count, (isFinal?std::move(val):infra::withtime_utils::makeValueCopy<S>(val)));
                                if constexpr (std::is_same_v<T, std::decay_t<decltype(t)>>) {
                                    this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                        input.environment
                                        , {
                                            fire_tp
                                            , {id, std::move(t)}
                                            , isFinal
                                        }
                                    });
                                } else if constexpr (std::is_same_v<std::vector<T>, std::decay_t<decltype(t)>>) {
                                    std::size_t kk = t.size()-1;
                                    for (auto &&item : t) {
                                        this->publish(typename M::template InnerData<typename M::template Key<T>> {
                                            input.environment
                                            , {
                                                fire_tp
                                                , {id, std::move(item)}
                                                , (isFinal && (kk == 0))
                                            }
                                        });
                                        --kk;
                                    }
                                }
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