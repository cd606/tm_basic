using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using Dev.CD606.TM.Infra;
using Dev.CD606.TM.Infra.RealTimeApp;

namespace Dev.CD606.TM.Basic
{
    public static class FileUtils<Env> where Env : EnvBase
    {
        class ByteDataWithTopicOutput : AbstractExporter<Env,ByteDataWithTopic>
        {
            private BinaryWriter writer;
            private string fileName;
            private byte[] filePrefix;
            private byte[] recordPrefix;
            public ByteDataWithTopicOutput(string fileName, byte[] filePrefix, byte[] recordPrefix)
            {
                this.fileName = fileName;
                this.filePrefix = filePrefix;
                this.recordPrefix = recordPrefix;
            }
            public void start(Env env)
            {
                writer = new BinaryWriter(File.OpenWrite(fileName));
                if (filePrefix != null) 
                {
                    writer.Write(filePrefix);
                    writer.Flush();
                }
            }
            public void handle(TimedDataWithEnvironment<Env,ByteDataWithTopic> data)
            {
                int prefixLen = 0;
                if (recordPrefix != null)
                {
                    prefixLen = recordPrefix.Length;
                }
                var v = data.timedData.value;
                var buffer = new byte[prefixLen+8+4+v.topic.Length+4+v.content.Length+1];
                if (recordPrefix != null)
                {
                    Buffer.BlockCopy(recordPrefix, 0, buffer, 0, prefixLen);
                }
                var b = BitConverter.GetBytes(
                    ((UInt64) data.timedData.timePoint*1000)
                );
                if (!BitConverter.IsLittleEndian)
                {
                    Array.Reverse(b);
                }
                Buffer.BlockCopy(b, 0, buffer, prefixLen, 8);
                var topicBuffer = Encoding.UTF8.GetBytes(v.topic);
                b = BitConverter.GetBytes((UInt32) topicBuffer.Length);
                if (!BitConverter.IsLittleEndian)
                {
                    Array.Reverse(b);
                }
                Buffer.BlockCopy(b, 0, buffer, prefixLen+8, 4);
                Buffer.BlockCopy(topicBuffer, 0, buffer, prefixLen+12, topicBuffer.Length);
                b = BitConverter.GetBytes((UInt32) v.content.Length);
                if (!BitConverter.IsLittleEndian)
                {
                    Array.Reverse(b);
                }
                Buffer.BlockCopy(b, 0, buffer, prefixLen+12+topicBuffer.Length, 4);
                Buffer.BlockCopy(v.content, 0, buffer, prefixLen+16+topicBuffer.Length, v.content.Length);
                buffer[buffer.Length-1] = (data.timedData.finalFlag?(byte) 0x1:(byte) 0x0);
                writer.Write(buffer);
                writer.Flush();
            }
        }
        public static AbstractExporter<Env,ByteDataWithTopic> 
        byteDataWithTopicOutput(string fileName, ByteData filePrefix=null, ByteData recordPrefix=null)
        {
            return new ByteDataWithTopicOutput(fileName,filePrefix?.content,recordPrefix?.content);
        }
    }

