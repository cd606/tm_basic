import * as TMInfra from '../../tm_infra/node_lib/TMInfra'
import * as _ from 'lodash';
import * as dateformat from 'dateformat'
import * as fs from 'fs'
import * as Stream from 'stream'
import { Console } from 'console';

export interface ClockSettings {
    synchronizationPointActual : Date;
    synchronizationPointVirtual : Date;
    speed : number;
}
export interface ClockSettingsDescription {
    clock_settings : {
        actual : {
            time : string; //"HH:MM"
        }
        , virtual : {
            date : string; //"YYYY-mm-dd"
            time : string; //"HH:MM"
        }
        , speed : number; 
    }
}
export function parseClockSettingsDescription(d : ClockSettingsDescription) : ClockSettings {
    if (!/^\d{2}:\d{2}$/.test(d.clock_settings.actual.time)) {
        return null;
    }
    if (!/^\d{4}-\d{2}-\d{2}$/.test(d.clock_settings.virtual.date)) {
        return null;
    }
    if (!/^\d{2}:\d{2}$/.test(d.clock_settings.virtual.time)) {
        return null;
    }
    let actual = new Date();
    actual.setHours(parseInt(d.clock_settings.actual.time.substring(0,2)));
    actual.setMinutes(parseInt(d.clock_settings.actual.time.substring(3,5)));
    actual.setSeconds(0);
    actual.setMilliseconds(0);
    let virtual = new Date(
        parseInt(d.clock_settings.virtual.date.substring(0,4))
        , parseInt(d.clock_settings.virtual.date.substring(5,7))
        , parseInt(d.clock_settings.virtual.date.substring(8,10))
    );
    virtual.setHours(parseInt(d.clock_settings.virtual.time.substring(0,2)));
    virtual.setMinutes(parseInt(d.clock_settings.virtual.time.substring(3,5)));
    virtual.setSeconds(0);
    virtual.setMilliseconds(0);
    return {
        synchronizationPointActual : actual
        , synchronizationPointVirtual : virtual
        , speed : d.clock_settings.speed
    };
}
export class ClockEnv implements TMInfra.EnvBase {
    private clockSettings : ClockSettings;
    private logConsole : Console;
    public constructor(clockSettings? : ClockSettings, logFile? : string) {
        this.clockSettings = clockSettings;
        if (logFile === undefined || logFile === null || logFile == "") {
            this.logConsole = console;
        } else {
            this.logConsole = new Console(fs.createWriteStream(logFile));
        }
    }
    private actualToVirtual(d : Date) : Date {
        if (this.clockSettings === undefined || this.clockSettings === null) {
            return d;
        } else {
            return new Date(
                this.clockSettings.synchronizationPointVirtual.getTime()
                +Math.round((d.getTime()-this.clockSettings.synchronizationPointActual.getTime())*this.clockSettings.speed)
            );
        }
    }
    public virtualDuration(actualD : number) : number {
        if (this.clockSettings === undefined || this.clockSettings === null) {
            return actualD;
        } else {
            return Math.round(actualD*this.clockSettings.speed);
        }
    }
    public virtualToActual(d : Date) : Date {
        if (this.clockSettings === undefined || this.clockSettings === null) {
            return d;
        } else {
            return new Date(
                this.clockSettings.synchronizationPointActual.getTime()
                +Math.round((d.getTime()-this.clockSettings.synchronizationPointVirtual.getTime())*1.0/this.clockSettings.speed)
            );
        }
    }
    public actualDuration(virtualD : number) : number {
        if (this.clockSettings === undefined || this.clockSettings === null) {
            return virtualD;
        } else {
            return Math.round(virtualD*1.0/this.clockSettings.speed);
        }
    }
    public now() : Date {
        if (this.clockSettings === undefined || this.clockSettings === null) {
            return new Date();
        } else {
            return this.actualToVirtual(new Date());
        }
    }
    public log(level : TMInfra.LogLevel, s : string) : void {
        this.logConsole.log(`[${ClockEnv.formatDate(this.now())}] [${TMInfra.LogLevel[level]}] ${s}`);
    }
    public exit() : void {
        process.exit(0);
    }

