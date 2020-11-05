using System;
using System.Collections.Generic;
using Here;
using Dev.CD606.TM.Infra;
using PeterO.Cbor;

namespace Dev.CD606.TM.Basic
{
    public static class DataStreamInterface<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
        where GlobalVersion : IComparable 
        where Version : IComparable
    {
        [CborWithFieldNames]
        public class OneFullUpdateItem : GroupedVersionedData<Key,Version,Option<Data>>
        {
            public Key groupID {get; set;}
            public Version version {get; set;}
            public Option<Data> data {get; set;}
        }
        [CborWithFieldNames]
        public class OneDeltaUpdateItem
        {
            public Key key {get; set;}
            public VersionDelta versionDelta {get; set;}
            public DataDelta dataDelta {get; set;}
        }
        public enum OneUpdateItemSubtypes
        {
            OneFullUpdateItem = 0
            , OneDeltaUpdateItem = 1
        }
        [CborUsingCustomMethods]
        public class OneUpdateItem 
        {
            public Variant<OneFullUpdateItem,OneDeltaUpdateItem> theUpdate {get; set;}
            public CBORObject asCborObject()
            {
                return theUpdate.asCborObject();
            }
            public static Option<OneUpdateItem> fromCborObject(CBORObject o)
            {
                var u = Variant<OneFullUpdateItem,OneDeltaUpdateItem>.fromCborObject(o);
                if (u.HasValue)
                {
                    return new OneUpdateItem() {theUpdate = u.Value};
                }
                else
                {
                    return Option.None;
                }
            }
        }
        [CborWithFieldNames]
        public class Update : VersionedData<GlobalVersion, List<OneUpdateItem>>
        {
            public GlobalVersion version {get; set;}
            public List<OneUpdateItem> data {get; set;}
        }
        [CborWithFieldNames]
        public class FullUpdate : VersionedData<GlobalVersion, List<OneFullUpdateItem>>
        {
            public GlobalVersion version {get; set;}
            public List<OneFullUpdateItem> data {get; set;}
        }
        public static Option<FullUpdate> getFullUpdatesOnly(Update u)
        {
            var ret = new FullUpdate {version = u.version, data = new List<OneFullUpdateItem>()};
            foreach (var x in u.data)
            {
                if (x.theUpdate.Index == 0)
                {
                    ret.data.Add(x.theUpdate.Item1.Value);
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
        [CborWithFieldNames]
        public class InsertAction
        {
            public Key key {get; set;}
            public Data data {get; set;}
        }
        [CborWithFieldNames]
        public class UpdateAction
        {
            public Key key {get; set;}
            public Option<VersionSlice> oldVersionSlice {get; set;}
            public Option<DataSummary> oldDataSummary {get; set;}
            public DataDelta dataDelta{get; set;}
        }
        [CborWithFieldNames]
        public class DeleteAction 
        {
            public Key key {get; set;}
            public Option<Version> oldVersion {get; set;}
            public Option<DataSummary> oldDataSummary {get; set;}
        }
        [CborWithFieldNames]
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
        [CborUsingCustomMethods]
        public class Transaction 
        {
            public Variant<InsertAction, UpdateAction, DeleteAction> data {get; set;}
            public CBORObject asCborObject()
            {
                return data.asCborObject();
            }
            public static Option<Transaction> fromCborObject(CBORObject o)
            {
                var d = Variant<InsertAction, UpdateAction, DeleteAction>.fromCborObject(o);
                if (d.HasValue)
                {
                    return new Transaction() {data = d.Value};
                }
                else
                {
                    return Option.None;
                }
            }
        }
    }
    public static class GeneralSubscriber<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
        where GlobalVersion : IComparable
        where Version : IComparable
    {
        [CborWithFieldNames]
        public class Subscription 
        {
            public List<Key> keys {get; set;}
        }
        [CborWithFieldNames]
        public class Unsubscription 
        {
            public string originalSubscriptionID {get; set;}
        }
        [CborWithFieldNames]
        public class ListSubscriptions 
        {
        }
        [CborWithFieldNames]
        public class SubscriptionInfo 
        {
            public List<(string, List<Key>)> subscriptions {get; set;}
        }
        [CborWithFieldNames]
        public class UnsubscribeAll 
        {
        }
        [CborWithFieldNames]
        public class SnapshotRequest 
        {
            public List<Key> keys {get; set;}
        }
        [CborUsingCustomMethods]
        public class SubscriptionUpdate
        {
            public DataStreamInterface<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>.Update update {get; set;}
            public CBORObject asCborObject()
            {
                return CborEncoder<DataStreamInterface<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>.Update>.Encode(update);
            }
            public static Option<SubscriptionUpdate> fromCborObject(CBORObject o)
            {
                var u = CborDecoder<DataStreamInterface<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>.Update>.Decode(o);
                if (u.HasValue)
                {
                    return new SubscriptionUpdate() {update = u.Value};
                }
                else
                {
                    return Option.None;
                }
            }
        }
        
        public enum InputSubtypes 
        {
            Subscription = 0
            , Unsubscription = 1
            , ListSubscriptions = 2
            , UnsubscribeAll = 3
            , SnapshotRequest = 4
        }
        [CborUsingCustomMethods]
        public class Input
        {
            public Variant<Subscription,Unsubscription,ListSubscriptions,UnsubscribeAll,SnapshotRequest> data {get; set;}
            public CBORObject asCborObject()
            {
                return data.asCborObject();
            }
            public static Option<Input> fromCborObject(CBORObject o)
            {
                var d = Variant<Subscription,Unsubscription,ListSubscriptions,UnsubscribeAll,SnapshotRequest>.fromCborObject(o);
                if (d.HasValue)
                {
                    return new Input() {data = d.Value};
                }
                else
                {
                    return Option.None;
                }
            }
        }
        public enum OutputSubtypes 
        {
            Subscription = 0
            , Unsubscription = 1
            , SubscriptionUpdate = 2
            , SubscriptionInfo = 3
            , UnsubscribeAll = 4
        }
        [CborUsingCustomMethods]
        public class Output
        {
            public Variant<Subscription,Unsubscription,SubscriptionUpdate,SubscriptionInfo,UnsubscribeAll> data {get; set;}
            public CBORObject asCborObject()
            {
                return data.asCborObject();
            }
            public static Option<Output> fromCborObject(CBORObject o)
            {
                var d = Variant<Subscription,Unsubscription,SubscriptionUpdate,SubscriptionInfo,UnsubscribeAll>.fromCborObject(o);
                if (d.HasValue)
                {
                    return new Output() {data = d.Value};
                }
                else
                {
                    return Option.None;
                }
            }
        }
    }
}