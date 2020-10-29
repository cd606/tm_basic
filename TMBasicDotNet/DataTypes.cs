namespace Dev.CD606.TM.Basic
{
    public readonly struct VoidStruct 
    {
    }

    public class ByteData
    {
        public readonly byte[] content;
        public ByteData(byte[] content)
        {
            this.content = content;
        } 
    }

    public class ByteDataWithTopic
    {
        public readonly string topic;
        public readonly byte[] content;
        public ByteDataWithTopic(string topic, byte[] content)
        {
            this.topic = topic;
            this.content = content;
        }
    }
    public class TypedDataWithTopic<T>
    {
        public readonly string topic;
        public readonly T content;
        public TypedDataWithTopic(string topic, T content)
        {
            this.topic = topic;
            this.content = content;
        }
    }
}