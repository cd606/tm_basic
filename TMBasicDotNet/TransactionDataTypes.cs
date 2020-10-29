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
        public class OneFullUpdateItem : GroupedVersionedData<Key,Version,Option<Data>>
        {
            public Key groupID {get; set;}
            public Version version {get; set;}
            public Option<Data> data {get; set;}
        }
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
        public class OneUpdateItem 
        {
            public Either<OneFullUpdateItem, OneDeltaUpdateItem> theUpdate {get; set;}
        }
        public class Update : VersionedData<GlobalVersion, List<OneUpdateItem>>
        {
            public GlobalVersion version {get; set;}
            public List<OneUpdateItem> data {get; set;}
        }
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