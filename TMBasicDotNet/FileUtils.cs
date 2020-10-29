using System;
using System.IO;
using System.Text;
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
            }
        }
        public static AbstractExporter<Env,ByteDataWithTopic> 
        byteDataWithTopicOutput(string fileName, ByteData filePrefix=null, ByteData recordPrefix=null)
        {
            return new ByteDataWithTopicOutput(fileName,filePrefix?.content,recordPrefix?.content);
        }
    }
}