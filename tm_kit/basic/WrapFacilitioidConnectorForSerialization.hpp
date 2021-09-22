#ifndef TM_KIT_BASIC_WRAP_FACILITIOID_CONNECTOR_FOR_SERIALIZATION_HPP_
#define TM_KIT_BASIC_WRAP_FACILITIOID_CONNECTOR_FOR_SERIALIZATION_HPP_

#include <tm_kit/basic/AppRunnerUtils.hpp>
#include <tm_kit/basic/ByteData.hpp>

#include <tm_kit/basic/ProtoInterop.hpp>
#include <tm_kit/basic/StructFieldInfoBasedFlatPackUtils.hpp>
#include <tm_kit/basic/NlohmannJsonInterop.hpp>

#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    namespace WrapFacilitioidConnectorForSerializationHelpers {
        template <template <class... Xs> class Wrapper>
        class WrapperUtils {};
        template<>
        class WrapperUtils<std::void_t> {
        public:
            template <class T>
            static T extract(T &&x) {
                return std::move(x);
            }
            template <class T>
            static T enclose(T &&x) {
                return std::move(x);
            }
        };
        template<>
        class WrapperUtils<CBOR> {
        public:
            template <class T>
            static T extract(CBOR<T> &&x) {
                return std::move(x.value);
            }
            template <class T>
            static CBOR<T> enclose(T &&x) {
                return CBOR<T> {std::move(x)};
            }
        };
        template<>
        class WrapperUtils<proto_interop::Proto> {
        public:
            template <class T>
            static T extract(proto_interop::Proto<T> &&x) {
                return std::move(x).moveValue();
            }
            template <class T>
            static proto_interop::Proto<T> enclose(T &&x) {
                return proto_interop::Proto<T> {std::move(x)};
            }
        };
        template<>
        class WrapperUtils<nlohmann_json_interop::Json> {
        public:
            template <class T>
            static T extract(nlohmann_json_interop::Json<T> &&x) {
                return std::move(x).moveValue();
            }
            template <class T>
            static nlohmann_json_interop::Json<T> enclose(T &&x) {
                return nlohmann_json_interop::Json<T> {std::move(x)};
            }
        };
        template<>
        class WrapperUtils<struct_field_info_utils::FlatPack> {
        public:
            template <class T>
            static T extract(struct_field_info_utils::FlatPack<T> &&x) {
                return std::move(x).moveValue();
            }
            template <class T>
            static struct_field_info_utils::FlatPack<T> enclose(T &&x) {
                return struct_field_info_utils::FlatPack<T> {std::move(x)};
            }
        };
        template <template<class... Xs> class Wrapper, class T>
        struct WrappedTypeInternal {
            using TheType = Wrapper<T>;
        };
        template <class T>
        struct WrappedTypeInternal<std::void_t, T> {
            using TheType = T;
        };

        template <template<class... Xs> class Wrapper, class T>
        using WrappedType = typename WrappedTypeInternal<Wrapper,T>::TheType;
    }

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

        template <template<class... Xs> class Wrapper, class A, class B>
        static auto wrapClientSideWithProtocol(
            typename R::template FacilitioidConnector<
                WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>
                , WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>
            > const &client
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<A,B>
        {
            if constexpr(
                std::is_same_v<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>,A>
            ) {
                return client;
            } else {
                return AppRunnerUtilComponents<R>::template wrapFacilitioidConnector<A,B,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>>(
                    infra::KleisliUtils<typename R::AppType>::template liftPure<A>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template enclose<A>)
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template extract<A>)
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template extract<B>)
                    )
                    , client
                    , prefix
                );
            }
        }
        template <template<class... Xs> class Wrapper, class Extra, class A, class B>
        static auto wrapClientSideWithProtocol(
            typename R::template FacilitioidConnector<
                std::tuple<Extra, WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>
                , WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>
            > const &client
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<std::tuple<Extra,A>,B>
        {
            if constexpr(
                std::is_same_v<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>,A>
            ) {
                return client;
            } else {
                return AppRunnerUtilComponents<R>::template wrapFacilitioidConnector<std::tuple<Extra,A>,B,std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>>(
                    infra::KleisliUtils<typename R::AppType>::template liftPure<std::tuple<Extra,A>>(
                        [](std::tuple<Extra,A> &&x) -> std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>> {
                            return {std::move(std::get<0>(x)), WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template enclose<A>(std::move(std::get<1>(x)))};
                        }
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>>(
                        [](std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>> &&x) -> std::tuple<Extra,A> {
                            return {std::move(std::get<0>(x)), WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template extract<A>(std::move(std::get<1>(x)))};
                        }
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template extract<B>)
                    )
                    , client
                    , prefix
                );
            }
        }
        template <template <class... Xs> class Wrapper, class A, class B>
        static auto wrapServerSideWithProtocol(
            typename R::template FacilitioidConnector<A,B> const &server
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<
                WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>
                , WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>
            >
        {
            if constexpr(
                std::is_same_v<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>,A>
            ) {
                return server;
            } else {
                return AppRunnerUtilComponents<R>::template wrapFacilitioidConnector<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>,A,B>(
                    infra::KleisliUtils<typename R::AppType>::template liftPure<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template extract<A>)
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<A>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template enclose<A>)
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<B>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template enclose<B>)
                    )
                    , server
                    , prefix
                );
            }
        }
        template <template <class... Xs> class Wrapper, class Extra, class A, class B>
        static auto wrapServerSideWithProtocol(
            typename R::template FacilitioidConnector<std::tuple<Extra,A>,B> const &server
            , std::string const &prefix
        ) -> typename R::template FacilitioidConnector<
                std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>
                , WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>
            >
        {
            if constexpr(
                std::is_same_v<WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>,A>
            ) {
                return server;
            } else {
                return AppRunnerUtilComponents<R>::template wrapFacilitioidConnector<std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,B>,std::tuple<Extra,A>,B>(
                    infra::KleisliUtils<typename R::AppType>::template liftPure<std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>>>(
                        [](std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>> &&x) -> std::tuple<Extra,A> {
                            return {std::move(std::get<0>(x)), WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template extract<A>(std::move(std::get<1>(x)))};
                        }
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<std::tuple<Extra,A>>(
                        [](std::tuple<Extra,A> &&x) -> std::tuple<Extra,WrapFacilitioidConnectorForSerializationHelpers::WrappedType<Wrapper,A>> {
                            return {std::move(std::get<0>(x), WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template enclose<A>(std::move(std::get<1>(x))))};
                        }
                    )
                    , infra::KleisliUtils<typename R::AppType>::template liftPure<B>(
                        &(WrapFacilitioidConnectorForSerializationHelpers::WrapperUtils<Wrapper>::template enclose<B>)
                    )
                    , server
                    , prefix
                );
            }
        }
    };
} } } }

#endif