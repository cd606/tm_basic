import * as TMInfra from '@dev_cd606/tm_infra';
import * as _ from 'lodash';
import * as dateFNS from 'date-fns'
import * as fs from 'fs'
import * as Stream from 'stream'
import { Console } from 'console';
import * as Types from './types';

export type * from './types';
export { Transaction_DataStreamInterface_OneUpdateItemSubtypes, TransactionInterface_RequestDecision, TransactionInterface_TransactionSubtypes, GeneralSubscriber_InputSubtypes, GeneralSubscriber_OutputSubtypes } from './types';

const printf = require('printf');  //this module can only be loaded this way in TypeScript

export function parseClockSettingsDescription(d: Types.ClockSettingsDescription): Types.ClockSettings | null {
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
    actual.setHours(parseInt(d.clock_settings.actual.time.substring(0, 2)));
    actual.setMinutes(parseInt(d.clock_settings.actual.time.substring(3, 5)));
    actual.setSeconds(0);
    actual.setMilliseconds(0);
    let virtual = new Date(
        parseInt(d.clock_settings.virtual.date.substring(0, 4))
        , parseInt(d.clock_settings.virtual.date.substring(5, 7))
        , parseInt(d.clock_settings.virtual.date.substring(8, 10))
    );
    virtual.setHours(parseInt(d.clock_settings.virtual.time.substring(0, 2)));
    virtual.setMinutes(parseInt(d.clock_settings.virtual.time.substring(3, 5)));
    virtual.setSeconds(0);
    virtual.setMilliseconds(0);
    return {
        synchronizationPointActual: actual
        , synchronizationPointVirtual: virtual
        , speed: d.clock_settings.speed
    };
}
export class ClockEnv implements TMInfra.EnvBase {
    private clockSettings: Types.ClockSettings | null;
    private logConsole: Console;
    public constructor(clockSettings?: Types.ClockSettings, logFile?: string) {
        if (clockSettings === undefined || clockSettings === null) {
            this.clockSettings = null;
        } else {
            this.clockSettings = clockSettings;
        }
        if (logFile === undefined || logFile === null || logFile == "") {
            this.logConsole = console;
        } else {
            this.logConsole = new Console(fs.createWriteStream(logFile));
        }
    }
    private actualToVirtual(d: Date): Date {
        if (this.clockSettings === null) {
            return d;
        } else {
            return new Date(
                this.clockSettings.synchronizationPointVirtual.getTime()
                + Math.round((d.getTime() - this.clockSettings.synchronizationPointActual.getTime()) * this.clockSettings.speed)
            );
        }
    }
    public virtualDuration(actualD: number): number {
        if (this.clockSettings === null) {
            return actualD;
        } else {
            return Math.round(actualD * this.clockSettings.speed);
        }
    }
    public virtualToActual(d: Date): Date {
        if (this.clockSettings === null) {
            return d;
        } else {
            return new Date(
                this.clockSettings.synchronizationPointActual.getTime()
                + Math.round((d.getTime() - this.clockSettings.synchronizationPointVirtual.getTime()) * 1.0 / this.clockSettings.speed)
            );
        }
    }
    public actualDuration(virtualD: number): number {
        if (this.clockSettings === null) {
            return virtualD;
        } else {
            return Math.round(virtualD * 1.0 / this.clockSettings.speed);
        }
    }
    public now(): Date {
        if (this.clockSettings === null) {
            return new Date();
        } else {
            return this.actualToVirtual(new Date());
        }
    }
    public log(level: TMInfra.LogLevel, s: string): void {
        this.logConsole.log(`[${ClockEnv.formatDate(this.now())}] [${TMInfra.LogLevel[level]}] ${s}`);
    }
    public exit(): void {
        process.exit(0);
    }

