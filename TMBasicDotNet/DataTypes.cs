using System;
using System.Linq;
using System.Collections.Generic;
using PeterO.Cbor;
using Here;

namespace Dev.CD606.TM.Basic
{
    [CborUsingCustomMethods]
    public readonly struct VoidStruct
    {
        public CBORObject asCborObject()
        {
            return CBORObject.FromObject((int) 0);
        }
        public static Option<VoidStruct> fromCborObject(CBORObject obj)
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

    [CborUsingCustomMethods]
    public class ByteData
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

    [CborWithoutFieldNames]
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
    [CborWithoutFieldNames]
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