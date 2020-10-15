import * as TMInfra from '../../tm_infra/node_lib/TMInfra'
import * as _ from 'lodash';

export class ClockImporter {
    public static createOneShotClockImporter<Env,T>(when : Date, gen : (d : Date) => T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let now = new Date();
                if (now <= when) {
                    setTimeout(() => {
                        pub(gen(new Date()), true);
                    }, when.getTime()-now.getTime());
                }
            }
        );
    }
    public static createOneShotClockConstImporter<Env,T>(when : Date, t : T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let now = new Date();
                if (now <= when) {
                    setTimeout(() => {
                        pub(t, true);
                    }, when.getTime()-now.getTime());
                }
            }
        );
    }
    public static createRecurringClockImporter<Env,T>(start : Date, end : Date, periodMs : number, gen : (d : Date) => T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let f = () => {
                    let d = new Date();
                    let isFinal = (d.getTime()+periodMs)>end.getTime();
                    pub(gen(d), isFinal);
                    if (!isFinal) {
                        setTimeout(f, periodMs);
                    }
                };
                let now = new Date();
                let realStart = start;
                while (now > realStart) {
                    realStart.setTime(realStart.getTime()+periodMs);
                }
                if (realStart < end) {
                    setTimeout(f, realStart.getTime()-now.getTime());
                }
            }
        );
    }
    public static createRecurringConstClockImporter<Env,T>(start : Date, end : Date, periodMs : number, t : T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return this.createRecurringClockImporter<Env,T>(start,end,periodMs,((_d : Date) => t));
    }
    public static createVariableDurationRecurringClockImporter<Env,T>(start : Date, end : Date, periodCalc : ((d : Date) => number), gen : (d : Date) => T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env,T>(
            function (pub : TMInfra.RealTimeApp.PubFunc<T>) : void {
                let f = () => {
                    let d = new Date();
                    let nextPeriod = periodCalc(d);
                    let isFinal = (d.getTime()+nextPeriod)>end.getTime();
                    pub(gen(d), isFinal);
                    if (!isFinal) {
                        setTimeout(f, nextPeriod);
                    }
                };
                let now = new Date();
                let realStart = start;
                let firstPeriod = periodCalc(start);
                while (now > realStart) {
                    realStart.setTime(realStart.getTime()+firstPeriod);
                }
                if (realStart < end) {
                    setTimeout(f, realStart.getTime()-now.getTime());
                }
            }
        );
    }
    public static createRecurringVariableDurationConstClockImporter<Env,T>(start : Date, end : Date, periodCalc : ((d : Date) => number), t : T) : TMInfra.RealTimeApp.Importer<Env,T> {
        return this.createVariableDurationRecurringClockImporter<Env,T>(start,end,periodCalc,((_d : Date) => t));
    }
}

export interface ClockOnOrderFacilityInput<S> {
    inputData : S;
    callbackDurations : number[];        
}
export class ClockOnOrderFacility {
    public static createClockCallback<Env,S,T>(
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
                        let now = new Date();
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
                            let now = new Date();
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
                        }, filteredDurations[count]);
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