    public static clockSettingsWithStartPoint(actualHHMM: number, virtualStartPoint: Date, speed: number): Types.ClockSettings {
        let d = new Date();
        d.setHours(Math.floor(actualHHMM / 100));
        d.setMinutes(actualHHMM % 100);
        d.setSeconds(0);
        d.setMilliseconds(0);
        return {
            synchronizationPointActual: d
            , synchronizationPointVirtual: virtualStartPoint
            , speed: speed
        };
    }
    public static clockSettingsWithStartPointCorrespondingToNextAlignment(actualTimeAlignMinute: number, virtualStartPoint: Date, speed: number): Types.ClockSettings {
        let d = new Date();
        let min = d.getMinutes();
        if (min % actualTimeAlignMinute == 0) {
            min += actualTimeAlignMinute;
        } else {
            min += (min % actualTimeAlignMinute);
        }
        let hour = d.getHours();
        if (min >= 60) {
            hour = hour + 1;
            min = min % 60;
            if (hour >= 24) {
                d = new Date(d.getTime() + 24 * 3600 * 1000);
                d.setHours(0);
                d.setMinutes(min);
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
            synchronizationPointActual: d
            , synchronizationPointVirtual: virtualStartPoint
            , speed: speed
        };
    }
    public static formatDate(d: Date): string {
        return dateFNS.format(d, "yyyy-MM-dd HH:mm:ss.SSS");
    }
}

export class ClockImporter {
    public static createOneShotClockImporter<Env extends ClockEnv, T>(when: Date, gen: (d: Date) => T): TMInfra.RealTimeApp_Importer<Env, T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env, T>(
            function (env: Env, pub: TMInfra.PubFunc<T>): void {
                let now = env.now();
                if (now <= when) {
                    setTimeout(() => {
                        pub(gen(env.now()), true);
                    }, env.actualDuration(when.getTime() - now.getTime()));
                }
            }
        );
    }
    public static createOneShotClockConstImporter<Env extends ClockEnv, T>(when: Date, t: T): TMInfra.RealTimeApp_Importer<Env, T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env, T>(
            function (env: Env, pub: TMInfra.PubFunc<T>): void {
                let now = env.now()
                if (now <= when) {
                    setTimeout(() => {
                        pub(t, true);
                    }, env.actualDuration(when.getTime() - now.getTime()));
                }
            }
        );
    }
    public static createRecurringClockImporter<Env extends ClockEnv, T>(start: Date, end: Date, periodMs: number, gen: (d: Date) => T): TMInfra.RealTimeApp_Importer<Env, T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env, T>(
            function (env: Env, pub: TMInfra.PubFunc<T>): void {
                let f = () => {
                    let d = env.now();
                    let isFinal = (d.getTime() + periodMs) > end.getTime();
                    pub(gen(d), isFinal);
                    if (!isFinal) {
                        setTimeout(f, env.actualDuration(periodMs));
                    }
                };
                let now = env.now();
                let realStart = start;
                while (now > realStart) {
                    realStart.setTime(realStart.getTime() + periodMs);
                }
                if (realStart < end) {
                    setTimeout(f, env.actualDuration(realStart.getTime() - now.getTime()));
                }
            }
        );
    }
    public static createRecurringConstClockImporter<Env extends ClockEnv, T>(start: Date, end: Date, periodMs: number, t: T): TMInfra.RealTimeApp_Importer<Env, T> {
        return this.createRecurringClockImporter<Env, T>(start, end, periodMs, ((_d: Date) => t));
    }
    public static createVariableDurationRecurringClockImporter<Env extends ClockEnv, T>(start: Date, end: Date, periodCalc: ((d: Date) => number), gen: (d: Date) => T): TMInfra.RealTimeApp_Importer<Env, T> {
        return TMInfra.RealTimeApp.Utils.simpleImporter<Env, T>(
            function (env: Env, pub: TMInfra.PubFunc<T>): void {
                let f = () => {
                    let d = env.now();
                    let nextPeriod = periodCalc(d);
                    let isFinal = (d.getTime() + nextPeriod) > end.getTime();
                    pub(gen(d), isFinal);
                    if (!isFinal) {
                        setTimeout(f, env.actualDuration(nextPeriod));
                    }
                };
                let now = env.now();
                let realStart = start;
                let firstPeriod = periodCalc(start);
                while (now > realStart) {
                    realStart.setTime(realStart.getTime() + firstPeriod);
                }
                if (realStart < end) {
                    setTimeout(f, env.actualDuration(realStart.getTime() - now.getTime()));
                }
            }
        );
    }
    public static createRecurringVariableDurationConstClockImporter<Env extends ClockEnv, T>(start: Date, end: Date, periodCalc: ((d: Date) => number), t: T): TMInfra.RealTimeApp_Importer<Env, T> {
        return this.createVariableDurationRecurringClockImporter<Env, T>(start, end, periodCalc, ((_d: Date) => t));
    }
}

