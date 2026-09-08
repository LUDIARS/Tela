// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;
using System.IO;
using System.Text;

namespace Tela.Editor
{
    // Identical framing on all hosts: LE integers, IEEE754 floats, bounded UTF-8.
    internal sealed class BridgeWire
    {
        internal readonly ulong Generation = BitConverter.ToUInt64(Guid.NewGuid().ToByteArray(), 0) | 1UL;
        internal readonly string Host;
        internal readonly string View;
        private ulong sequence;
        internal ulong Revision;
        internal BridgeWire(string host, string view) { Host = host; View = view; }
        internal byte[] Message(ushort kind, Action<BinaryWriter> payload)
        {
            using (var stream = new MemoryStream())
            using (var writer = new BinaryWriter(stream, new UTF8Encoding(false, true)))
            {
                writer.Write((uint)0); writer.Write((uint)0x31574c54); writer.Write((ushort)1); writer.Write(kind);
                writer.Write(Generation); writer.Write(++sequence); writer.Write(Revision);
                Text(writer, Host); Text(writer, View); payload?.Invoke(writer);
                if (stream.Length - 4 > 65536) throw new InvalidDataException("Tela frame exceeds 64 KiB");
                stream.Position = 0; writer.Write((uint)(stream.Length - 4)); return stream.ToArray();
            }
        }
        internal static void Text(BinaryWriter writer, string value)
        {
            byte[] bytes = new UTF8Encoding(false, true).GetBytes(value ?? "");
            if (bytes.Length > 4096) throw new InvalidDataException("Tela string exceeds 4096 bytes");
            writer.Write((ushort)bytes.Length); writer.Write(bytes);
        }
    }
}
