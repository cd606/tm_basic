using System;
using System.Collections.Generic;
using System.Linq;
using Here;
using Dev.CD606.TM.Infra;
using PeterO.Cbor;

namespace Dev.CD606.TM.Basic
{
    public static class DataStreamInterface<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
        where GlobalVersion : IComparable 
        where Version : IComparable
    {
        public class OneFullUpdateItem : GroupedVersionedData<Key,Version,Option<Data>>, ToCbor, FromCbor<OneFullUpdateItem>
        {
            public Key groupID {get; set;}
            public Version version {get; set;}
            public Option<Data> data {get; set;}
            public CBORObject asCborObject()
            {
                return CBORObject.NewMap()
                    .Add("groupID", TMCborEncoder<Key>.Instance.encode(groupID))
                    .Add("version", TMCborEncoder<Version>.Instance.encode(version))
                    .Add("data", TMCborEncoder<Data>.Instance.encode(data));
            }
            public Option<OneFullUpdateItem> fromCborObject(CBORObject o)
            {
                if (o.Type != CBORType.Map)
                {
                    return Option.None;
                }
                int toFind = 3;
                var ret = new OneFullUpdateItem();
                var entries = o.Entries;
                if (entries.Count != 3)
                {
                    return Option.None;
                }
                foreach (var item in entries)
                {
                    if (item.Key.Type != CBORType.TextString)
                    {
                        return Option.None;
                    }
                    var keyStr = item.Key.AsString();
                    switch (keyStr)
                    {
                        case "groupID":
                        {
                            var g = TMCborDecoder<Key>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.groupID = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                        case "version":
                        {
                            var g = TMCborDecoder<Version>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.version = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                        case "data":
                        {
                            var g = TMCborDecoder<Data>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.data = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                    }
                }
                if (toFind != 0)
                {
                    return Option.None;
                }
                return ret;
            }
        }
        public class OneDeltaUpdateItem : ToCbor, FromCbor<OneDeltaUpdateItem>
        {
            public Key key {get; set;}
            public VersionDelta versionDelta {get; set;}
            public DataDelta dataDelta {get; set;}
            public CBORObject asCborObject()
            {
                return CBORObject.NewMap()
                    .Add("key", TMCborEncoder<Key>.Instance.encode(key))
                    .Add("versionDelta", TMCborEncoder<VersionDelta>.Instance.encode(versionDelta))
                    .Add("dataDelta", TMCborEncoder<DataDelta>.Instance.encode(dataDelta));
            }
            public Option<OneDeltaUpdateItem> fromCborObject(CBORObject o)
            {
                if (o.Type != CBORType.Map)
                {
                    return Option.None;
                }
                int toFind = 3;
                var ret = new OneDeltaUpdateItem();
                var entries = o.Entries;
                if (entries.Count != 3)
                {
                    return Option.None;
                }
                foreach (var item in entries)
                {
                    if (item.Key.Type != CBORType.TextString)
                    {
                        return Option.None;
                    }
                    var keyStr = item.Key.AsString();
                    switch (keyStr)
                    {
                        case "key":
                        {
                            var g = TMCborDecoder<Key>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.key = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                        case "versionDelta":
                        {
                            var g = TMCborDecoder<VersionDelta>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.versionDelta = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                        case "dataDelta":
                        {
                            var g = TMCborDecoder<DataDelta>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.dataDelta = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                    }
                }
                if (toFind != 0)
                {
                    return Option.None;
                }
                return ret;
            }
        }
        public enum OneUpdateItemSubtypes
        {
            OneFullUpdateItem = 0
            , OneDeltaUpdateItem = 1
        }
        public class OneUpdateItem : ToCbor, FromCbor<OneUpdateItem>
        {
            public Either<OneFullUpdateItem, OneDeltaUpdateItem> theUpdate {get; set;}
            public CBORObject asCborObject()
            {
                return theUpdate.Match<CBORObject>(
                    onLeft : (OneFullUpdateItem x) => {
                        return CBORObject.NewArray().Add(CBORObject.FromObject(0)).Add(x.asCborObject());
                    }
                    , onRight : (OneDeltaUpdateItem x) => {
                        return CBORObject.NewArray().Add(CBORObject.FromObject(1)).Add(x.asCborObject());
                    }
                );
            }
            public Option<OneUpdateItem> fromCborObject(CBORObject obj)
            {
                if (obj.Type != CBORType.Array)
                {
                    return Option.None;
                }
                var vals = obj.Values;
                if (vals.Count != 2)
                {
                    return Option.None;
                }
                if (vals.First().Type != CBORType.Integer)
                {
                    return Option.None;
                }
                int which = vals.First().ToObject<int>();
                switch (which)
                {
                    case 0:
                    {
                        var x = TMCborDecoder<OneFullUpdateItem>.Instance.decode(vals.ElementAt(1));
                        if (x.HasValue)
                        {
                            return new OneUpdateItem {theUpdate = Either.Left(x.Value)};
                        }
                        else
                        {
                            return Option.None;
                        }
                    }
                    case 1:
                    {
                        var x = TMCborDecoder<OneDeltaUpdateItem>.Instance.decode(vals.ElementAt(1));
                        if (x.HasValue)
                        {
                            return new OneUpdateItem {theUpdate = Either.Right(x.Value)};
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
        public class Update : VersionedData<GlobalVersion, List<OneUpdateItem>>, ToCbor, FromCbor<Update>
        {
            public GlobalVersion version {get; set;}
            public List<OneUpdateItem> data {get; set;}
            public CBORObject asCborObject()
            {
                return CBORObject.NewMap()
                    .Add("version", TMCborEncoder<GlobalVersion>.Instance.encode(version))
                    .Add("data", TMCborEncoder<OneUpdateItem>.Instance.encode(data));
            }
            public Option<Update> fromCborObject(CBORObject obj)
            {
                if (obj.Type != CBORType.Map)
                {
                    return Option.None;
                }
                int toFind = 2;
                var ret = new Update();
                var entries = obj.Entries;
                if (entries.Count != 2)
                {
                    return Option.None;
                }
                foreach (var item in entries)
                {
                    if (item.Key.Type != CBORType.TextString)
                    {
                        return Option.None;
                    }
                    var keyStr = item.Key.AsString();
                    switch (keyStr)
                    {
                        case "version":
                        {
                            var g = TMCborDecoder<GlobalVersion>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.version = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                        case "data":
                        {
                            var g = TMCborDecoder<OneUpdateItem>.Instance.decodeList(item.Value);
                            if (g.HasValue)
                            {
                                ret.data = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                    }
                }
                if (toFind != 0)
                {
                    return Option.None;
                }
                return ret;
            }
        }
        public class FullUpdate : VersionedData<GlobalVersion, List<OneFullUpdateItem>>, ToCbor, FromCbor<FullUpdate>
        {
            public GlobalVersion version {get; set;}
            public List<OneFullUpdateItem> data {get; set;}
            public CBORObject asCborObject()
            {
                return CBORObject.NewMap()
                    .Add("version", TMCborEncoder<GlobalVersion>.Instance.encode(version))
                    .Add("data", TMCborEncoder<OneFullUpdateItem>.Instance.encode(data));
            }
            public Option<FullUpdate> fromCborObject(CBORObject obj)
            {
                if (obj.Type != CBORType.Map)
                {
                    return Option.None;
                }
                int toFind = 2;
                var ret = new FullUpdate();
                var entries = obj.Entries;
                if (entries.Count != 2)
                {
                    return Option.None;
                }
                foreach (var item in entries)
                {
                    if (item.Key.Type != CBORType.TextString)
                    {
                        return Option.None;
                    }
                    var keyStr = item.Key.AsString();
                    switch (keyStr)
                    {
                        case "version":
                        {
                            var g = TMCborDecoder<GlobalVersion>.Instance.decode(item.Value);
                            if (g.HasValue)
                            {
                                ret.version = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                        case "data":
                        {
                            var g = TMCborDecoder<OneFullUpdateItem>.Instance.decodeList(item.Value);
                            if (g.HasValue)
                            {
                                ret.data = g.Value;
                                --toFind;
                            }
                            else
                            {
                                return Option.None;
                            }
                        }
                        break;
                    }
                }
                if (toFind != 0)
                {
                    return Option.None;
                }
                return ret;
            }
        }
        public static Option<FullUpdate> getFullUpdatesOnly(Update u)
        {
            var ret = new FullUpdate {version = u.version, data = new List<OneFullUpdateItem>()};
            foreach (var x in u.data)
            {
                if (x.theUpdate.IsLeft)
                {
                    ret.data.Add(x.theUpdate.LeftValue);
                }
            }
            if (ret.data.Count > 0)
            {
                return ret;
            }
            else
            {
                return Option<FullUpdate>.None;
            }
        }
    }

    public static class TransactionInterface<GlobalVersion,Key,Version,Data,DataSummary,VersionSlice,DataDelta>
        where GlobalVersion : IComparable
        where Version : IComparable
    {
        public enum RequestDecision 
        {
            Success = 0
            , FailurePrecondition = 1
            , FailurePermission = 2
            , FailureConsistency = 3
        };
        public class InsertAction
        {
            public Key key {get; set;}
            public Data data {get; set;}
        }
        public class UpdateAction
        {
            public Key key {get; set;}
            public Option<VersionSlice> oldVersionSlice {get; set;}
            public Option<DataSummary> oldDataSummary {get; set;}
            public DataDelta dataDelta{get; set;}
        }
        public class DeleteAction 
        {
            public Key key {get; set;}
            public Option<Version> oldVersion {get; set;}
            public Option<DataSummary> oldDataSummary {get; set;}
        }
        public class TransactionResponse 
        {
            public GlobalVersion globalVersion {get; set;}
            public RequestDecision requestDecision {get; set;}
        }
        public enum TransactionSubtypes 
        {
            InsertAction = 0
            , UpdateAction = 1
            , DeleteAction = 2
        }
        public class Transaction 
        {
            public Either<InsertAction, Either<UpdateAction, DeleteAction>> data {get; set;}
        }
    }
    public static class GeneralSubscriber<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
        where GlobalVersion : IComparable
        where Version : IComparable
    {
        public class Subscription 
        {
            public List<Key> keys {get; set;}
        }
        public class Unsubscription 
        {
            public string originalSubscriptionID {get; set;}
        }
        public class ListSubscriptions 
        {
        }
        public class SubscriptionInfo 
        {
            public List<(string, List<Key>)> subscriptions {get; set;}
        }
        public class UnsubscribeAll 
        {
        }
        public class SnapshotRequest 
        {
            public List<Key> keys {get; set;}
        }
        public class SubscriptionUpdate : DataStreamInterface<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>.Update
        {
        }
        
        public enum InputSubtypes 
        {
            Subscription = 0
            , Unsubscription = 1
            , ListSubscriptions = 2
            , UnsubscribeAll = 3
            , SnapshotRequest = 4
        }
        public class Input
        {
            public Either<
                Subscription
                , Either<
                    Unsubscription
                    , Either<
                        ListSubscriptions
                        , Either<
                            UnsubscribeAll
                            , SnapshotRequest
                        >
                    >
                >
            > input {get; set;}
        }
        public enum OutputSubtypes 
        {
            Subscription = 0
            , Unsubscription = 1
            , SubscriptionUpdate = 2
            , SubscriptionInfo = 3
            , UnsubscribeAll = 4
        }
        public class Output
        {
            public Either<
                Subscription
                , Either<
                    Unsubscription
                    , Either<
                        SubscriptionUpdate
                        , Either<
                            SubscriptionInfo
                            , UnsubscribeAll
                        >
                    >
                >
            > output {get; set;}
        }
    }
}