    public static clockSettingsWithStartPoint(actualHHMM : number, virtualStartPoint : Date, speed : number) : ClockSettings {
        let d = new Date();
        d.setHours(Math.floor(actualHHMM/100));
        d.setMinutes(actualHHMM%100);
        d.setSeconds(0);
        d.setMilliseconds(0);
        return {
            synchronizationPointActual : d
            , synchronizationPointVirtual : virtualStartPoint
            , speed : speed
        };
    }
    public static clockSettingsWithStartPointCorrespondingToNextAlignment(actualTimeAlignMinute : number, virtualStartPoint : Date, speed : number) : ClockSettings {
        let d = new Date();
        let min = d.getMinutes();
        if (min % actualTimeAlignMinute == 0) {
            min += actualTimeAlignMinute;
        } else {
            min += (min % actualTimeAlignMinute);
        }
        let hour = d.getHours();
        if (min >= 60) {
            hour = hour+1;
            min = 0;
            if (hour >= 24) {
                d = new Date(d.getTime()+24*3600*1000);
                d.setHours(0);
                d.setMinutes(0);
                d.setSeconds(0);
                d.setMilliseconds(0);
            } else {
                d.setHours(hour);
                d.setMinutes(min);
                d.setSeconds(0);
                d.setMilliseconds(0);
            }
        } else {
            d.setHours(hour);
            d.setMinutes(min);
            d.setSeconds(0);
            d.setMilliseconds(0);
        }
        return {
            synchronizationPointActual : d
            , synchronizationPointVirtual : virtualStartPoint
            , speed : speed
        };
    }
    public static formatDate(d : Date) : string {
        return dateformat(d, "yyyy-mm-dd HH:MM:ss.l");
    }
}

export class ClockImporter {
    public static createOneShotClockImporter<Env extends ClockEnv,T>(when : Date, gen : (d : Date) => T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (env : Env, pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let now = env.now();
                if (now <= when) {
                    setTimeout(() => {
                        pub(gen(env.now()), true);
                    }, env.actualDuration(when.getTime()-now.getTime()));
                }
            }
        );
    }
    public static createOneShotClockConstImporter<Env extends ClockEnv,T>(when : Date, t : T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (env : Env, pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let now = env.now()
                if (now <= when) {
                    setTimeout(() => {
                        pub(t, true);
                    }, env.actualDuration(when.getTime()-now.getTime()));
                }
            }
        );
    }
    public static createRecurringClockImporter<Env extends ClockEnv,T>(start : Date, end : Date, periodMs : number, gen : (d : Date) => T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (env : Env, pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let f = () => {
                    let d = env.now();
                    let isFinal = (d.getTime()+periodMs)>end.getTime();
                    pub(gen(d), isFinal);
                    if (!isFinal) {
                        setTimeout(f, env.actualDuration(periodMs));
                    }
                };
                let now = env.now();
                let realStart = start;
                while (now > realStart) {
                    realStart.setTime(realStart.getTime()+periodMs);
                }
                if (realStart < end) {
                    setTimeout(f, env.actualDuration(realStart.getTime()-now.getTime()));
                }
            }
        );
    }
    public static createRecurringConstClockImporter<Env extends ClockEnv,T>(start : Date, end : Date, periodMs : number, t : T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return this.createRecurringClockImporter<Env,T>(start,end,periodMs,((_d : Date) => t));
    }
    public static createVariableDurationRecurringClockImporter<Env extends ClockEnv,T>(start : Date, end : Date, periodCalc : ((d : Date) => number), gen : (d : Date) => T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (env : Env, pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let f = () => {
                    let d = env.now();
                    let nextPeriod = periodCalc(d);
                    let isFinal = (d.getTime()+nextPeriod)>end.getTime();
                    pub(gen(d), isFinal);
                    if (!isFinal) {
                        setTimeout(f, env.actualDuration(nextPeriod));
                    }
                };
                let now = env.now();
                let realStart = start;
                let firstPeriod = periodCalc(start);
                while (now > realStart) {
                    realStart.setTime(realStart.getTime()+firstPeriod);
                }
                if (realStart < end) {
                    setTimeout(f, env.actualDuration(realStart.getTime()-now.getTime()));
                }
            }
        );
    }
    public static createRecurringVariableDurationConstClockImporter<Env extends ClockEnv,T>(start : Date, end : Date, periodCalc : ((d : Date) => number), t : T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return this.createVariableDurationRecurringClockImporter<Env,T>(start,end,periodCalc,((_d : Date) => t));
    }
}

