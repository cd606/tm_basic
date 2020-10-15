import * as TMInfra from '../../tm_infra/node_lib/TMInfra'
import * as _ from 'lodash';
import * as dateformat from 'dateformat'

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
    public constructor(s : ClockSettings = null) {
        this.clockSettings = s;
    }
    private actualToVirtual(d : Date) : Date {
        if (this.clockSettings == null) {
            return d;
        } else {
            return new Date(
                this.clockSettings.synchronizationPointVirtual.getTime()
                +Math.round((d.getTime()-this.clockSettings.synchronizationPointActual.getTime())*this.clockSettings.speed)
            );
        }
    }
    public virtualDuration(actualD : number) : number {
        if (this.clockSettings == null) {
            return actualD;
        } else {
            return Math.round(actualD*this.clockSettings.speed);
        }
    }
    public virtualToActual(d : Date) : Date {
        if (this.clockSettings == null) {
            return d;
        } else {
            return new Date(
                this.clockSettings.synchronizationPointActual.getTime()
                +Math.round((d.getTime()-this.clockSettings.synchronizationPointVirtual.getTime())*1.0/this.clockSettings.speed)
            );
        }
    }
    public actualDuration(virtualD : number) : number {
        if (this.clockSettings == null) {
            return virtualD;
        } else {
            return Math.round(virtualD*1.0/this.clockSettings.speed);
        }
    }
    public now() : Date {
        if (this.clockSettings == null) {
            return new Date();
        } else {
            return this.actualToVirtual(new Date());
        }
    }
    public log(level : TMInfra.LogLevel, s : string) : void {
        console.log(`[${ClockEnv.formatDate(this.now())}] [${TMInfra.LogLevel[level]}] ${s}`);
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
        export type OneFullUpdateItem<Key,Version,Data> = TMInfra.GroupedVersionedData<Key,Version,Data>;
        export type OneDeltaUpdateItem<Key,VersionDelta,DataDelta> = [Key, VersionDelta, DataDelta];
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
            oldVersionSlice : VersionSlice;
            oldDataSummary : DataSummary;
            dataDelta : DataDelta;
        }
        export interface DeleteAction<Key,Version,DataSummary> {
            key : Key;
            oldVersion : Version;
            oldDataSummary : DataSummary;
        }
        export interface TransactionResponse<GlobalVersion> {
            globalVersion : GlobalVersion;
            requestDecision : RequestDecision;
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
}