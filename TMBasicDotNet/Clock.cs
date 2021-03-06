﻿using System;
using System.Text.RegularExpressions;
using System.Timers;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json.Linq;
using Here;
using NLog;

using Dev.CD606.TM.Infra;
using Dev.CD606.TM.Infra.RealTimeApp;

namespace Dev.CD606.TM.Basic
{
    public readonly struct ClockSettings
    {
        public readonly long synchronizationPointActualUnixMillis;
        public readonly long synchronizationPointVirtualUnixMillis;
        public readonly double speed;
        public ClockSettings(DateTimeOffset synchronizationPointActual, DateTimeOffset synchronizationPointVirtual, double speed)
        {
            this.synchronizationPointActualUnixMillis = synchronizationPointActual.ToUnixTimeMilliseconds();
            this.synchronizationPointVirtualUnixMillis = synchronizationPointVirtual.ToUnixTimeMilliseconds();
            this.speed = speed;
        }
    }

    public class ClockEnv : EnvBase
    {
        public static Option<ClockSettings> parseClockSettingsDescription(JObject description)
        {
            var actualTime = (string) description["clock_settings"]["actual"]["time"];
            if (!Regex.Match(actualTime, "^\\d{2}:\\d{2}$").Success)
            {
                return Option.None;
            }
            var virtualDate = (string) description["clock_settings"]["virtual"]["date"];
            if (!Regex.Match(actualTime, "^\\d{4}-\\d{2}-\\d{2}$").Success)
            {
                return Option.None;
            }
            var virtualTime = (string) description["clock_settings"]["virtual"]["time"];
            if (!Regex.Match(virtualTime, "^\\d{2}:\\d{2}$").Success)
            {
                return Option.None;
            }
            var actualTP = new DateTimeOffset(DateTime.Today)
                .AddHours(Int16.Parse(actualTime.Substring(0,2)))
                .AddMinutes(Int16.Parse(actualTime.Substring(3,5)));
            var virtualTP = new DateTimeOffset(new DateTime(
                Int32.Parse(virtualDate.Substring(0,4))
                , Int16.Parse(virtualDate.Substring(5,7))
                , Int16.Parse(virtualDate.Substring(8,10))
                , Int16.Parse(virtualTime.Substring(0,2))
                , Int16.Parse(virtualTime.Substring(3,5))
                , 0
            ));
            return new ClockSettings(
                synchronizationPointActual : actualTP
                , synchronizationPointVirtual : virtualTP
                , speed : Double.Parse((string) description["clock_settings"]["speed"])
            );
        }
        public static ClockSettings clockSettingsWithStartPoint(int actualHHMM, DateTimeOffset virtualStartPoint, double speed)
        {
            var d = new DateTimeOffset(DateTime.Today)
                .AddHours(actualHHMM/100)
                .AddMinutes(actualHHMM%100);
            return new ClockSettings(
                synchronizationPointActual: d
                , synchronizationPointVirtual: virtualStartPoint
                , speed: speed
            );
        }
        public static ClockSettings clockSettingsWithStartPointCorrespondingToNextAlignment(int actualTimeAlignMinute, DateTimeOffset virtualStartPoint, double speed) 
        {
            var d = DateTimeOffset.Now;
            var min = d.Minute;
            if (min % actualTimeAlignMinute == 0)
            {
                min += actualTimeAlignMinute;
            }
            else
            {
                min += (min%actualTimeAlignMinute);
            }
            var hour = d.Hour;
            if (min >= 60)
            {
                ++hour;
                min = min%60;
                if (hour >= 24)
                {
                    d = new DateTimeOffset(
                        DateTime.Today.AddDays(1)
                    ).AddMinutes(min);
                }
                else
                {
                    d = new DateTimeOffset(
                        DateTime.Today
                    ).AddHours(hour).AddMinutes(min);
                }
            }
            else
            {
                d = new DateTimeOffset(
                            DateTime.Today
                ).AddHours(hour).AddMinutes(min);
            }
            return new ClockSettings(
                synchronizationPointActual : d
                , synchronizationPointVirtual : virtualStartPoint
                , speed : speed
            );
        }
        private Option<ClockSettings> settings = Option.None;
        private static readonly NLog.Logger logger = NLog.LogManager.GetLogger("");
        private void initLogs(string logFile)
        {
            var config = new NLog.Config.LoggingConfiguration();
            var layout = "${level}|Thread ${threadid}|${message}";
            if (logFile == null)
            {
                config.AddRuleForAllLevels(new NLog.Targets.ConsoleTarget {Layout = layout});
            }
            else
            {
                config.AddRuleForAllLevels(new NLog.Targets.FileTarget(logFile) {Layout = layout});
            }
            NLog.LogManager.Configuration = config;
        }
        public ClockEnv(string logFile=null)
        {
            initLogs(logFile);
        }
        public ClockEnv(ClockSettings settings, string logFile=null)
        {
            this.settings = settings;
            initLogs(logFile);
        }
        public DateTimeOffset actualToVirtual(DateTimeOffset d)
        {
            if (!settings.HasValue)
            {
                return d;
            }
            var s = settings.Unwrap();
            var t = s.synchronizationPointVirtualUnixMillis
                +Convert.ToInt64(
                    Math.Round(
                        (d.ToUnixTimeMilliseconds()-s.synchronizationPointActualUnixMillis)*s.speed))
                ;
            return DateTimeOffset.FromUnixTimeMilliseconds(t).ToLocalTime();
        }
        public DateTimeOffset virtualToActual(DateTimeOffset d)
        {
            if (!settings.HasValue)
            {
                return d;
            }
            var s = settings.Unwrap();
            var t = s.synchronizationPointActualUnixMillis
                +Convert.ToInt64(
                    Math.Round(
                        (d.ToUnixTimeMilliseconds()-s.synchronizationPointVirtualUnixMillis)*1.0/s.speed))
                ;
            return DateTimeOffset.FromUnixTimeMilliseconds(t).ToLocalTime();
        }
        public TimeSpan virtualDuration(TimeSpan actualD)
        {
            if (!settings.HasValue)
            {
                return actualD;
            }
            return TimeSpan.FromMilliseconds(Convert.ToInt64(Math.Round(actualD.TotalMilliseconds*settings.Unwrap().speed)));
        } 
        public TimeSpan actualDuration(TimeSpan virtualD)
        {
            if (!settings.HasValue)
            {
                return virtualD;
            }
            return TimeSpan.FromMilliseconds(Convert.ToInt64(Math.Round(virtualD.TotalMilliseconds*1.0/settings.Unwrap().speed)));
        } 
        public DateTimeOffset now()
        {
            return actualToVirtual(DateTimeOffset.Now);
        }
        public void log(Dev.CD606.TM.Infra.LogLevel level, string s)
        {
            var s1 = $"{formatTime(now())}|{s}";
            switch (level)
            {
                case Dev.CD606.TM.Infra.LogLevel.Trace:
                    logger.Trace(s1);
                    break;
                case Dev.CD606.TM.Infra.LogLevel.Debug:
                    logger.Debug(s1);
                    break;
                case Dev.CD606.TM.Infra.LogLevel.Info:
                    logger.Info(s1);
                    break;
                case Dev.CD606.TM.Infra.LogLevel.Warning:
                    logger.Warn(s1);
                    break;
                case Dev.CD606.TM.Infra.LogLevel.Error:
                    logger.Error(s1);
                    break;
                case Dev.CD606.TM.Infra.LogLevel.Critical:
                    logger.Fatal(s1);
                    break;
                default:
                    break;
            }
        }
        public void exit()
        {
            Environment.Exit(0);
        }