export interface ClockOnOrderFacilityInput<S> {
    inputData : S;
    callbackDurations : number[];        
}
export class ClockOnOrderFacility {
    public static createClockCallback<Env extends ClockEnv,S,T>(
        converter : ((d: Date, index : number, count : number) => T)
    ) : TMInfra.RealTimeApp.OnOrderFacility<Env,ClockOnOrderFacilityInput<S>,T> {
        class LocalF extends TMInfra.RealTimeApp.OnOrderFacility<Env,ClockOnOrderFacilityInput<S>,T> {
            private conv : ((d: Date, index : number, count : number) => T);
            public constructor(c : ((d: Date, index : number, count : number) => T)) {
                super();
                this.conv = c;
            }
            public start(_e : Env) : void {}
            public handle(d : TMInfra.TimedDataWithEnvironment<Env,TMInfra.Key<ClockOnOrderFacilityInput<S>>>) : void {
                let filteredDurations = _.orderBy(_.filter(
                    d.timedData.value.key.callbackDurations
                    , (v : number) => (v >= 0)
                ));
                let id = d.timedData.value.id;
                let env = d.environment;
                let thisObj = this;
                if (filteredDurations.length == 0) {
                    setImmediate(() => {
                        let now = env.now();
                        let t = thisObj.conv(now, 0, 0);
                        thisObj.publish({
                            environment : env
                            , timedData : {
                                timePoint : now
                                , value : {
                                    id : id
                                    , key : t
                                }
                                , finalFlag : true
                            }
                        });
                    });
                } else {
                    let count = filteredDurations.length;
                    for (let i=0; i<count; ++i) {
                        setTimeout(() => {
                            let now = env.now();
                            let t = thisObj.conv(now, i, count);
                            thisObj.publish({
                                environment : env
                                , timedData : {
                                    timePoint : now
                                    , value : {
                                        id : id
                                        , key : t
                                    }
                                    , finalFlag : (i == count-1)
                                }
                            });
                        }, env.actualDuration(filteredDurations[i]));
                    }
                }
            }
        };
        return new LocalF(converter);
    }
}

export namespace Transaction {
    export namespace DataStreamInterface {
        export type OneFullUpdateItem<Key,Version,Data> = TMInfra.GroupedVersionedData<Key,Version,([] | [Data])>;
        export type OneDeltaUpdateItem<Key,VersionDelta,DataDelta> = [Key, VersionDelta, DataDelta];
        export enum OneUpdateItemSubtypes {
            OneFullUpdateItem = 0
            , OneDeltaUpdateItem = 1
        }
        export type OneUpdateItem<Key,Version,Data,VersionDelta,DataDelta> = 
            TMInfra.RealTimeApp.Either2<
                OneFullUpdateItem<Key,Version,Data>
                , OneDeltaUpdateItem<Key,VersionDelta,DataDelta>
            >;
        export type Update<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta> = 
            TMInfra.VersionedData<
                GlobalVersion
                , OneUpdateItem<Key,Version,Data,VersionDelta,DataDelta>[]
            >;
        export type FullUpdate<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta> = 
            TMInfra.VersionedData<
                GlobalVersion
                , OneFullUpdateItem<Key,Version,Data>[]
            >;
        export function getFullUpdatesOnly<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>(
            u : Update<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
        ) : FullUpdate<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta> {
            let fu : OneFullUpdateItem<Key,Version,Data>[] = [];
            for (let x of u.data) {
                if (x[0] == 0) {
                    fu.push(x[1] as OneFullUpdateItem<Key,Version,Data>);
                }
            }
            if (fu.length == 0) {
                return null;
            }
            return {
                version : u.version
                , data: fu
            };
        }
    }
    export namespace TransactionInterface {
        export enum RequestDecision {
            Success = 0
            , FailurePrecondition = 1
            , FailurePermission = 2
            , FailureConsistency = 3
        };
        export interface InsertAction<Key,Data> {
            key : Key;
            data : Data;
        }
        export interface UpdateAction<Key,VersionSlice,DataSummary,DataDelta> {
            key : Key;
            oldVersionSlice : [] | [VersionSlice];
            oldDataSummary : [] | [DataSummary];
            dataDelta : DataDelta;
        }
        export interface DeleteAction<Key,Version,DataSummary> {
            key : Key;
            oldVersion : [] | [Version];
            oldDataSummary : [] | [DataSummary];
        }
        export interface TransactionResponse<GlobalVersion> {
            globalVersion : GlobalVersion;
            requestDecision : RequestDecision;
        }
        export enum TransactionSubtypes {
            InsertAction = 0
            , UpdateAction = 1
            , DeleteAction = 2
        }
        export type Transaction<Key,Version,Data,DataSummary,VersionSlice,DataDelta> =
            TMInfra.RealTimeApp.Either3<
                InsertAction<Key,Data>
                , UpdateAction<Key,VersionSlice,DataSummary,DataDelta>
                , DeleteAction<Key,Version,DataSummary>
            >;
        export type Account = string;
        export type TransactionWithAccountInfo<Key,Version,Data,DataSummary,VersionSlice,DataDelta> = 
            [Account, Transaction<Key,Version,Data,DataSummary,VersionSlice,DataDelta>];
    }
    export namespace GeneralSubscriber {
        export interface Subscription<Key> {
            keys : Key[];
        }
        export interface Unsubscription {
            originalSubscriptionID : string;
        }
        export interface ListSubscriptions {};
        export interface SubscriptionInfo<Key> {
            subscriptions : [string, Key[]][];
        }
        export interface UnsubscribeAll {};
        export interface SnapshotRequest<Key> {
            keys : Key[];
        }
        export type SubscriptionUpdate<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
            = DataStreamInterface.Update<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>;
        