    public static class RecordFileUtils
    {
        public interface RecordReader<T> 
        {
            int Start(BinaryReader r);
            int ReadOne(BinaryReader r, out T result);
        }
        public static IEnumerable<T> GenericRecordDataSource<T>(BinaryReader r, RecordReader<T> reader) 
        {
            if (reader.Start(r) < 0)
            {
                yield break;
            }
            else
            {
                T t;
                while (true)
                {
                    if (reader.ReadOne(r, out t) < 0)  
                    {
                        yield break;
                    }
                    else
                    {
                        yield return t;
                    }
                }
            }
        }
        public class TopicCaptureFileRecord 
        {
            public DateTimeOffset Time {get; set;}
            public string TimeString {get; set;}
            public string Topic {get; set;}
            public byte[] Data {get; set;}
            public bool IsFinal {get; set;}
        }
        public class TopicCaptureFileRecordReaderOption 
        {
            public enum TimePrecisionLevel
            {
                Second
                , Millisecond
                , Microsecond
            }
            public ushort FileMagicLength {get; set;} = 4;
            public ushort RecordMagicLength {get; set;} = 4; 
            public ushort TimeFieldLength {get; set;} = 8;
            public ushort TopicLengthFieldLength {get; set;} = 4;
            public ushort DataLengthFieldLength {get; set;} = 4;
            public bool HasFinalFlagField {get; set;} = true;
            public TimePrecisionLevel TimePrecision {get; set;} = TimePrecisionLevel.Microsecond;
        }
        public class TopicCaptureFileRecordReader : RecordReader<TopicCaptureFileRecord>
        {
            private TopicCaptureFileRecordReaderOption option;
            private byte[] numberBuffer;
            public TopicCaptureFileRecordReader(TopicCaptureFileRecordReaderOption option)
            {
                this.option = option;
                this.numberBuffer = new byte[8];
            }
            public int Start(BinaryReader r)
            {
                if (option.FileMagicLength == 0)
                {
                    return 0;
                }
                if (r.ReadBytes(option.FileMagicLength).Length < option.FileMagicLength)
                {
                    return -1;
                }
                return option.FileMagicLength;
            }
            public int ReadOne(BinaryReader r, out TopicCaptureFileRecord output)
            {
                output = null;
                int count = 0;
                if (option.RecordMagicLength > 0)
                {
                    if (r.ReadBytes(option.RecordMagicLength).Length < option.RecordMagicLength)
                    {
                        return -1;
                    }
                    count += option.RecordMagicLength;
                }
                output = new TopicCaptureFileRecord();
                if (option.TimeFieldLength > 0)
                {
                    var timeBytes = r.ReadBytes(option.TimeFieldLength);
                    if (timeBytes.Length < option.TimeFieldLength)
                    {
                        return -1;
                    }
                    count += timeBytes.Length;
                    Int64 timeInt = 0;
                    if (timeBytes.Length != 8)
                    {
                        Array.Clear(numberBuffer, 0, 8);
                        Buffer.BlockCopy(timeBytes, 0, numberBuffer, 0, Math.Min(timeBytes.Length, 8));
                        timeInt = BitConverter.ToInt64(numberBuffer, 0);
                    }
                    else
                    {
                        timeInt = BitConverter.ToInt64(timeBytes, 0);
                    }
                    switch (option.TimePrecision)
                    {
                        case TopicCaptureFileRecordReaderOption.TimePrecisionLevel.Second:
                            {
                                output.Time = DateTimeOffset.FromUnixTimeSeconds(timeInt).ToLocalTime();
                                output.TimeString = output.Time.ToString("yyyy-MM-dd HH:mm:ss");
                            }
                            break;
                        case TopicCaptureFileRecordReaderOption.TimePrecisionLevel.Millisecond:
                            {
                                output.Time = DateTimeOffset.FromUnixTimeMilliseconds(timeInt).ToLocalTime();
                                output.TimeString = output.Time.ToString("yyyy-MM-dd HH:mm:ss.fff");
                            }
                            break;
                        case TopicCaptureFileRecordReaderOption.TimePrecisionLevel.Microsecond:
                        default:
                            {
                                output.Time = DateTimeOffset.FromUnixTimeMilliseconds(timeInt/1000);
                                output.Time = output.Time.AddTicks((timeInt%1000)*10);
                                output.Time = output.Time.ToLocalTime();
                                output.TimeString = output.Time.ToString("yyyy-MM-dd HH:mm:ss.ffffff");
                            }
                            break;
                    }
                }
                if (option.TopicLengthFieldLength > 0)
                {
                    var topicLenBytes = r.ReadBytes(option.TopicLengthFieldLength);
                    if (topicLenBytes.Length < option.TopicLengthFieldLength)
                    {
                        return -1;
                    }
                    count += topicLenBytes.Length;
                    Int64 topicLenL = 0;
                    if (topicLenBytes.Length != 8)
                    {
                        Array.Clear(numberBuffer, 0, 8);
                        Buffer.BlockCopy(topicLenBytes, 0, numberBuffer, 0, Math.Min(topicLenBytes.Length, 8));
                        topicLenL = BitConverter.ToInt64(numberBuffer, 0);
                    }
                    else
                    {
                        topicLenL = BitConverter.ToInt64(topicLenBytes, 0);
                    }
                    int topicLen = (int) topicLenL;
                    var topic = r.ReadBytes(topicLen);
                    if (topic.Length != topicLen)
                    {
                        return -1;
                    }
                    count += topic.Length;
                    output.Topic = System.Text.Encoding.UTF8.GetString(topic);
                }
                if (option.DataLengthFieldLength > 0)
                {
                    var dataLenBytes = r.ReadBytes(option.DataLengthFieldLength);
                    if (dataLenBytes.Length < option.DataLengthFieldLength)
                    {
                        return -1;
                    }
                    count += dataLenBytes.Length;
                    Int64 dataLenL = 0;
                    if (dataLenBytes.Length != 8)
                    {
                        Array.Clear(numberBuffer, 0, 8);
                        Buffer.BlockCopy(dataLenBytes, 0, numberBuffer, 0, Math.Min(dataLenBytes.Length, 8));
                        dataLenL = BitConverter.ToInt64(numberBuffer, 0);
                    }
                    else
                    {
                        dataLenL = BitConverter.ToInt64(dataLenBytes, 0);
                    }
                    int dataLen = (int) dataLenL;
                    var data = r.ReadBytes(dataLen);
                    if (data.Length != dataLen)
                    {
                        return -1;
                    }
                    count += data.Length;
                    output.Data = data;
                }
                if (option.HasFinalFlagField)
                {
                    var b = r.ReadBytes(1);
                    if (b.Length != 1)
                    {
                        return -1;
                    }
                    count += 1;
                    output.IsFinal = (b[0] != 0);
                }
                return count;
            }
        }
        public class TopicCaptureFileReplayImporter<Env> : AbstractImporter<Env, ByteDataWithTopic> where Env: ClockEnv
        {
            private BinaryReader reader;
            private TopicCaptureFileRecordReaderOption option;
            private bool overrideDate;
            TopicCaptureFileReplayImporter(BinaryReader reader, TopicCaptureFileRecordReaderOption option, bool overrideDate=false)
            {
                this.reader = reader;
                this.option = option;
                this.overrideDate = overrideDate;
            }
            public override void start(Env env)
            {
                new System.Threading.Thread(
                    () => {
                        DateTimeOffset todayStart;
                        bool todayStartSet = false;
                        foreach (var item in GenericRecordDataSource<TopicCaptureFileRecord>(reader, new TopicCaptureFileRecordReader(option)))
                        {
                            var now = env.now();
                            if (overrideDate)
                            {
                                if (!todayStartSet)
                                {
                                    todayStart = new DateTimeOffset(now.Date);
                                    todayStartSet = true;
                                }
                                item.Time = todayStart+(item.Time-item.Time.Date);
                            }
                            if (item.Time < now)
                            {
                                if ((now-item.Time) < TimeSpan.FromSeconds(1))
                                {
                                    publish(new TimedDataWithEnvironment<Env, ByteDataWithTopic>(
                                        env, new WithTime<ByteDataWithTopic>(item.Time, new ByteDataWithTopic(item.Topic, item.Data), item.IsFinal)
                                    ));
                                }
                                else 
                                {
                                    continue;
                                }
                            }
                            else if (item.Time == now)
                            {
                                publish(new TimedDataWithEnvironment<Env, ByteDataWithTopic>(
                                    env, new WithTime<ByteDataWithTopic>(item.Time, new ByteDataWithTopic(item.Topic, item.Data), item.IsFinal)
                                ));
                            }
                            else 
                            {
                                System.Threading.Thread.Sleep(env.actualDuration(item.Time-now));
                                publish(new TimedDataWithEnvironment<Env, ByteDataWithTopic>(
                                    env, new WithTime<ByteDataWithTopic>(item.Time, new ByteDataWithTopic(item.Topic, item.Data), item.IsFinal)
                                ));
                            }
                        }
                    }
                ).Start();
            }
        }
    }
}