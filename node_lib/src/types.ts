import type * as TMInfra from '@dev_cd606/tm_infra';

export interface ClockSettings {
    synchronizationPointActual: Date;
    synchronizationPointVirtual: Date;
    speed: number;
}
export interface ClockSettingsDescription {
    clock_settings: {
        actual: {
            time: string; //"HH:MM"
        }
        , virtual: {
            date: string; //"YYYY-mm-dd"
            time: string; //"HH:MM"
        }
        , speed: number;
    }
}
export interface ClockOnOrderFacilityInput<S> {
    inputData: S;
    callbackDurations: number[];
}
export type Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data> = TMInfra.GroupedVersionedData<Key, Version, ([] | [Data])>;
export type Transaction_DataStreamInterface_OneDeltaUpdateItem<Key, VersionDelta, DataDelta> = [Key, VersionDelta | null, DataDelta];
export enum Transaction_DataStreamInterface_OneUpdateItemSubtypes {
    OneFullUpdateItem = 0
    , OneDeltaUpdateItem = 1
}
export type Transaction_DataStreamInterface_OneUpdateItem<Key, Version, Data, VersionDelta, DataDelta> =
    TMInfra.Either2<
        Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>
        , Transaction_DataStreamInterface_OneDeltaUpdateItem<Key, VersionDelta, DataDelta>
    >;
export type Transaction_DataStreamInterface_Update<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta> =
    TMInfra.VersionedData<
        GlobalVersion
        , Transaction_DataStreamInterface_OneUpdateItem<Key, Version, Data, VersionDelta, DataDelta>[]
    >;
export type Transaction_DataStreamInterface_FullUpdate<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta> =
    TMInfra.VersionedData<
        GlobalVersion
        , Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>[]
    >;
export enum TransactionInterface_RequestDecision {
    Success = 0
    , FailurePrecondition = 1
    , FailurePermission = 2
    , FailureConsistency = 3
};
export interface TransactionInterface_InsertAction<Key, Data> {
    key: Key;
    data: Data;
}
export interface TransactionInterface_UpdateAction<Key, VersionSlice, DataSummary, DataDelta> {
    key: Key;
    oldVersionSlice: [] | [VersionSlice];
    oldDataSummary: [] | [DataSummary];
    dataDelta: DataDelta;
}
export interface TransactionInterface_DeleteAction<Key, Version, DataSummary> {
    key: Key;
    oldVersion: [] | [Version];
    oldDataSummary: [] | [DataSummary];
}
export interface TransactionInterface_TransactionResponse<GlobalVersion> {
    globalVersion: GlobalVersion;
    requestDecision: TransactionInterface_RequestDecision;
}
export enum TransactionInterface_TransactionSubtypes {
    InsertAction = 0
    , UpdateAction = 1
    , DeleteAction = 2
}
export type TransactionInterface_Transaction<Key, Version, Data, DataSummary, VersionSlice, DataDelta> =
    TMInfra.Either3<
        TransactionInterface_InsertAction<Key, Data>
        , TransactionInterface_UpdateAction<Key, VersionSlice, DataSummary, DataDelta>
        , TransactionInterface_DeleteAction<Key, Version, DataSummary>
    >;
export type TransactionInterface_Account = string;
export type TransactionInterface_TransactionWithAccountInfo<Key, Version, Data, DataSummary, VersionSlice, DataDelta> =
    [TransactionInterface_Account, TransactionInterface_Transaction<Key, Version, Data, DataSummary, VersionSlice, DataDelta>];
export interface GeneralSubscriber_Subscription<Key> {
    keys: Key[];
}
export interface GeneralSubscriber_Unsubscription {
    originalSubscriptionID: string;
}
export interface GeneralSubscriber_ListSubscriptions { };
export interface GeneralSubscriber_SubscriptionInfo<Key> {
    subscriptions: [string, Key[]][];
}
export interface GeneralSubscriber_UnsubscribeAll { };
export interface GeneralSubscriber_SnapshotRequest<Key> {
    keys: Key[];
}
export type GeneralSubscriber_SubscriptionUpdate<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>
    = Transaction_DataStreamInterface_Update<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>;

export enum GeneralSubscriber_InputSubtypes {
    Subscription = 0
    , Unsubscription = 1
    , ListSubscriptions = 2
    , UnsubscribeAll = 3
    , SnapshotRequest = 4
}
export type GeneralSubscriber_Input<Key> = TMInfra.Either5<
    GeneralSubscriber_Subscription<Key>, GeneralSubscriber_Unsubscription, GeneralSubscriber_ListSubscriptions, GeneralSubscriber_UnsubscribeAll, GeneralSubscriber_SnapshotRequest<Key>
>;
export enum GeneralSubscriber_OutputSubtypes {
    Subscription = 0
    , Unsubscription = 1
    , SubscriptionUpdate = 2
    , SubscriptionInfo = 3
    , UnsubscribeAll = 4
}
export type GeneralSubscriber_Output<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta> = TMInfra.Either5<
    GeneralSubscriber_Subscription<Key>, GeneralSubscriber_Unsubscription, GeneralSubscriber_SubscriptionUpdate<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>, GeneralSubscriber_SubscriptionInfo<Key>, GeneralSubscriber_UnsubscribeAll
>;

export type VoidStruct = 0;

export type ByteData = Buffer;
export interface ByteDataWithTopic {
    topic: string;
    content: ByteData;
}
export interface TypedDataWithTopic<T> {
    topic: string;
    content: T;
}

export interface Files_TopicCaptureFileRecord {
    time: Date
    , timeString: string
    , topic: string
    , data: Buffer
    , isFinal: boolean
}
export interface Files_TopicCaptureFileRecordReaderOption {
    fileMagicLength: number
    , recordMagicLength: number
    , timeFieldLength: number
    , topicLengthFieldLength: number
    , dataLengthFieldLength: number
    , hasFinalFlagField: boolean
    , timePrecision: "second" | "millisecond" | "microsecond"
}