        export enum InputSubtypes {
            Subscription = 0
            , Unsubscription = 1
            , ListSubscriptions = 2
            , UnsubscribeAll = 3
            , SnapshotRequest = 4
        }
        export type Input<Key> = TMInfra.RealTimeApp.Either5<
            Subscription<Key>, Unsubscription, ListSubscriptions, UnsubscribeAll, SnapshotRequest<Key>
        >;
        export enum OutputSubtypes {
            Subscription = 0
            , Unsubscription = 1
            , SubscriptionUpdate = 2
            , SubscriptionInfo = 3
            , UnsubscribeAll = 4
        }
        export type Output<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta> = TMInfra.RealTimeApp.Either5<
            Subscription<Key>, Unsubscription, SubscriptionUpdate<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>, SubscriptionInfo<Key>, UnsubscribeAll
        >;
    }
    export namespace Helpers {
        export interface ClientGSComboOutput<
            Env extends ClockEnv,LocalData,GlobalVersion,Key,Version,Data,VersionDelta,DataDelta
        > {
            localDataSource : TMInfra.RealTimeApp.Source<Env,LocalData>;
            gsInputSink : TMInfra.RealTimeApp.Sink<Env, TMInfra.Key<GeneralSubscriber.Input<Key>>>;
            gsOutputSource : TMInfra.RealTimeApp.Source<Env, TMInfra.KeyedData<GeneralSubscriber.Input<Key>, GeneralSubscriber.Output<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>>>;
        }
        export function clientGSCombo<
            Env extends ClockEnv,LocalData,GlobalVersion,Key,Version,Data,VersionDelta,DataDelta
        >(
            r : TMInfra.RealTimeApp.Runner<Env>
            , facility : TMInfra.RealTimeApp.OnOrderFacility<
                Env
                , GeneralSubscriber.Input<Key>
                , GeneralSubscriber.Output<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
            >
            , localData : LocalData
            , deltaApplier : (localData : LocalData, delta : DataStreamInterface.OneDeltaUpdateItem<Key,VersionDelta,DataDelta>) => void
            , fullUpdateApplier : (localData : LocalData, update : DataStreamInterface.OneFullUpdateItem<Key,Version,Data>) => void
        ) : ClientGSComboOutput<Env,LocalData,GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>
        {
            type GSInput = GeneralSubscriber.Input<Key>;
            type GSOutput = GeneralSubscriber.Output<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>;
            type Update = DataStreamInterface.Update<GlobalVersion,Key,Version,Data,VersionDelta,DataDelta>;
            let gsInputPipe = TMInfra.RealTimeApp.Utils.kleisli<Env,TMInfra.Key<GSInput>,TMInfra.Key<GSInput>>(
                CommonFlowUtils.idFunc<Env,TMInfra.Key<GSInput>>()
            );
            let gsOutputPipe = TMInfra.RealTimeApp.Utils.kleisli<Env,TMInfra.KeyedData<GSInput,GSOutput>,TMInfra.KeyedData<GSInput,GSOutput>>(
                CommonFlowUtils.idFunc<Env,TMInfra.KeyedData<GSInput,GSOutput>>()
            );
            let updateExtractor = TMInfra.RealTimeApp.Utils.liftMaybe<Env,TMInfra.KeyedData<GSInput,GSOutput>,Update>(
                (x : TMInfra.KeyedData<GSInput,GSOutput>) => {
                    if (x.data[0] == Transaction.GeneralSubscriber.OutputSubtypes.SubscriptionUpdate) {
                        return (x.data[1] as Update);
                    } else {
                        return null;
                    }
                }
            );
            let updateApplier = TMInfra.RealTimeApp.Utils.liftPure<Env,Update,LocalData>(
                (x : Update) => {
                    for (let u of x.data) {
                        if (u[0] == DataStreamInterface.OneUpdateItemSubtypes.OneDeltaUpdateItem) {
                            let delta = u[1] as DataStreamInterface.OneDeltaUpdateItem<Key,VersionDelta,DataDelta>;
                            deltaApplier(localData, delta);
                        } else {
                            let full = u[1] as DataStreamInterface.OneFullUpdateItem<Key,Version,Data>;
                            fullUpdateApplier(localData, full);
                        }
                    }
                    return localData;
                }
            );
            r.placeOrderWithFacility<GSInput,GSOutput>(
                r.actionAsSource<TMInfra.Key<GSInput>,TMInfra.Key<GSInput>>(gsInputPipe)
                , facility
                , r.actionAsSink<TMInfra.KeyedData<GSInput,GSOutput>,TMInfra.KeyedData<GSInput,GSOutput>>(gsOutputPipe)
            );
            let updateSrc = r.execute<TMInfra.KeyedData<GSInput,GSOutput>,Update>(
                updateExtractor
                , r.actionAsSource<TMInfra.KeyedData<GSInput,GSOutput>,TMInfra.KeyedData<GSInput,GSOutput>>(gsOutputPipe)
            );
            let localDataSrc = r.execute<Update,LocalData>(updateApplier, updateSrc);
            return {
                localDataSource : localDataSrc
                , gsInputSink : r.actionAsSink<TMInfra.Key<GSInput>,TMInfra.Key<GSInput>>(gsInputPipe)
                , gsOutputSource : r.actionAsSource<TMInfra.KeyedData<GSInput,GSOutput>,TMInfra.KeyedData<GSInput,GSOutput>>(gsOutputPipe)
            };
        }
    }
}