        public string formatTime(DateTimeOffset d)
        {
            return d.ToString("yyyy-MM-dd HH:mm:ss.ffffff");
        }
    }

    public static class ClockImporter<Env> where Env : ClockEnv
    {
        public static AbstractImporter<Env,T> createOneShotClockImporter<T>(DateTimeOffset when, Func<DateTimeOffset,T> gen) 
        {
            return RealTimeAppUtils<Env>.simpleImporter<T>(
                (Env env, Action<T,bool> pub) => {
                    var now = env.now();
                    if (now <= when)
                    {
                        var t = new Timer(env.actualDuration(when-now).TotalMilliseconds);
                        t.Elapsed += (object Source, ElapsedEventArgs args) => {
                            pub(gen(env.now()), true);
                        };
                        t.AutoReset = false;
                        t.Enabled = true;
                    }
                }
            );
        }
        public static AbstractImporter<Env,T> createOneShotClockConstImporter<T>(DateTimeOffset when, T t) 
        {
            return createOneShotClockImporter<T>(when, (DateTimeOffset d) => t);
        }
        public static AbstractImporter<Env,T> createRecurringClockImporter<T>(DateTimeOffset start, DateTimeOffset end, long periodMs, Func<DateTimeOffset,T> gen) 
        {
            return RealTimeAppUtils<Env>.simpleImporter<T>(
                (Env env, Action<T,bool> pub) => {
                    var now = env.now();
                    var realStart = start;
                    while (now > realStart)
                    {
                        realStart = realStart.AddMilliseconds(periodMs);
                    }
                    if (realStart < end)
                    {
                        var actualPeriod = env.actualDuration(TimeSpan.FromMilliseconds(periodMs)).TotalMilliseconds;
                        var timer = new Timer(env.actualDuration(realStart-now).TotalMilliseconds);
                        timer.Elapsed += (object Source, ElapsedEventArgs args) => {
                            timer.Interval = actualPeriod;
                            var current = env.now();
                            var isFinal = (current.AddMilliseconds(periodMs) > end);
                            if (isFinal)
                            {
                                timer.Enabled = false;
                            }
                            pub(gen(current), isFinal);
                        };
                        timer.AutoReset = true;
                        timer.Enabled = true;
                    }
                }
            );
        }
        public static AbstractImporter<Env,T> createRecurringClockConstImporter<T>(DateTimeOffset start, DateTimeOffset end, long periodMs, T t) 
        {
            return createRecurringClockImporter<T>(start, end, periodMs, (DateTimeOffset d) => t);
        }
        public static AbstractImporter<Env,T> createVariableDurationRecurringClockImporter<T>(DateTimeOffset start, DateTimeOffset end, Func<DateTimeOffset,long> periodCalc, Func<DateTimeOffset,T> gen) 
        {
            return RealTimeAppUtils<Env>.simpleImporter<T>(
                (Env env, Action<T,bool> pub) => {
                    var now = env.now();
                    var realStart = start;
                    var firstPeriod = periodCalc(start);
                    while (now > realStart)
                    {
                        realStart = realStart.AddMilliseconds(firstPeriod);
                    }
                    if (realStart < end)
                    {
                        var timer = new Timer(env.actualDuration(realStart-now).TotalMilliseconds);
                        timer.Elapsed += (object Source, ElapsedEventArgs args) => {
                            var current = env.now();
                            var newPeriod = periodCalc(current);
                            timer.Interval = env.actualDuration(TimeSpan.FromMilliseconds(newPeriod)).TotalMilliseconds;
                            var isFinal = (current.AddMilliseconds(newPeriod) > end);
                            if (isFinal)
                            {
                                timer.Enabled = false;
                            }
                            pub(gen(current), isFinal);
                        };
                        timer.AutoReset = true;
                        timer.Enabled = true;
                    }
                }
            );
        }
        public static AbstractImporter<Env,T> createVariableDurationRecurringConstClockImporter<T>(DateTimeOffset start, DateTimeOffset end, Func<DateTimeOffset,long> periodCalc, T t)
        {
            return createVariableDurationRecurringClockImporter<T>(start, end, periodCalc, (DateTimeOffset d) => t);
        }
    }

