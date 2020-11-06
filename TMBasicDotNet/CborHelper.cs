using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.IO;
using Here;
using PeterO.Cbor;
using ProtoBuf;

namespace Dev.CD606.TM.Basic
{
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class CborWithFieldNamesAttribute : System.Attribute 
    {
    }
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class CborWithoutFieldNamesAttribute : System.Attribute 
    {
    }
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class CborUsingCustomMethodsAttribute : System.Attribute
    {
    }
    public class CborEncoder<T>
    {
        
        private Func<T,CBORObject> encoder;
        CborEncoder()
        {
            var theType = typeof(T);
            if (theType.Equals(typeof(byte[])))
            {
                this.encoder = (t) => CBORObject.FromObject((byte[]) (object) t);
                return;
            }
            if (theType.Equals(typeof(string)))
            {
                this.encoder = (t) => CBORObject.FromObject((string) (object) t);
                return;
            }
            if (theType.IsEnum)
            {
                this.encoder = (t) => CBORObject.FromObject((int) (object) t);
                return;
            }
            if (theType.IsGenericType && theType.Name.StartsWith("ValueTuple`") && theType.Namespace.Equals("System"))
            {
                var fieldHelpers = theType.GetFields().Select(
                    (f) => {
                        var encoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {f.FieldType});
                        var encoder = encoderType.GetMethod("Encode");
                        return (f, encoder);
                    }
                ).ToArray();
                this.encoder = (t) => {
                    var ret = CBORObject.NewArray();
                    foreach (var f in fieldHelpers)
                    {
                        ret.Add(
                            f.Item2.Invoke(null, new object[] {f.Item1.GetValue(t)})
                        );
                    }
                    return ret;
                };
                return;
            }
            if (theType.GetCustomAttribute(typeof(ProtoContractAttribute), false) != null)
            {
                var encodeMethod = typeof(ProtoBuf.Serializer).GetMethods()
                    .Where(
                        (m) => 
                            m.IsGenericMethod
                            && m.IsPublic
                            && m.Name.Equals("Serialize")
                            && m.GetGenericArguments().Length == 1
                    )
                    .First()
                    .MakeGenericMethod(new Type[] {theType});
                this.encoder = (t) => {
                    var s = new MemoryStream();
                    encodeMethod.Invoke(null, new object[] {s, t});
                    return CBORObject.FromObject(s.ToArray());
                };
                return;
            }
            if (theType.IsGenericType && theType.Name.Equals("Option`1") && theType.Namespace.Equals("Here"))
            {
                var underlyingType = theType.GetGenericArguments()[0];
                var underlyingTypeEncoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {underlyingType});
                var encodeMethod = underlyingTypeEncoderType.GetMethod("Encode");
                var hasValPropMethod = theType.GetProperty("HasValue").GetGetMethod();
                var valPropMethod = theType.GetProperty("Value").GetGetMethod();
                this.encoder = (t) => {
                    if ((bool) hasValPropMethod.Invoke(t, null))
                    {
                        var val = valPropMethod.Invoke(t, null);
                        return CBORObject.NewArray().Add((CBORObject) encodeMethod.Invoke(null, new object[] {val}));
                    }
                    else
                    {
                        return CBORObject.NewArray();
                    }
                };
                return;
            }
            var attrib = theType.GetCustomAttributes(typeof(CborWithFieldNamesAttribute), false);
            if (attrib.Length > 0)
            {
                var properties = theType.GetProperties();
                var l = new List<(string,MethodInfo,MethodInfo)>();
                foreach (var p in properties)
                {
                    var n = p.Name;
                    var vType = p.PropertyType;
                    var getter = p.GetGetMethod();
                    var encoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {vType});
                    var encodeMethod = encoderType.GetMethod("Encode");
                    l.Add((n, getter, encodeMethod));
                }
                var helperArray = l.ToArray();
                this.encoder = (t) => {
                    var ret = CBORObject.NewMap();
                    foreach (var h in helperArray)
                    {
                        ret.Add(h.Item1, h.Item3.Invoke(null, new object[] {h.Item2.Invoke(t, null)}));
                    }
                    return ret;
                };
                return;
            }
            attrib = theType.GetCustomAttributes(typeof(CborWithoutFieldNamesAttribute), false);
            if (attrib.Length > 0)
            {
                var properties = theType.GetProperties();
                var l = new List<(MethodInfo,MethodInfo)>();
                foreach (var p in properties)
                {
                    var vType = p.PropertyType;
                    var getter = p.GetGetMethod();
                    var encoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {vType});
                    var encodeMethod = encoderType.GetMethod("Encode");
                    l.Add((getter, encodeMethod));
                }
                var helperArray = l.ToArray();
                this.encoder = (t) => {
                    var ret = CBORObject.NewArray();
                    foreach (var h in helperArray)
                    {
                        ret.Add(h.Item2.Invoke(null, new object[] {h.Item1.Invoke(t, null)}));
                    }
                    return ret;
                };
                return;
            }
            attrib = theType.GetCustomAttributes(typeof(CborUsingCustomMethodsAttribute), false);
            if (attrib.Length > 0)
            {
                var method = theType.GetMethod("asCborObject");
                this.encoder = (t) => {
                    return (CBORObject) method.Invoke(t, null);
                };
                return;
            }
            var interfaces = theType.GetInterfaces();
            var dictionaryQuery = interfaces.Where(
                (t) => t.IsGenericType && t.Name.Equals("IDictionary`2") && t.Namespace.Equals("System.Collections.Generic")
            ).ToArray();
            if (dictionaryQuery.Length == 1)
            {
                var underlyingKeyType = dictionaryQuery[0].GetGenericArguments()[0];
                var underlyingDataType = dictionaryQuery[0].GetGenericArguments()[1];
                var underlyingKeyTypeEncoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {underlyingKeyType});
                var keyEncodeMethod = underlyingKeyTypeEncoderType.GetMethod("Encode");
                var underlyingDataTypeEncoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {underlyingDataType});
                var dataEncodeMethod = underlyingDataTypeEncoderType.GetMethod("Encode");
                this.encoder = (t) => {
                    var col = (System.Collections.IDictionary) ((object) t);
                    var ret = CBORObject.NewMap();
                    foreach (System.Collections.DictionaryEntry obj in col)
                    {
                        ret.Add(
                            (CBORObject) keyEncodeMethod.Invoke(null, new object[] {obj.Key})
                            , (CBORObject) dataEncodeMethod.Invoke(null, new object[] {obj.Value})
                        );
                    }
                    return ret;
                };
                return;
            }
            var listQuery = interfaces.Where(
                (t) => t.IsGenericType && t.Name.Equals("IList`1") && t.Namespace.Equals("System.Collections.Generic")
            ).ToArray();
            if (listQuery.Length == 1)
            {
                var underlyingType = listQuery[0].GetGenericArguments()[0];
                var underlyingTypeEncoderType = typeof(CborEncoder<>).MakeGenericType(new Type[] {underlyingType});
                var encodeMethod = underlyingTypeEncoderType.GetMethod("Encode");
                this.encoder = (t) => {
                    var col = (System.Collections.IList) ((object) t);
                    var ret = CBORObject.NewArray();
                    foreach (var obj in col)
                    {
                        ret.Add(
                            (CBORObject) encodeMethod.Invoke(null, new object[] {obj})
                        );
                    }
                    return ret;
                };
                return;
            }
            this.encoder = (t) => CBORObject.FromObject(t);
        }
        private static CborEncoder<T> instance = new CborEncoder<T>();
        public static CBORObject Encode(T t)
        {
            return instance.encoder(t);
        }
    }
    public class CborDecoder<T>
    {
        
        private Func<CBORObject,Option<T>> decoder;
        CborDecoder()
        {
            var theType = typeof(T);
            if (theType.Equals(typeof(byte[])))
            {
                this.decoder = (o) => {
                    try 
                    {
                        return o.ToObject<T>();
                    }
                    catch (Exception)
                    {
                        return Option.None;
                    }
                };
                return;
            }
            if (theType.Equals(typeof(string)))
            {
                this.decoder = (o) => {
                    try 
                    {
                        return o.ToObject<T>();
                    }
                    catch (Exception)
                    {
                        return Option.None;
                    }
                };
                return;
            }
            if (theType.IsEnum)
            {
                this.decoder = (o) => {
                    try
                    {
                        var t = o.AsNumber().ToInt32Checked();
                        return Option<T>.Some((T) (object) t);
                    }
                    catch (Exception)
                    {
                        return Option.None;
                    }
                };
                return;
            }
            if (theType.IsGenericType && theType.Name.StartsWith("ValueTuple`") && theType.Namespace.Equals("System"))
            {
                var fieldHelpers = theType.GetFields().Select(
                    (f) => {
                        var decoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {f.FieldType});
                        var decoder = decoderType.GetMethod("Decode");
                        var optionType = typeof(Option<>).MakeGenericType(new Type[] {f.FieldType});
                        var hasValue = optionType.GetProperty("HasValue").GetGetMethod();
                        var value = optionType.GetProperty("Value").GetGetMethod();
                        return (f, decoder, hasValue, value);
                    }
                ).ToArray();
                this.decoder = (o) => {
                    if (o.Type != CBORType.Array || o.Count != fieldHelpers.Length)
                    {
                        return Option.None;
                    }
                    var ret = Activator.CreateInstance(theType);
                    foreach (var f in o.Values.Zip(fieldHelpers, (x,y) => (x,y)))
                    {
                        var item = f.Item2.Item2.Invoke(null, new object[] {f.Item1});
                        if (!((bool) f.Item2.Item3.Invoke(item, null)))
                        {
                            return Option.None;
                        }
                        f.Item2.Item1.SetValue(ret, f.Item2.Item4.Invoke(item, null));
                    }
                    return (T) ret;
                };
                return;
            }
            if (theType.GetCustomAttribute(typeof(ProtoContractAttribute), false) != null)
            {
                var decodeMethod = typeof(ProtoBuf.Serializer).GetMethods()
                    .Where(
                        (m) => 
                            m.IsGenericMethod
                            && m.IsPublic
                            && m.Name.Equals("Deserialize")
                            && m.GetGenericArguments().Length == 1
                    )
                    .First().MakeGenericMethod(new Type[] {theType});
                this.decoder = (o) => {
                    try 
                    {
                        var b = o.ToObject<byte[]>();
                        var ms = new MemoryStream(b);
                        var t = (T) decodeMethod.Invoke(null, new object[] {ms});
                        return t;
                    }
                    catch (Exception)
                    {
                        return Option.None;
                    }
                };
                return;
            }
            if (theType.IsGenericType && theType.Name.Equals("Option`1") && theType.Namespace.Equals("Here"))
            {
                var underlyingType = theType.GetGenericArguments()[0];
                var underlyingTypeDecoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {underlyingType});
                var decodeMethod = underlyingTypeDecoderType.GetMethod("Decode");
                var hasValPropMethod = theType.GetProperty("HasValue").GetGetMethod();
                var valPropMethod = theType.GetProperty("Value").GetGetMethod();
                var underlyingNone = (T) theType.GetField("None").GetValue(null);
                this.decoder = (o) => {
                    if (o.Type != CBORType.Array)
                    {
                        return Option<T>.None;
                    }
                    if (o.Count == 0)
                    {
                        return Option<T>.Some(underlyingNone);
                    }
                    else if (o.Count == 1)
                    {
                        try
                        {
                            var inner = decodeMethod.Invoke(null, new object[] {o[0]});
                            return Option<T>.Some((T) inner);
                        }
                        catch (Exception)
                        {
                            return Option<T>.None;
                        }
                    }
                    else
                    {
                        return Option<T>.None;
                    }
                };
                return;
            }
            var attrib = theType.GetCustomAttributes(typeof(CborWithFieldNamesAttribute), false);
            if (attrib.Length > 0)
            {
                var properties = theType.GetProperties();
                var l = new List<(string,(MethodInfo,MethodInfo,MethodInfo,MethodInfo))>();
                foreach (var p in properties)
                {
                    var n = p.Name;
                    var vType = p.PropertyType;
                    var setter = p.GetSetMethod();
                    var decoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {vType});
                    var decodeMethod = decoderType.GetMethod("Decode");
                    var optType = typeof(Option<>).MakeGenericType(new Type[] {vType});
                    var hasValPropMethod = optType.GetProperty("HasValue").GetGetMethod();
                    var valPropMethod = optType.GetProperty("Value").GetGetMethod();
                    l.Add((n, (setter, decodeMethod, hasValPropMethod, valPropMethod)));
                }
                var helperMap = l.ToDictionary(
                    keySelector : (x) => x.Item1
                    , elementSelector : (x) => x.Item2
                );
                this.decoder = (o) => {
                    if (o.Type != CBORType.Map)
                    {
                        return Option.None;
                    }
                    if (o.Count != helperMap.Count)
                    {
                        return Option.None;
                    }
                    T ret = (T) Activator.CreateInstance(theType);
                    foreach (var item in o.Entries)
                    {
                        var name = item.Key.AsString();
                        if (!helperMap.TryGetValue(name, out var val))
                        {
                            return Option.None;
                        }
                        var data = val.Item2.Invoke(null, new object[] {item.Value});
                        if (!((bool) val.Item3.Invoke(data, null)))
                        {
                            return Option.None;
                        }
                        val.Item1.Invoke(ret, new object[] {val.Item4.Invoke(data,null)});
                    }
                    return ret;
                };
                return;
            }
            attrib = theType.GetCustomAttributes(typeof(CborWithoutFieldNamesAttribute), false);
            if (attrib.Length > 0)
            {
                var properties = theType.GetProperties();
                var l = new List<(MethodInfo,MethodInfo,MethodInfo,MethodInfo)>();
                foreach (var p in properties)
                {
                    var vType = p.PropertyType;
                    var setter = p.GetSetMethod();
                    var decoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {vType});
                    var decodeMethod = decoderType.GetMethod("Decode");
                    var optType = typeof(Option<>).MakeGenericType(new Type[] {vType});
                    var hasValPropMethod = optType.GetProperty("HasValue").GetGetMethod();
                    var valPropMethod = optType.GetProperty("Value").GetGetMethod();
                    l.Add((setter, decodeMethod, hasValPropMethod, valPropMethod));
                }
                var helperArray = l.ToArray();
                this.decoder = (o) => {
                    if (o.Type != CBORType.Array)
                    {
                        return Option.None;
                    }
                    if (o.Count != helperArray.Length)
                    {
                        return Option.None;
                    }
                    T ret = (T) Activator.CreateInstance(theType);
                    foreach (var item in o.Values.Zip(helperArray, (x, h) => (x, h)))
                    {
                        var data = item.Item2.Item2.Invoke(null, new object[] {item.Item1});
                        if (!((bool) item.Item2.Item3.Invoke(data, null)))
                        {
                            return Option.None;
                        }
                        item.Item2.Item1.Invoke(ret, new object[] {item.Item2.Item4.Invoke(data, null)});
                    }
                    return ret;
                };
                return;
            }
            attrib = theType.GetCustomAttributes(typeof(CborUsingCustomMethodsAttribute), false);
            if (attrib.Length > 0)
            {
                var method = theType.GetMethod("fromCborObject");
                this.decoder = (t) => {
                    return (Option<T>) method.Invoke(null, new object[] {t});
                };
                return;
            }
            var interfaces = theType.GetInterfaces();
            var dictionaryQuery = interfaces.Where(
                (t) => t.IsGenericType && t.Name.Equals("IDictionary`2") && t.Namespace.Equals("System.Collections.Generic")
            ).ToArray();
            if (dictionaryQuery.Length == 1)
            {
                var underlyingKeyType = dictionaryQuery[0].GetGenericArguments()[0];
                var underlyingDataType = dictionaryQuery[0].GetGenericArguments()[1];
                var underlyingKeyTypeDecoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {underlyingKeyType});
                var keyDecodeMethod = underlyingKeyTypeDecoderType.GetMethod("Decode");
                var underlyingDataTypeDecoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {underlyingDataType});
                var dataDecodeMethod = underlyingDataTypeDecoderType.GetMethod("Decode");
                var keyOptType = typeof(Option<>).MakeGenericType(new Type[] {underlyingKeyType});
                var keyHasValPropMethod = keyOptType.GetProperty("HasValue").GetGetMethod();
                var keyValPropMethod = keyOptType.GetProperty("Value").GetGetMethod();
                var dataOptType = typeof(Option<>).MakeGenericType(new Type[] {underlyingDataType});
                var dataHasValPropMethod = dataOptType.GetProperty("HasValue").GetGetMethod();
                var dataValPropMethod = dataOptType.GetProperty("Value").GetGetMethod();
                this.decoder = (o) => {
                    if (o.Type != CBORType.Map)
                    {
                        return Option.None;
                    }
                    T ret = (T) Activator.CreateInstance(theType);
                    foreach (var item in o.Entries)
                    {
                        var key = keyDecodeMethod.Invoke(null, new object[] {item.Key});
                        if (!((bool) keyHasValPropMethod.Invoke(key, null)))
                        {
                            return Option.None;
                        }
                        var data = dataDecodeMethod.Invoke(null, new object[] {item.Value});
                        if (!((bool) dataHasValPropMethod.Invoke(data, null)))
                        {
                            return Option.None;
                        }
                        ((System.Collections.IDictionary) (object) ret).Add(
                            keyValPropMethod.Invoke(key, null)
                            , dataValPropMethod.Invoke(data, null)
                        );
                    }
                    return ret;
                };
                return;
            }
            var listQuery = interfaces.Where(
                (t) => t.IsGenericType && t.Name.Equals("IList`1") && t.Namespace.Equals("System.Collections.Generic")
            ).ToArray();
            if (listQuery.Length == 1)
            {
                var underlyingType = listQuery[0].GetGenericArguments()[0];
                var underlyingTypeDecoderType = typeof(CborDecoder<>).MakeGenericType(new Type[] {underlyingType});
                var decodeMethod = underlyingTypeDecoderType.GetMethod("Decode");
                var optType = typeof(Option<>).MakeGenericType(new Type[] {underlyingType});
                var hasValPropMethod = optType.GetProperty("HasValue").GetGetMethod();
                var valPropMethod = optType.GetProperty("Value").GetGetMethod();
                var listType = typeof(List<>).MakeGenericType(new Type[] {underlyingType});
                var toArrayMethod = listType.GetMethod("ToArray");
                this.decoder = (o) => {
                    if (o.Type != CBORType.Array)
                    {
                        return Option.None;
                    }
                    if (theType.IsArray)
                    {
                        var retList = Activator.CreateInstance(listType);
                        foreach (var item in o.Values)
                        {
                            var data = decodeMethod.Invoke(null, new object[] {item});
                            if (!((bool) hasValPropMethod.Invoke(data, null)))
                            {
                                return Option.None;
                            }
                            ((System.Collections.IList) retList).Add(valPropMethod.Invoke(data, null));
                        }
                        return (T) toArrayMethod.Invoke(retList, null);
                    }
                    else
                    {
                        T ret = (T) Activator.CreateInstance(theType);
                        foreach (var item in o.Values)
                        {
                            var data = decodeMethod.Invoke(null, new object[] {item});
                            if (!((bool) hasValPropMethod.Invoke(data, null)))
                            {
                                return Option.None;
                            }
                            ((System.Collections.IList) (object) ret).Add(valPropMethod.Invoke(data, null));
                        }
                        return ret;
                    }
                };
                return;
            }
            this.decoder = (o) => {
                try 
                {
                    var t = o.ToObject<T>();
                    return Option<T>.Some(t);
                }
                catch (Exception)
                {
                    return Option.None;
                }
            };
        }
        private static CborDecoder<T> instance = new CborDecoder<T>();
        public static Option<T> Decode(CBORObject o)
        {
            return instance.decoder(o);
        }
    }
}