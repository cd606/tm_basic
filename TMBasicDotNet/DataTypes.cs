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
    [CborUsingCustomMethods]
    public class Variant<A,B>
    {
        private int index = -1;
        private A item1 = default(A);
        private B item2 = default(B);
        public Option<A> Item1
        {
            get
            {
                if (index == 0)
                {
                    return item1;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<B> Item2
        {
            get
            {
                if (index == 1)
                {
                    return item2;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public int Index
        {
            get
            {
                return index;
            }
        }
        private Variant()
        {
        }
        public static Variant<A,B> From1(A a)
        {
            return new Variant<A,B>() {index = 0, item1 = a};
        }
        public static Variant<A,B> From2(B b)
        {
            return new Variant<A,B>() {index = 1, item2 = b};
        }
        public void Dispatch(Action<A> action1, Action<B> action2)
        {
            switch (index)
            {
                case 0:
                    action1(item1);
                    break;
                case 1:
                    action2(item2);
                    break;
                default:
                    break;
            }
        }
        public OutT Dispatch<OutT>(Func<A,OutT> func1, Func<B,OutT> func2)
        {
            switch (index)
            {
                case 0:
                    return func1(item1);
                case 1:
                    return func2(item2);
                default:
                    return default(OutT);
            }
        }
        public CBORObject asCborObject()
        {
            return CBORObject.NewArray()
                .Add(index)
                .Add(index==0?CborEncoder<A>.Encode(item1):CborEncoder<B>.Encode(item2));
        }
        public static Option<Variant<A,B>> fromCborObject(CBORObject obj)
        {
            if (obj.Type != CBORType.Array || obj.Count != 2)
            {
                return Option.None;
            }
            int index = obj[0].AsNumber().ToInt32Checked();
            switch (index)
            {
                case 0:
                    {
                        var item1 = CborDecoder<A>.Decode(obj[1]);
                        if (item1.HasValue)
                        {
                            return From1(item1.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 1:
                    {
                        var item2 = CborDecoder<B>.Decode(obj[1]);
                        if (item2.HasValue)
                        {
                            return From2(item2.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                default:
                    return Option.None;
            }
        }
    }
    [CborUsingCustomMethods]
    public class Variant<A,B,C>
    {
        private int index = -1;
        private A item1 = default(A);
        private B item2 = default(B);
        private C item3 = default(C);
        public Option<A> Item1
        {
            get
            {
                if (index == 0)
                {
                    return item1;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<B> Item2
        {
            get
            {
                if (index == 1)
                {
                    return item2;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<C> Item3
        {
            get
            {
                if (index == 2)
                {
                    return item3;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public int Index
        {
            get
            {
                return index;
            }
        }
        private Variant()
        {
        }
        public static Variant<A,B,C> From1(A a)
        {
            return new Variant<A,B,C>() {index = 0, item1 = a};
        }
        public static Variant<A,B,C> From2(B b)
        {
            return new Variant<A,B,C>() {index = 1, item2 = b};
        }
        public static Variant<A,B,C> From3(C c)
        {
            return new Variant<A,B,C>() {index = 2, item3 = c};
        }
        public void Dispatch(Action<A> action1, Action<B> action2, Action<C> action3)
        {
            switch (index)
            {
                case 0:
                    action1(item1);
                    break;
                case 1:
                    action2(item2);
                    break;
                case 2:
                    action3(item3);
                    break;
                default:
                    break;
            }
        }
        public OutT Dispatch<OutT>(Func<A,OutT> func1, Func<B,OutT> func2, Func<C,OutT> func3)
        {
            switch (index)
            {
                case 0:
                    return func1(item1);
                case 1:
                    return func2(item2);
                case 2:
                    return func3(item3);
                default:
                    return default(OutT);
            }
        }
        public CBORObject asCborObject()
        {
            var ret = CBORObject.NewArray().Add(index);
            switch (index)
            {
                case 0:
                    ret.Add(CborEncoder<A>.Encode(item1));
                    break;
                case 1:
                    ret.Add(CborEncoder<B>.Encode(item2));
                    break;
                case 2:
                    ret.Add(CborEncoder<C>.Encode(item3));
                    break;
                default:
                    break;
            }
            return ret;
        }
        public static Option<Variant<A,B,C>> fromCborObject(CBORObject obj)
        {
            if (obj.Type != CBORType.Array || obj.Count != 2)
            {
                return Option.None;
            }
            int index = obj[0].AsNumber().ToInt32Checked();
            switch (index)
            {
                case 0:
                    {
                        var item1 = CborDecoder<A>.Decode(obj[1]);
                        if (item1.HasValue)
                        {
                            return From1(item1.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 1:
                    {
                        var item2 = CborDecoder<B>.Decode(obj[1]);
                        if (item2.HasValue)
                        {
                            return From2(item2.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 2:
                    {
                        var item3 = CborDecoder<C>.Decode(obj[1]);
                        if (item3.HasValue)
                        {
                            return From3(item3.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                default:
                    return Option.None;
            }
        }
    }
    [CborUsingCustomMethods]
    public class Variant<A,B,C,D>
    {
        private int index = -1;
        private A item1 = default(A);
        private B item2 = default(B);
        private C item3 = default(C);
        private D item4 = default(D);
        public Option<A> Item1
        {
            get
            {
                if (index == 0)
                {
                    return item1;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<B> Item2
        {
            get
            {
                if (index == 1)
                {
                    return item2;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<C> Item3
        {
            get
            {
                if (index == 2)
                {
                    return item3;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<D> Item4
        {
            get
            {
                if (index == 3)
                {
                    return item4;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public int Index
        {
            get
            {
                return index;
            }
        }
        private Variant()
        {
        }
        public static Variant<A,B,C,D> From1(A a)
        {
            return new Variant<A,B,C,D>() {index = 0, item1 = a};
        }
        public static Variant<A,B,C,D> From2(B b)
        {
            return new Variant<A,B,C,D>() {index = 1, item2 = b};
        }
        public static Variant<A,B,C,D> From3(C c)
        {
            return new Variant<A,B,C,D>() {index = 2, item3 = c};
        }
        public static Variant<A,B,C,D> From4(D d)
        {
            return new Variant<A,B,C,D>() {index = 3, item4 = d};
        }
        public void Dispatch(Action<A> action1, Action<B> action2, Action<C> action3, Action<D> action4)
        {
            switch (index)
            {
                case 0:
                    action1(item1);
                    break;
                case 1:
                    action2(item2);
                    break;
                case 2:
                    action3(item3);
                    break;
                case 3:
                    action4(item4);
                    break;
                default:
                    break;
            }
        }
        public OutT Dispatch<OutT>(Func<A,OutT> func1, Func<B,OutT> func2, Func<C,OutT> func3, Func<D,OutT> func4)
        {
            switch (index)
            {
                case 0:
                    return func1(item1);
                case 1:
                    return func2(item2);
                case 2:
                    return func3(item3);
                case 3:
                    return func4(item4);
                default:
                    return default(OutT);
            }
        }
        public CBORObject asCborObject()
        {
            var ret = CBORObject.NewArray().Add(index);
            switch (index)
            {
                case 0:
                    ret.Add(CborEncoder<A>.Encode(item1));
                    break;
                case 1:
                    ret.Add(CborEncoder<B>.Encode(item2));
                    break;
                case 2:
                    ret.Add(CborEncoder<C>.Encode(item3));
                    break;
                case 3:
                    ret.Add(CborEncoder<D>.Encode(item4));
                    break;
                default:
                    break;
            }
            return ret;
        }
        public static Option<Variant<A,B,C,D>> fromCborObject(CBORObject obj)
        {
            if (obj.Type != CBORType.Array || obj.Count != 2)
            {
                return Option.None;
            }
            int index = obj[0].AsNumber().ToInt32Checked();
            switch (index)
            {
                case 0:
                    {
                        var item1 = CborDecoder<A>.Decode(obj[1]);
                        if (item1.HasValue)
                        {
                            return From1(item1.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 1:
                    {
                        var item2 = CborDecoder<B>.Decode(obj[1]);
                        if (item2.HasValue)
                        {
                            return From2(item2.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 2:
                    {
                        var item3 = CborDecoder<C>.Decode(obj[1]);
                        if (item3.HasValue)
                        {
                            return From3(item3.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 3:
                    {
                        var item4 = CborDecoder<D>.Decode(obj[1]);
                        if (item4.HasValue)
                        {
                            return From4(item4.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                default:
                    return Option.None;
            }
        }
    }
    [CborUsingCustomMethods]
    public class Variant<A,B,C,D,E>
    {
        private int index = -1;
        private A item1 = default(A);
        private B item2 = default(B);
        private C item3 = default(C);
        private D item4 = default(D);
        private E item5 = default(E);
        public Option<A> Item1
        {
            get
            {
                if (index == 0)
                {
                    return item1;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<B> Item2
        {
            get
            {
                if (index == 1)
                {
                    return item2;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<C> Item3
        {
            get
            {
                if (index == 2)
                {
                    return item3;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<D> Item4
        {
            get
            {
                if (index == 3)
                {
                    return item4;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public Option<E> Item5
        {
            get
            {
                if (index == 4)
                {
                    return item5;
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public int Index
        {
            get
            {
                return index;
            }
        }
        private Variant()
        {
        }
        public static Variant<A,B,C,D,E> From1(A a)
        {
            return new Variant<A,B,C,D,E>() {index = 0, item1 = a};
        }
        public static Variant<A,B,C,D,E> From2(B b)
        {
            return new Variant<A,B,C,D,E>() {index = 1, item2 = b};
        }
        public static Variant<A,B,C,D,E> From3(C c)
        {
            return new Variant<A,B,C,D,E>() {index = 2, item3 = c};
        }
        public static Variant<A,B,C,D,E> From4(D d)
        {
            return new Variant<A,B,C,D,E>() {index = 3, item4 = d};
        }
        public static Variant<A,B,C,D,E> From5(E e)
        {
            return new Variant<A,B,C,D,E>() {index = 4, item5 = e};
        }
        public void Dispatch(Action<A> action1, Action<B> action2, Action<C> action3, Action<D> action4, Action<E> action5)
        {
            switch (index)
            {
                case 0:
                    action1(item1);
                    break;
                case 1:
                    action2(item2);
                    break;
                case 2:
                    action3(item3);
                    break;
                case 3:
                    action4(item4);
                    break;
                case 4:
                    action5(item5);
                    break;
                default:
                    break;
            }
        }
        public OutT Dispatch<OutT>(Func<A,OutT> func1, Func<B,OutT> func2, Func<C,OutT> func3, Func<D,OutT> func4, Func<E,OutT> func5)
        {
            switch (index)
            {
                case 0:
                    return func1(item1);
                case 1:
                    return func2(item2);
                case 2:
                    return func3(item3);
                case 3:
                    return func4(item4);
                case 4:
                    return func5(item5);
                default:
                    return default(OutT);
            }
        }
        public CBORObject asCborObject()
        {
            var ret = CBORObject.NewArray().Add(index);
            switch (index)
            {
                case 0:
                    ret.Add(CborEncoder<A>.Encode(item1));
                    break;
                case 1:
                    ret.Add(CborEncoder<B>.Encode(item2));
                    break;
                case 2:
                    ret.Add(CborEncoder<C>.Encode(item3));
                    break;
                case 3:
                    ret.Add(CborEncoder<D>.Encode(item4));
                    break;
                case 4:
                    ret.Add(CborEncoder<E>.Encode(item5));
                    break;
                default:
                    break;
            }
            return ret;
        }
        public static Option<Variant<A,B,C,D,E>> fromCborObject(CBORObject obj)
        {
            if (obj.Type != CBORType.Array || obj.Count != 2)
            {
                return Option.None;
            }
            int index = obj[0].AsNumber().ToInt32Checked();
            switch (index)
            {
                case 0:
                    {
                        var item1 = CborDecoder<A>.Decode(obj[1]);
                        if (item1.HasValue)
                        {
                            return From1(item1.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 1:
                    {
                        var item2 = CborDecoder<B>.Decode(obj[1]);
                        if (item2.HasValue)
                        {
                            return From2(item2.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 2:
                    {
                        var item3 = CborDecoder<C>.Decode(obj[1]);
                        if (item3.HasValue)
                        {
                            return From3(item3.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 3:
                    {
                        var item4 = CborDecoder<D>.Decode(obj[1]);
                        if (item4.HasValue)
                        {
                            return From4(item4.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                case 4:
                    {
                        var item5 = CborDecoder<E>.Decode(obj[1]);
                        if (item5.HasValue)
                        {
                            return From5(item5.Value);
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                default:
                    return Option.None;
            }
        }
    }
}