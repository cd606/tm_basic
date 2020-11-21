#ifndef TM_KIT_BASIC_WRAP_FACILITIOID_CONNECTOR_FOR_SERIALIZATION_HPP_
#define TM_KIT_BASIC_WRAP_FACILITIOID_CONNECTOR_FOR_SERIALIZATION_HPP_

#include <tm_kit/basic/AppRunnerUtils.hpp>
#include <tm_kit/basic/ByteData.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class R>
    class WrapFacilitioidConnectorForSerialization {
    public:
        template <class A, class B>
        static auto wrapClientSide(
            typename R::template FacilitioidConnector<
                std::conditional_t<
                    bytedata_utils::DirectlySerializableChecker<A>::IsDirectlySerializable()
                    , A 
                    , CBOR<A>
                >
                , std::conditional_t<
                    bytedata_utils::DirectlySerializableChecker<B>::IsDirectlySerializable()
                    , B
                    , CBOR<B>
                >
            > const &client
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<A,B>
        {
            if constexpr (bytedata_utils::DirectlySerializableChecker<A>::IsDirectlySerializable()) {
                if constexpr (bytedata_utils::DirectlySerializableChecker<B>::IsDirectlySerializable()) {
                    return client;
                } else {
                    return AppRunnerUtilComponents<R>::template wrapFacilitioidConnectorBackOnly<A,B,CBOR<B>>(
                        infra::KleisliUtils<typename R::AppType>::template liftPure<CBOR<B>>(
                            [](CBOR<B> &&b) -> B {
                                return std::move(b.value);
                            }
                        )
                        , client
                        , prefix
                    );
                }
            } else {
                if constexpr (bytedata_utils::DirectlySerializableChecker<B>::IsDirectlySerializable()) {
                    return AppRunnerUtilComponents<R>::template wrapFacilitioidConnectorFrontOnly<A,B,CBOR<A>>(
                        infra::KleisliUtils<typename R::AppType>::template liftPure<A>(
                            [](A &&a) -> CBOR<A> {
                                return {std::move(a)};
                            }
                        )
                        , infra::KleisliUtils<typename R::AppType>::template liftPure<CBOR<A>>(
                            [](CBOR<A> &&a) -> A {
                                return std::move(a.value);
                            }
                        )
                        , client
                        , prefix
                    );
                } else {
                    return AppRunnerUtilComponents<R>::template wrapFacilitioidConnector<A,B,CBOR<A>,CBOR<B>>(
                        infra::KleisliUtils<typename R::AppType>::template liftPure<A>(
                            [](A &&a) -> CBOR<A> {
                                return {std::move(a)};
                            }
                        )
                        , infra::KleisliUtils<typename R::AppType>::template liftPure<CBOR<A>>(
                            [](CBOR<A> &&a) -> A {
                                return std::move(a.value);
                            }
                        )
                        , infra::KleisliUtils<typename R::AppType>::template liftPure<CBOR<B>>(
                            [](CBOR<B> &&b) -> B {
                                return std::move(b.value);
                            }
                        )
                        , client
                        , prefix
                    );
                }
            }
        }
        template <class A, class B>
        static auto wrapServerSide(
            typename R::template FacilitioidConnector<A,B> const &server
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<
                std::conditional_t<
                    bytedata_utils::DirectlySerializableChecker<A>::IsDirectlySerializable()
                    , A 
                    , CBOR<A>
                >
                , std::conditional_t<
                    bytedata_utils::DirectlySerializableChecker<B>::IsDirectlySerializable()
                    , B
                    , CBOR<B>
                >
            >
        {
            if constexpr (bytedata_utils::DirectlySerializableChecker<A>::IsDirectlySerializable()) {
                if constexpr (bytedata_utils::DirectlySerializableChecker<B>::IsDirectlySerializable()) {
                    return server;
                } else {
                    return AppRunnerUtilComponents<R>::template wrapFacilitioidConnectorBackOnly<A,CBOR<B>,B>(
                        infra::KleisliUtils<typename R::AppType>::template liftPure<B>(
                            [](B &&b) -> CBOR<B> {
                                return {std::move(b)};
                            }
                        )
                        , server
                        , prefix
                    );
                }
            } else {
                if constexpr (bytedata_utils::DirectlySerializableChecker<B>::IsDirectlySerializable()) {
                    return AppRunnerUtilComponents<R>::template wrapFacilitioidConnectorFrontOnly<CBOR<A>,B,A>(
                        infra::KleisliUtils<typename R::AppType>::template liftPure<CBOR<A>>(
                            [](CBOR<A> &&a) -> A {
                                return std::move(a.value);
                            }
                        )
                        , infra::KleisliUtils<typename R::AppType>::template liftPure<A>(
                            [](A &&a) -> CBOR<A> {
                                return {std::move(a)};
                            }
                        )
                        , server
                        , prefix
                    );
                } else {
                    return AppRunnerUtilComponents<R>::template wrapFacilitioidConnector<CBOR<A>,CBOR<B>,A,B>(
                        infra::KleisliUtils<typename R::AppType>::template liftPure<CBOR<A>>(
                            [](CBOR<A> &&a) -> A {
                                return std::move(a.value);
                            }
                        )
                        , infra::KleisliUtils<typename R::AppType>::template liftPure<A>(
                            [](A &&a) -> CBOR<A> {
                                return {std::move(a)};
                            }
                        )
                        , infra::KleisliUtils<typename R::AppType>::template liftPure<B>(
                            [](B &&b) -> CBOR<B> {
                                return {std::move(b)};
                            }
                        )
                        , server
                        , prefix
                    );
                }
            }
        }
    };
} } } }

#endif