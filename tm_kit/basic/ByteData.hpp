#ifndef TM_KIT_BASIC_BYTE_DATA_HPP_
#define TM_KIT_BASIC_BYTE_DATA_HPP_

#include <string>
#include <type_traits>
#include <memory>
#include <iostream>
#include <tm_kit/basic/ConstGenerator.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic { 

    struct ByteData {
        std::string content;
    };

    struct ByteDataWithTopic {
        std::string topic;
        std::string content;
    };

    struct ByteDataWithID {
        std::string id; //When we reach the transport level, all IDs are already converted to string representations
        std::string content;       
    };

    template <class T>
    struct TypedDataWithTopic {
        using ContentType = T;
        std::string topic;
        T content;
    };

    class SerializationFunctions {
    private:
        template <class T, int B=(std::is_same_v<T,std::string>?1:(std::is_same_v<T,ByteData>?2:0))>
        class S {};

        template <class T>
        class S<T, 0> {
        public:
            static std::string apply(T const &data) {
                std::string s;
                data.SerializeToString(&s);
                return s;
            }
        };
        template <class T>
        class S<T, 1>  {
        public:
            static std::string apply(T const &data) {
                return data;
            }
        };
        template <class T>
        class S<T, 2>  {
        public:
            static std::string apply(T const &data) {
                return data.content;
            }
        };

        template <class T, int B=(std::is_same_v<T,std::string>?1:(std::is_same_v<T,ByteData>?2:0))>
        class D {};

        template <class T>
        class D<T, 0> {
        public:
            static std::optional<T> apply(std::string const &data) {
                T t;
                if (t.ParseFromString(data)) {
                    return t;
                } else {
                    return std::nullopt;
                }
            }
        };
        template <class T>
        class D<T, 1>  {
        public:
            static std::optional<T> apply(std::string const &data) {
                return {data};
            }
        };
        template <class T>
        class D<T, 2>  {
        public:
            static std::optional<T> apply(std::string const &data) {
                return {T {data}};
            }
        };
    public:
        template <class T>
        static std::string serializeFunc(T const &t) {
            return S<T>::apply(t);
        }
        template <class T>
        static std::optional<T> deserializeFunc(std::string const &data) {
            return { D<T>::apply(data) };
        }
    };

    template <class M>
    class SerializationActions {
    public:
        template <class T>
        static std::string serializeFunc(T const &t) {
            return SerializationFunctions::serializeFunc<T>(t);
        }
        template <class T>
        static std::optional<T> deserializeFunc(std::string const &data) {
            return SerializationFunctions::deserializeFunc<T>(data);
        }
        template <class T, class F>
        static std::shared_ptr<typename M::template Action<T, TypedDataWithTopic<T>>> addTopic(F &&f) {
            return M::template liftPure<T>(
                [f=std::move(f)](T &&data) -> TypedDataWithTopic<T> {
                    return {f(data), std::move(data)};
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<T, TypedDataWithTopic<T>>> addConstTopic(std::string const &s) {
            return addTopic<T>(ConstGenerator<std::string, T>(s));
        }

        template <class T>
        static std::shared_ptr<typename M::template Action<TypedDataWithTopic<T>, ByteDataWithTopic>> serialize() {
            return M::template liftPure<TypedDataWithTopic<T>>(
                [](TypedDataWithTopic<T> &&data) -> ByteDataWithTopic {
                    return {std::move(data.topic), { SerializationFunctions::serializeFunc<T>(data.content) }};
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<typename M::template Key<T>, ByteDataWithID>> serializeWithKey() {
            return M::template liftPure<typename M::template Key<T>>(
                [](typename M::template Key<T> &&data) -> ByteDataWithID {
                    return {M::EnvironmentType::id_to_string(data.id()), { SerializationFunctions::serializeFunc<T>(data.key()) }};
                }
            );
        }
        template <class A, class B>
        static std::shared_ptr<typename M::template Action<typename M::template KeyedData<A,B>, ByteDataWithID>> serializeWithKey() {
            return M::template liftPure<typename M::template KeyedData<A,B>>(
                [](typename M::template KeyedData<A,B> &&data) -> ByteDataWithID {
                    return {M::EnvironmentType::id_to_string(data.key.id()), { SerializationFunctions::serializeFunc<B>(data.data) }};
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<ByteDataWithTopic, TypedDataWithTopic<T>>> deserialize() {
            return M::template liftMaybe<ByteDataWithTopic>(
                [](ByteDataWithTopic &&data) -> std::optional<TypedDataWithTopic<T>> {
                    std::optional<T> t = SerializationFunctions::deserializeFunc<T>(data.content);
                    if (t) {
                        return TypedDataWithTopic<T> {std::move(data.topic), std::move(*t)};
                    } else {
                        return std::nullopt;
                    }
                }
            );
        }
        template <class T>
        static std::shared_ptr<typename M::template Action<ByteDataWithID, typename M::template Key<T>>> deserializeWithKey() {
            return M::template liftMaybe<ByteDataWithID>(
                [](ByteDataWithID &&data) -> std::optional<typename M::template Key<T>> {
                    std::optional<T> t = SerializationFunctions::deserializeFunc<T>(data.content);
                    if (t) {
                        return typename M::template Key<T> {M::EnvironmentType::id_from_string(data.id), std::move(*t)};
                    } else {
                        return std::nullopt;
                    }
                }
            );
        }
    };

} } } } 

namespace std {
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteData const &d) {
        os << "ByteData{length=" << d.content.length() << "}";
        return os; 
    }
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteDataWithTopic const &d) {
        os << "ByteDataWithTopic{topic=" << d.topic << ",length=" << d.content.length() << "}";
        return os; 
    }
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::ByteDataWithID const &d) {
        os << "ByteDataWithID{id=" << d.id << ",length=" << d.content.length() << "}";
        return os; 
    }
    template <class T>
    inline std::ostream &operator<<(std::ostream &os, dev::cd606::tm::basic::TypedDataWithTopic<T> const &d) {
        os << "TypedDataWithTopic{topic=" << d.topic << ",content=" << d.content << "}";
        return os; 
    }
};

#endif