export class ClockOnOrderFacility {
    public static createClockCallback<Env extends ClockEnv, S, T>(
        converter: ((d: Date, index: number, count: number) => T)
    ): TMInfra.RealTimeApp_OnOrderFacility<Env, Types.ClockOnOrderFacilityInput<S>, T> {
        class LocalF extends TMInfra.RealTimeApp.OnOrderFacility<Env, Types.ClockOnOrderFacilityInput<S>, T> {
            private conv: ((d: Date, index: number, count: number) => T);
            public constructor(c: ((d: Date, index: number, count: number) => T)) {
                super();
                this.conv = c;
            }
            public start(_e: Env): void { }
            public handle(d: TMInfra.TimedDataWithEnvironment<Env, TMInfra.Key<Types.ClockOnOrderFacilityInput<S>>>): void {
                let filteredDurations = _.orderBy(_.filter(
                    d.timedData.value.key.callbackDurations
                    , (v: number) => (v >= 0)
                ));
                let id = d.timedData.value.id;
                let env = d.environment;
                let thisObj = this;
                if (filteredDurations.length == 0) {
                    setImmediate(() => {
                        let now = env.now();
                        let t = thisObj.conv(now, 0, 0);
                        thisObj.publish({
                            environment: env
                            , timedData: {
                                timePoint: now
                                , value: {
                                    id: id
                                    , key: t
                                }
                                , finalFlag: true
                            }
                        });
                    });
                } else {
                    let count = filteredDurations.length;
                    for (let i = 0; i < count; ++i) {
                        setTimeout(() => {
                            let now = env.now();
                            let t = thisObj.conv(now, i, count);
                            thisObj.publish({
                                environment: env
                                , timedData: {
                                    timePoint: now
                                    , value: {
                                        id: id
                                        , key: t
                                    }
                                    , finalFlag: (i == count - 1)
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
        export function getFullUpdatesOnly<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>(
            u: Types.Transaction_DataStreamInterface_Update<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>
        ): Types.Transaction_DataStreamInterface_FullUpdate<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta> | null {
            let fu: Types.Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>[] = [];
            for (let x of u.data) {
                if (x[0] == 0) {
                    fu.push(x[1] as Types.Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>);
                }
            }
            if (fu.length == 0) {
                return null;
            }
            return {
                version: u.version
                , data: fu
            };
        }
        export function eraseVersion<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>(
            u: Types.Transaction_DataStreamInterface_Update<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>
        ): Types.Transaction_DataStreamInterface_Update<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta> {
            u.version = null;
            for (let x of u.data) {
                if (x[0] == 0) {
                    //full update
                    (x[1] as Types.Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>).version = null;
                } else {
                    (x[1] as Types.Transaction_DataStreamInterface_OneDeltaUpdateItem<Key, VersionDelta, DataDelta>)[1] = null;
                }
            }
            return u;
        }
    }
    export namespace Helpers {
        export interface ClientGSComboOutput<
            Env extends ClockEnv, LocalData, GlobalVersion, Key, Version, Data, VersionDelta, DataDelta
        > {
            localDataSource: TMInfra.RealTimeApp_Source<Env, LocalData>;
            gsInputSink: TMInfra.RealTimeApp_Sink<Env, TMInfra.Key<Types.GeneralSubscriber_Input<Key>>>;
            gsOutputSource: TMInfra.RealTimeApp_Source<Env, TMInfra.KeyedData<Types.GeneralSubscriber_Input<Key>, Types.GeneralSubscriber_Output<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>>>;
        }
        export function clientGSCombo<
            Env extends ClockEnv, LocalData, GlobalVersion, Key, Version, Data, VersionDelta, DataDelta
        >(
            r: TMInfra.RealTimeApp_Runner<Env>
            , facility: TMInfra.RealTimeApp_OnOrderFacility<
                Env
                , Types.GeneralSubscriber_Input<Key>
                , Types.GeneralSubscriber_Output<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>
            >
            , localData: LocalData
            , deltaApplier: (localData: LocalData, delta: Types.Transaction_DataStreamInterface_OneDeltaUpdateItem<Key, VersionDelta, DataDelta>) => void
            , fullUpdateApplier: (localData: LocalData, update: Types.Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>) => void
        ): ClientGSComboOutput<Env, LocalData, GlobalVersion, Key, Version, Data, VersionDelta, DataDelta> {
            type GSInput = Types.GeneralSubscriber_Input<Key>;
            type GSOutput = Types.GeneralSubscriber_Output<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>;
            type Update = Types.Transaction_DataStreamInterface_Update<GlobalVersion, Key, Version, Data, VersionDelta, DataDelta>;
            let gsInputPipe = TMInfra.RealTimeApp.Utils.kleisli<Env, TMInfra.Key<GSInput>, TMInfra.Key<GSInput>>(
                CommonFlowUtils.idFunc<Env, TMInfra.Key<GSInput>>()
            );
            let gsOutputPipe = TMInfra.RealTimeApp.Utils.kleisli<Env, TMInfra.KeyedData<GSInput, GSOutput>, TMInfra.KeyedData<GSInput, GSOutput>>(
                CommonFlowUtils.idFunc<Env, TMInfra.KeyedData<GSInput, GSOutput>>()
            );
            let updateExtractor = TMInfra.RealTimeApp.Utils.liftMaybe<Env, TMInfra.KeyedData<GSInput, GSOutput>, Update>(
                (x: TMInfra.KeyedData<GSInput, GSOutput>) => {
                    if (x.data[0] == Types.GeneralSubscriber_OutputSubtypes.SubscriptionUpdate) {
                        return (x.data[1] as Update);
                    } else {
                        return null;
                    }
                }
            );
            let updateApplier = TMInfra.RealTimeApp.Utils.liftPure<Env, Update, LocalData>(
                (x: Update) => {
                    for (let u of x.data) {
                        if (u[0] == Types.Transaction_DataStreamInterface_OneUpdateItemSubtypes.OneDeltaUpdateItem) {
                            let delta = u[1] as Types.Transaction_DataStreamInterface_OneDeltaUpdateItem<Key, VersionDelta, DataDelta>;
                            deltaApplier(localData, delta);
                        } else {
                            let full = u[1] as Types.Transaction_DataStreamInterface_OneFullUpdateItem<Key, Version, Data>;
                            fullUpdateApplier(localData, full);
                        }
                    }
                    return localData;
                }
            );
            r.placeOrderWithFacility<GSInput, GSOutput>(
                r.actionAsSource<TMInfra.Key<GSInput>, TMInfra.Key<GSInput>>(gsInputPipe)
                , facility
                , r.actionAsSink<TMInfra.KeyedData<GSInput, GSOutput>, TMInfra.KeyedData<GSInput, GSOutput>>(gsOutputPipe)
            );
            let updateSrc = r.execute<TMInfra.KeyedData<GSInput, GSOutput>, Update>(
                updateExtractor
                , r.actionAsSource<TMInfra.KeyedData<GSInput, GSOutput>, TMInfra.KeyedData<GSInput, GSOutput>>(gsOutputPipe)
            );
            let localDataSrc = r.execute<Update, LocalData>(updateApplier, updateSrc);
            return {
                localDataSource: localDataSrc
                , gsInputSink: r.actionAsSink<TMInfra.Key<GSInput>, TMInfra.Key<GSInput>>(gsInputPipe)
                , gsOutputSource: r.actionAsSource<TMInfra.KeyedData<GSInput, GSOutput>, TMInfra.KeyedData<GSInput, GSOutput>>(gsOutputPipe)
            };
        }
    }
}

export namespace Files {
    export function byteDataWithTopicOutput<Env extends TMInfra.EnvBase>(name: string, filePrefix?: Buffer, recordPrefix?: Buffer)
        : TMInfra.RealTimeApp_Exporter<Env, Types.ByteDataWithTopic> {
        class LocalE extends TMInfra.RealTimeApp.Exporter<Env, Types.ByteDataWithTopic> {
            private fileStream: Stream.Writable | null;
            private name: string;
            private filePrefix: Buffer | null;
            private recordPrefix: Buffer | null;
            public constructor(name: string, filePrefix: Buffer | null, recordPrefix: Buffer | null) {
                super();
                this.fileStream = null;
                this.name = name;
                this.filePrefix = filePrefix;
                this.recordPrefix = recordPrefix;
            }
            public start(_e: Env) {
                this.fileStream = fs.createWriteStream(this.name);
                if (this.filePrefix) {
                    this.fileStream.write(this.filePrefix);
                }
            }
            public handle(d: TMInfra.TimedDataWithEnvironment<Env, Types.ByteDataWithTopic>): void {
                if (this.fileStream === null) {
                    return;
                }
                let prefixLen = 0;
                if (this.recordPrefix) {
                    prefixLen = this.recordPrefix.length;
                }
                let v = d.timedData.value;
                let buffer = Buffer.alloc(prefixLen + 8 + 4 + v.topic.length + 4 + v.content.length + 1);
                if (this.recordPrefix) {
                    this.recordPrefix.copy(buffer, 0);
                }
                buffer.writeBigInt64LE(BigInt(d.timedData.timePoint.getTime()) * BigInt(1000), prefixLen);
                buffer.writeUInt32LE(v.topic.length, prefixLen + 8);
                buffer.write(v.topic, prefixLen + 12);
                buffer.writeUInt32LE(v.content.length, prefixLen + 12 + v.topic.length);
                buffer[buffer.length - 1] = (d.timedData.finalFlag ? 0x1 : 0x0);
                v.content.copy(buffer, prefixLen + 16 + v.topic.length);
                this.fileStream.write(buffer);
            }
        }
        return new LocalE(name, (filePrefix ? filePrefix : null), (recordPrefix ? recordPrefix : null));
    }
    export abstract class RecordReader<T> {
        public abstract start(input: Buffer): number;
        public abstract readOne(input: Buffer): [number, T | undefined];
    }
    export function genericRecordStream<T>(r: RecordReader<T>): Stream.Transform {
        class MyTransform extends Stream.Transform {
            localBuffer = Buffer.from([]);
            atStart = true;
            reader: RecordReader<T> | null = null;

            constructor(r: RecordReader<T>) {
                super({ objectMode: true });
                this.reader = r;
            }
            handleBuffer() {
            }
            _transform(chunk: Buffer, _encoding: BufferEncoding, callback: () => void) {
                this.localBuffer = Buffer.concat([this.localBuffer, chunk]);
                if (this.atStart) {
                    let n = this.reader!.start(this.localBuffer);
                    if (n >= 0) {
                        if (n > 0) {
                            this.localBuffer = this.localBuffer.slice(n);
                        }
                        this.atStart = false;
                    }
                }
                while (true) {
                    let res: [number, T | undefined] = this.reader!.readOne(this.localBuffer);
                    if (res[0] >= 0) {
                        if (res[1] !== undefined) {
                            this.push(res[1], undefined);
                        }
                        if (res[0] > 0) {
                            this.localBuffer = this.localBuffer.slice(res[0]);
                        }
                    } else {
                        break;
                    }
                }
                callback();
            }
            _flush(callback: () => void) {
                callback();
            }
        }
        return new MyTransform(r);
    }
    export function defaultTopicCaptureFileRecordReaderOption(): Types.Files_TopicCaptureFileRecordReaderOption {
        return {
            fileMagicLength: 4
            , recordMagicLength: 4
            , timeFieldLength: 8
            , topicLengthFieldLength: 4
            , dataLengthFieldLength: 4
            , hasFinalFlagField: true
            , timePrecision: "microsecond"
        };
    }
    export class TopicCaptureFileRecordReader extends RecordReader<Types.Files_TopicCaptureFileRecord> {
        option: Types.Files_TopicCaptureFileRecordReaderOption;

        constructor(o: Types.Files_TopicCaptureFileRecordReaderOption) {
            super();
            this.option = o;
        }
        start(b: Buffer): number {
            if (b.byteLength >= this.option.fileMagicLength) {
                return this.option.fileMagicLength;
            } else {
                return -1;
            }
        }
        readOne(b: Buffer): [number, Types.Files_TopicCaptureFileRecord | undefined] {
            let l = b.byteLength;
            if (l < this.option.recordMagicLength) {
                return [-1, undefined];
            }
            let idx = this.option.recordMagicLength;
            l -= this.option.recordMagicLength;
            if (l < this.option.timeFieldLength) {
                return [-1, undefined];
            }
            let theTime: Date | null = null;
            let theTimeString: string = "";
            if (this.option.timeFieldLength > 0) {
                let t: bigint = b.slice(idx, idx + this.option.timeFieldLength).readBigInt64LE();
                switch (this.option.timePrecision) {
                    case "second":
                        theTime = new Date(Number(t * BigInt(1000)));
                        theTimeString = dateFNS.format(theTime, "yyyy-MM-dd HH:mm:ss");
                        break;
                    case "millisecond":
                        theTime = new Date(Number(t));
                        theTimeString = dateFNS.format(theTime, "yyyy-MM-dd HH:mm:ss.SSS");
                        break;
                    case "microsecond":
                        theTime = new Date(Number(t / BigInt(1000)));
                        theTimeString = dateFNS.format(theTime, "yyyy-MM-dd HH:mm:ss") + printf('.%06d', Number(t % BigInt(1000000)));
                        break;
                }
                idx += this.option.timeFieldLength;
                l -= this.option.timeFieldLength;
            }
            if (l < this.option.topicLengthFieldLength) {
                return [-1, undefined];
            }
            let topic: string = "";
            if (this.option.topicLengthFieldLength > 0) {
                let topicLen = b.slice(idx, idx + this.option.topicLengthFieldLength).readUInt32LE();
                idx += this.option.topicLengthFieldLength;
                l -= this.option.topicLengthFieldLength;
                if (l < topicLen) {
                    return [-1, undefined];
                }
                topic = b.slice(idx, idx + topicLen).toString('utf-8');
                idx += topicLen;
                l -= topicLen;
            }
            if (l < this.option.dataLengthFieldLength) {
                return [-1, undefined];
            }
            let data = Buffer.from([]);
            if (this.option.dataLengthFieldLength > 0) {
                let dataLength = b.slice(idx, idx + this.option.dataLengthFieldLength).readUInt32LE();
                idx += this.option.dataLengthFieldLength;
                l -= this.option.dataLengthFieldLength;
                if (l < dataLength) {
                    return [-1, undefined];
                }
                data = b.slice(idx, idx + dataLength);
                idx += dataLength;
                l -= dataLength;
            }
            let isFinal = false;
            if (this.option.hasFinalFlagField) {
                if (l < 1) {
                    return [-1, undefined];
                }
                isFinal = (b[idx] != 0);
                ++idx;
                --l;
            }
            return [idx, {
                time: theTime!
                , timeString: theTimeString
                , topic: topic
                , data: data
                , isFinal: isFinal
            }];
        }
    }
    export function topicCaptureFileReplayImporter<Env extends ClockEnv>(inputStream: Stream.Readable, option: Types.Files_TopicCaptureFileRecordReaderOption, overrideDate: boolean): TMInfra.RealTimeApp_Importer<Env, Types.Files_TopicCaptureFileRecord> {
        class LocalI extends TMInfra.RealTimeApp.Importer<Env, Types.Files_TopicCaptureFileRecord> {
            private inputStream: Stream.Readable;
            private option: Types.Files_TopicCaptureFileRecordReaderOption;
            private overrideDate: boolean;
            private todayStart: Date | null;
            public constructor(inputStream: Stream.Readable, option: Types.Files_TopicCaptureFileRecordReaderOption, overrideDate: boolean) {
                super();
                this.inputStream = inputStream;
                this.option = option;
                this.overrideDate = overrideDate;
                this.todayStart = null;
            }
            public start(e: Env): void {
                this.env = e;
                if (this.overrideDate) {
                    var t = this.env.now();
                    this.todayStart = new Date(t.getFullYear(), t.getMonth(), t.getDate(), 0, 0, 0, 0);
                }
                var transform = genericRecordStream<Types.Files_TopicCaptureFileRecord>(new TopicCaptureFileRecordReader(this.option));
                this.inputStream.pipe(transform);
                (async () => {
                    while (true) {
                        var now = this.env!.now();
                        var hasNext = false;
                        for await (const rec of transform) {
                            hasNext = true;
                            var t = rec.time as Date;
                            if (this.overrideDate) {
                                var tStart = new Date(t.getFullYear(), t.getMonth(), t.getDate(), 0, 0, 0, 0);
                                t = new Date(t.getTime() - tStart.getTime() + this.todayStart!.getTime());
                                rec.time = t;
                            }

                            if (t < now) {
                                if (now.getTime() - t.getTime() < 1000) {
                                    this.publish(rec, false);
                                }
                            } else if (rec.time.equals(now)) {
                                this.publish(rec, false);
                            } else {
                                await new Promise(resolve => setTimeout(resolve, this.env!.actualDuration(t.getTime() - now.getTime())));
                                this.publish(rec, false);
                                break;
                            }
                        }
                        if (!hasNext) {
                            break;
                        }
                    }
                })();
            }
        };
        return new LocalI(inputStream, option, overrideDate);
    }
}

export namespace CommonFlowUtils {
    export function idFunc<Env extends TMInfra.EnvBase, T>(): TMInfra.Kleisli_F<Env, T, T> {
        return TMInfra.Kleisli.Utils.liftPure<Env, T, T>((x: T) => x);
    }
}
