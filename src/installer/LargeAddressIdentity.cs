using System;
using System.IO;
using System.Security.Cryptography;

namespace FfxScreenReader.Setup
{
    internal static class LargeAddressIdentity
    {
        private const int MaximumChunkSize = 65536;

        internal static bool MatchesFlagOnly(Stream input, string expectedSha1)
        {
            if (input == null || !input.CanRead || !input.CanSeek ||
                String.IsNullOrEmpty(expectedSha1) || expectedSha1.Length != 40)
                return false;

            long originalPosition;
            long length;
            try
            {
                originalPosition = input.Position;
                length = input.Length;
            }
            catch (IOException) { return false; }
            catch (NotSupportedException) { return false; }

            try
            {
                long characteristicsOffset;
                if (!TryGetCharacteristicsOffset(input, length, out characteristicsOffset))
                    return false;

                byte[] expected;
                try { expected = ParseSha1(expectedSha1); }
                catch (FormatException) { return false; }

                return HashMatches(input, length, characteristicsOffset, false, expected) ||
                    HashMatches(input, length, characteristicsOffset, true, expected);
            }
            finally
            {
                input.Position = originalPosition;
            }
        }

        private static bool TryGetCharacteristicsOffset(Stream input, long length, out long offset)
        {
            offset = 0;
            if (length < 64) return false;
            byte[] dos = new byte[64];
            if (!ReadAt(input, 0, dos, 0, dos.Length) || dos[0] != (byte)'M' || dos[1] != (byte)'Z')
                return false;

            uint peOffset = ReadUInt32(dos, 0x3c);
            if (peOffset < 64 || peOffset > (ulong)length || (ulong)length - peOffset < 24)
                return false;
            long pe = (long)peOffset;
            byte[] coffAndOptional = new byte[24];
            if (!ReadAt(input, pe, coffAndOptional, 0, coffAndOptional.Length) ||
                coffAndOptional[0] != (byte)'P' || coffAndOptional[1] != (byte)'E' ||
                coffAndOptional[2] != 0 || coffAndOptional[3] != 0)
                return false;

            if (ReadUInt16(coffAndOptional, 4) != 0x014c) return false;
            int optionalSize = ReadUInt16(coffAndOptional, 20);
            if (optionalSize < 0x60) return false;
            long optionalOffset = pe + 24;
            if (optionalOffset > length || optionalSize > length - optionalOffset) return false;
            byte[] magic = new byte[2];
            if (!ReadAt(input, optionalOffset, magic, 0, magic.Length) ||
                ReadUInt16(magic, 0) != 0x010b)
                return false;

            offset = pe + 22;
            return true;
        }

        private static bool HashMatches(Stream input, long length, long flagOffset, bool set, byte[] expected)
        {
            input.Position = 0;
            byte[] buffer = new byte[MaximumChunkSize];
            using (SHA1 sha = SHA1.Create())
            {
                long position = 0;
                while (position < length)
                {
                    int wanted = (int)Math.Min((long)buffer.Length, length - position);
                    int read = 0;
                    while (read < wanted)
                    {
                        int count = input.Read(buffer, read, wanted - read);
                        if (count <= 0) return false;
                        read += count;
                    }

                    long lowByteOffset = flagOffset;
                    if (lowByteOffset >= position && lowByteOffset < position + read)
                    {
                        int index = (int)(lowByteOffset - position);
                        buffer[index] = set
                            ? (byte)(buffer[index] | 0x20)
                            : (byte)(buffer[index] & 0xdf);
                    }

                    sha.TransformBlock(buffer, 0, read, buffer, 0);
                    position += read;
                }

                sha.TransformFinalBlock(new byte[0], 0, 0);
                return FixedTimeEqual(sha.Hash, expected);
            }
        }

        private static bool ReadAt(Stream input, long position, byte[] buffer, int offset, int count)
        {
            input.Position = position;
            int read = 0;
            while (read < count)
            {
                int amount = input.Read(buffer, offset + read, count - read);
                if (amount <= 0) return false;
                read += amount;
            }
            return true;
        }

        private static uint ReadUInt32(byte[] data, int offset)
        {
            return (uint)(data[offset] | (data[offset + 1] << 8) |
                (data[offset + 2] << 16) | (data[offset + 3] << 24));
        }

        private static int ReadUInt16(byte[] data, int offset)
        {
            return data[offset] | (data[offset + 1] << 8);
        }

        private static byte[] ParseSha1(string value)
        {
            byte[] result = new byte[20];
            for (int i = 0; i < result.Length; i++)
                result[i] = Convert.ToByte(value.Substring(i * 2, 2), 16);
            return result;
        }

        private static bool FixedTimeEqual(byte[] left, byte[] right)
        {
            if (left == null || right == null || left.Length != right.Length) return false;
            int difference = 0;
            for (int i = 0; i < left.Length; i++) difference |= left[i] ^ right[i];
            return difference == 0;
        }
    }
}