    public readonly struct ClockOnOrderFacilityInput<T>
    {
        public readonly T inputData;
        public readonly List<TimeSpan> durations;
        public ClockOnOrderFacilityInput(T inputData, List<TimeSpan> durations)
        {
            this.inputData = inputData;
            this.durations = durations;
        }
    }
    public static class ClockOnOrderFacility<Env> where Env : ClockEnv
    {
        public class ClockCallback<S,T> : AbstractOnOrderFacility<Env,ClockOnOrderFacilityInput<S>,T>
        {
            private Func<DateTimeOffset,int,int,T> converter;
            public ClockCallback(Func<DateTimeOffset,int,int,T> converter)
            {
                this.converter = converter;
            }
            public override void start(Env env)
            {
            }
            public override void handle(TimedDataWithEnvironment<Env, Key<ClockOnOrderFacilityInput<S>>> data)
            {
                var filteredDurations = data.timedData.value.key.durations
                    .Where((TimeSpan t) => (t.TotalMilliseconds >= 0))
                    .OrderBy((TimeSpan t) => t);
                var id = data.timedData.value.id;
                var env = data.environment;
                var count = filteredDurations.Count();
                if (count == 0)
                {
                    var now = env.now();
                    var t = converter(now, 0, 0);
                    publish(new TimedDataWithEnvironment<Env, Key<T>>(
                        env
                        , new WithTime<Key<T>>(
                            now
                            , new Key<T>(
                                id, t
                            )
                            , true
                        )
                    ));
                }
                else
                {
                    var ii = 0;
                    foreach (var d in filteredDurations)
                    {
                        var timer = new Timer(env.actualDuration(d).TotalMilliseconds);
                        var currentIdx = ii;
                        timer.Elapsed += (object Source, ElapsedEventArgs args) => {
                            var current = env.now();
                            var t = converter(current, currentIdx, count);
                            publish(new TimedDataWithEnvironment<Env, Key<T>>(
                                env
                                , new WithTime<Key<T>>(
                                    current
                                    , new Key<T>(
                                        id, t
                                    )
                                    , (currentIdx == count-1)
                                )
                            ));
                        };
                        timer.AutoReset = false;
                        timer.Enabled = true;
                        ++ii;
                    }
                }       
            }
        }
        public static AbstractOnOrderFacility<Env,ClockOnOrderFacilityInput<S>,T>
            createClockCallback<S,T>(Func<DateTimeOffset,int,int,T> converter)
            {
                return new ClockCallback<S,T>(converter);
            }
    }
}
