using System;
using System.Linq;
using System.Collections.Generic;
using PeterO.Cbor;
using Here;

namespace Dev.CD606.TM.Basic
{
    public interface ToCbor
    {
        CBORObject asCborObject();
    }
    public interface FromCbor<T>
    {
        Option<T> fromCborObject(CBORObject obj);
    }
    public class TMCborEncoder<T>
    {
        private Func<T,CBORObject> encoder;
        private TMCborEncoder()
        {
            var tType = typeof(T);
            if (tType.IsInterface && tType.Name.Equals("ToCbor") && tType.Namespace.Equals("Dev.CD606.TM.Basic"))
            {
                var method = tType.GetMethod("asCborObject");
                encoder = (T t) => {
                    return (CBORObject) method.Invoke(t, null);
                };
            }
            else if (tType.GetInterfaces().Where(
                (Type interf) => interf.Name.Equals("ToCbor") && interf.Namespace.Equals("Dev.CD606.TM.Basic")
            ).Count() > 0)
            {
                var method = tType.GetMethod("asCborObject");
                encoder = (T t) => {
                    return (CBORObject) method.Invoke(t, null);
                };
            }
            else
            {
                encoder = (T t) => {
                    return CBORObject.FromObject((object) t);
                };
            }
        }
        private static TMCborEncoder<T> instance = new TMCborEncoder<T>();
        public static TMCborEncoder<T> Instance
        {
            get
            {
                return instance;
            }
        }
        public CBORObject encode(T t)
        {
            return encoder(t);
        }
        public CBORObject encode(Option<T> t)
        {
            if (t.HasNoValue)
            {
                return CBORObject.NewArray();
            }
            else
            {
                return CBORObject.NewArray().Add(encode(t.Unwrap()));
            }
        }
        public CBORObject encode(List<T> t)
        {
            var ret = CBORObject.NewArray();
            foreach (var x in t)
            {
                ret.Add(encode(x));
            }
            return ret;
        }
    }
    public class TMCborDecoder<T>
    {
        private Func<CBORObject,Option<T>> decoder;
        private T dummy;
        private TMCborDecoder(T dummy = default(T))
        {
            this.dummy = dummy;
            var tType = typeof(T);
            var constructor = tType.GetConstructor(Type.EmptyTypes);
            if (constructor != null)
            {
                this.dummy = (T) constructor.Invoke(null);
            }
            if (tType.IsInterface && tType.Name.Equals("FromCbor`1") && tType.Namespace.Equals("Dev.CD606.TM.Basic"))
            {
                var method = tType.GetMethod("fromCborObject");
                decoder = (CBORObject o) => {
                    return (Option<T>) method.Invoke(dummy, new object[] {o});
                };
            }
            else if (tType.GetInterfaces().Where(
                (Type interf) => interf.Name.Equals("FromCbor`1") && interf.Namespace.Equals("Dev.CD606.TM.Basic")
            ).Count() > 0)
            {
                var method = tType.GetMethod("fromCborObject");
                decoder = (CBORObject o) => {
                    return (Option<T>) method.Invoke(dummy, new object[] {o});
                };
            }
            else
            {
                decoder = (CBORObject o) => {
                    return o.ToObject<T>();
                };
            }
        }
        private static TMCborDecoder<T> instance = new TMCborDecoder<T>();
        public static TMCborDecoder<T> Instance
        {
            get
            {
                return instance;
            }
        }
        public Option<T> decode(CBORObject obj)
        {
            return decoder(obj);
        }
        public Option<Option<T>> decodeOption(CBORObject o)
        {
            if (o.Type != CBORType.Array)
            {
                return Option<Option<T>>.None;
            }
            var v = o.Values;
            if (v.Count == 0)
            {
                return Option<T>.None;
            }
            return decode(o);
        }
        public Option<List<T>> decodeList(CBORObject o)
        {
            if (o.Type != CBORType.Array)
            {
                return Option.None;
            }
            var v = o.Values;
            var ret = new List<T>();
            foreach (var item in v)
            {
                var t = decode(item);
                if (t.HasValue)
                {
                    ret.Add(t.Unwrap());
                }
                else
                {
                    return Option.None;
                }
            }
            return ret;
        }
    }
    public readonly struct VoidStruct : ToCbor, FromCbor<VoidStruct>
    {
        public CBORObject asCborObject()
        {
            return CBORObject.FromObject((int) 0);
        }
        public Option<VoidStruct> fromCborObject(CBORObject obj)
        {
            if (obj.IsNumber)
            {
                if (obj.AsNumber().IsZero())
                {
                    return new VoidStruct();
                }
                else
                {
                    return Option.None;
                }
            }
            else
            {
                return Option.None;
            }
        }
    }

    public class ByteData : ToCbor, FromCbor<ByteData>
    {
        public readonly byte[] content;
        public ByteData(byte[] content)
        {
            this.content = content;
        } 
        public CBORObject asCborObject()
        {
            return CBORObject.FromObject(content);
        }
        public Option<ByteData> fromCborObject(CBORObject obj)
        {
            if (obj.Type == CBORType.ByteString)
            {
                return new ByteData(obj.ToObject<byte[]>());
            }
            else
            {
                return Option.None;
            }
        }
    }

    public class ByteDataWithTopic
    {
        public readonly string topic;
        public readonly byte[] content;
        public ByteDataWithTopic(string topic, byte[] content)
        {
            this.topic = topic;
            this.content = content;
        }
    }
    public class TypedDataWithTopic<T>
    {
        public readonly string topic;
        public readonly T content;
        public TypedDataWithTopic(string topic, T content)
        {
            this.topic = topic;
            this.content = content;
        }
    }
}