export type VoidStruct = 0;

export type ByteData = Buffer;
export interface ByteDataWithTopic {
    topic : string;
    content : ByteData;
}
export interface TypedDataWithTopic<T> {
    topic : string;
    content : T;
}

export namespace Files {
    export function byteDataWithTopicOutput<Env extends TMInfra.EnvBase>(name : string, filePrefix? : Buffer, recordPrefix? : Buffer)
        : TMInfra.RealTimeApp.Exporter<Env,ByteDataWithTopic>
    {
        class LocalE extends TMInfra.RealTimeApp.Exporter<Env,ByteDataWithTopic> {
            private fileStream : Stream.Writable;
            private name : string;
            private filePrefix : Buffer;
            private recordPrefix : Buffer;
            public constructor(name : string, filePrefix : Buffer, recordPrefix : Buffer) {
                super();
                this.fileStream = null;
                this.name = name;
                this.filePrefix = filePrefix;
                this.recordPrefix = recordPrefix;
            }
            public start(_e : Env) {
                this.fileStream = fs.createWriteStream(this.name);
                if (this.filePrefix) {
                    this.fileStream.write(this.filePrefix);
                }
            }
            public handle(d : TMInfra.TimedDataWithEnvironment<Env,ByteDataWithTopic>) : void {
                let prefixLen = 0;
                if (this.recordPrefix) {
                    prefixLen = this.recordPrefix.length;
                }
                let v = d.timedData.value;
                let buffer = Buffer.alloc(prefixLen+8+4+v.topic.length+4+v.content.length+1);
                if (this.recordPrefix) {
                    this.recordPrefix.copy(buffer, 0);
                }
                buffer.writeBigInt64LE(BigInt(d.environment.now().getTime())*BigInt(1000), prefixLen);
                buffer.writeUInt32LE(v.topic.length,prefixLen+8);
                buffer.write(v.topic, prefixLen+12);
                buffer.writeUInt32LE(v.content.length, prefixLen+12+v.topic.length);
                buffer[buffer.length-1] = (d.timedData.finalFlag?0x1:0x0);
                v.content.copy(buffer, prefixLen+16+v.topic.length);
                this.fileStream.write(buffer);
            }
        }
        return new LocalE(name, filePrefix, recordPrefix);
    }
}

export namespace CommonFlowUtils {
    export function idFunc<Env extends TMInfra.EnvBase,T>() : TMInfra.Kleisli.F<Env,T,T> {
        return TMInfra.Kleisli.Utils.liftPure<Env,T,T>((x : T) => x);
    }
}