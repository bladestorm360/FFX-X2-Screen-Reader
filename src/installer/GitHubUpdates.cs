using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Net.Cache;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Web.Script.Serialization;

namespace FfxScreenReader.Setup
{
    internal sealed class GitHubRelease
    {
        public readonly string Tag, Digest;
        public readonly Version Version;
        public readonly Uri Download;
        public readonly int Size;

        public GitHubRelease(string tag, Version version, Uri download, int size, string digest)
        {
            Tag = tag;
            Version = version;
            Download = download;
            Size = size;
            Digest = digest;
        }
    }

    internal static class GitHubUpdates
    {
        private const string Repository = "bladestorm360/FFX-X2-Screen-Reader";
        private const int MaxReleaseBytes = 1024 * 1024;
        private const int MaxPackageBytes = 64 * 1024 * 1024;
        private const int MaxChecksumBytes = 8192;
        private static readonly Uri Latest = new Uri("https://api.github.com/repos/" + Repository + "/releases/latest");
        private static readonly UTF8Encoding Utf8 = new UTF8Encoding(false, true);

        public static GitHubRelease Check()
        {
            return ParseLatest(Utf8.GetString(Download(Latest, MaxReleaseBytes, false)));
        }

        internal static GitHubRelease ParseLatest(string json)
        {
            var serializer = new JavaScriptSerializer { MaxJsonLength = MaxReleaseBytes, RecursionLimit = 24 };
            var release = serializer.DeserializeObject(json) as Dictionary<string, object>;
            if (release == null || !IsFalse(release, "draft") || !IsFalse(release, "prerelease"))
                throw new InvalidDataException("GitHub did not return a published stable release.");
            string tag = Text(release, "tag_name");
            Version number;
            if (!Regex.IsMatch(tag, @"\Av[0-9]{1,5}(?:\.[0-9]{1,5}){1,3}\z") || !Version.TryParse(tag.Substring(1), out number))
                throw new InvalidDataException("GitHub returned an unsupported release tag.");
            number = new Version(number.Major, number.Minor, Math.Max(0, number.Build), Math.Max(0, number.Revision));
            string name = "ffx-x2-screen-reader-" + tag.Substring(1) + "-payload.zip";
            var download = new Uri("https://github.com/" + Repository + "/releases/download/" + tag + "/" + name);
            object assetsValue;
            var assets = release.TryGetValue("assets", out assetsValue) ? assetsValue as object[] : null;
            GitHubRelease result = null;
            foreach (object value in assets ?? new object[0])
            {
                var asset = value as Dictionary<string, object>;
                if (asset == null || Text(asset, "name") != name) continue;
                object sizeValue;
                int size = asset.TryGetValue("size", out sizeValue) && sizeValue is int ? (int)sizeValue : 0;
                string digest = Text(asset, "digest");
                if (result != null || Text(asset, "state") != "uploaded" ||
                    Text(asset, "browser_download_url") != download.AbsoluteUri || size <= 0 || size > MaxPackageBytes ||
                    !Regex.IsMatch(digest, @"\Asha256:[a-fA-F0-9]{64}\z"))
                    throw new InvalidDataException("The GitHub release has invalid or ambiguous ZIP download information.");
                result = new GitHubRelease(tag, number, download, size, digest.Substring(7).ToLowerInvariant());
            }
            if (result == null) throw new InvalidDataException("The latest stable GitHub release has no setup payload. Expected " + name + ".");
            return result;
        }

        private static bool IsFalse(Dictionary<string, object> fields, string key)
        {
            object value;
            return fields.TryGetValue(key, out value) && value is bool && !(bool)value;
        }

        private static string Text(Dictionary<string, object> fields, string key)
        {
            object value;
            return fields.TryGetValue(key, out value) ? value as string ?? "" : "";
        }

        internal static Uri AssetRedirect(string location)
        {
            Uri uri;
            if (!Uri.TryCreate(location, UriKind.Absolute, out uri) || uri.Scheme != Uri.UriSchemeHttps ||
                !uri.IsDefaultPort || uri.UserInfo.Length != 0 || uri.Fragment.Length != 0 ||
                (uri.Host != "release-assets.githubusercontent.com" && uri.Host != "objects.githubusercontent.com"))
                throw new InvalidDataException("GitHub sent the update to an unexpected download location.");
            return uri;
        }

        private static byte[] Download(Uri uri, int maxBytes, bool asset)
        {
            using (var deadline = new CancellationTokenSource(TimeSpan.FromSeconds(35)))
            {
                for (int hop = 0; hop <= 2; hop++)
                {
                    var request = (HttpWebRequest)WebRequest.Create(uri);
                    request.AllowAutoRedirect = false;
                    request.Timeout = 10000;
                    request.ReadWriteTimeout = 10000;
                    request.UserAgent = "FFX-X2-Screen-Reader-Setup";
                    request.CachePolicy = new RequestCachePolicy(RequestCacheLevel.NoCacheNoStore);
                    request.Accept = asset ? "application/octet-stream" : "application/vnd.github+json";
                    if (!asset) request.Headers["X-GitHub-Api-Version"] = "2022-11-28";
                    using (deadline.Token.Register(request.Abort))
                    using (var response = (HttpWebResponse)request.GetResponse())
                    {
                        int code = (int)response.StatusCode;
                        if (asset && (code == 301 || code == 302 || code == 303 || code == 307 || code == 308))
                        {
                            uri = AssetRedirect(response.Headers["Location"]);
                            continue;
                        }
                        if (response.StatusCode != HttpStatusCode.OK || response.ContentLength > maxBytes)
                            throw new InvalidDataException("GitHub returned an unexpected response or download size.");
                        using (Stream stream = response.GetResponseStream()) return ReadBounded(stream, maxBytes);
                    }
                }
            }
            throw new InvalidDataException("GitHub sent too many update redirects.");
        }

        private static byte[] ReadBounded(Stream source, int maxBytes)
        {
            using (var result = new MemoryStream())
            {
                var buffer = new byte[8192];
                int read;
                while ((read = source.Read(buffer, 0, buffer.Length)) != 0)
                {
                    if (result.Length + read > maxBytes) throw new InvalidDataException("The update download or archive entry exceeds its size limit.");
                    result.Write(buffer, 0, read);
                }
                return result.ToArray();
            }
        }

        public static Func<string, byte[]> FetchPayload(GitHubRelease release)
        {
            return OpenPayload(release, Download(release.Download, release.Size, true));
        }

        internal static Func<string, byte[]> OpenPayload(GitHubRelease release, byte[] bytes)
        {
            if (bytes.Length != release.Size || InstallerEngine.Hash(bytes) != release.Digest)
                throw new InvalidDataException("The downloaded ZIP does not match the size and SHA-256 published by GitHub.");
            return OpenPackage(bytes, release.Tag);
        }

        // Read the same bounded data layout during release verification and on
        // the user's PC. No archive entry is extracted to a user-supplied path.
        internal static Func<string, byte[]> OpenPackage(byte[] bytes, string tag)
        {
            if (bytes.Length > MaxPackageBytes) throw new InvalidDataException("The release ZIP exceeds its size limit.");
            var payload = new Dictionary<string, byte[]>(StringComparer.Ordinal);
            using (var zip = new ZipArchive(new MemoryStream(bytes, false), ZipArchiveMode.Read))
            {
                const int expectedRuntimeFiles = 15;
                if (zip.Entries.Count != 3 + InstallerEngine.PayloadNames.Length + expectedRuntimeFiles)
                    throw new InvalidDataException("The GitHub release ZIP has the wrong file count.");
                var entries = new Dictionary<string, ZipArchiveEntry>(StringComparer.Ordinal);
                foreach (ZipArchiveEntry entry in zip.Entries)
                {
                    if (entries.ContainsKey(entry.FullName))
                        throw new InvalidDataException("The GitHub release ZIP contains a duplicate entry.");
                    entries.Add(entry.FullName, entry);
                }
                int remaining = MaxPackageBytes;
                if (Utf8.GetString(ReadEntry(entries, "release-version.txt", 64, ref remaining)) != tag)
                    throw new InvalidDataException("The release ZIP version does not match the GitHub tag.");
                payload.Add("SHA256SUMS.txt", ReadEntry(entries, "SHA256SUMS.txt", MaxChecksumBytes, ref remaining));
                byte[] sourceLock = ReadEntry(entries, "prism.lock.json", MaxReleaseBytes, ref remaining);
                payload.Add("prism.lock.json", sourceLock);
                var runtime = PrismPrerequisite.ParseLock(sourceLock);
                if (runtime.Files.Count != expectedRuntimeFiles)
                    throw new InvalidDataException("The release ZIP has an unsupported PRISM file inventory.");
                foreach (string name in runtime.Files.Keys)
                    payload.Add("PrismRuntime." + name, ReadEntry(entries, "PrismRuntime/" + name, remaining, ref remaining));
                foreach (string name in InstallerEngine.PayloadNames)
                {
                    byte[] file = ReadEntry(entries, "install/" + name, remaining, ref remaining);
                    if (name.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) && !InstallerEngine.IsX86Image(file))
                        throw new InvalidDataException("The release contains an invalid x86 DLL: " + name + ".");
                    payload.Add(name, file);
                }
            }
            return name =>
            {
                byte[] value;
                if (!payload.TryGetValue(name, out value))
                    throw new InvalidDataException("The release ZIP is missing a payload file: " + name + ".");
                return value;
            };
        }

        private static byte[] ReadEntry(Dictionary<string, ZipArchiveEntry> entries, string name, int limit, ref int remaining)
        {
            ZipArchiveEntry entry;
            if (!entries.TryGetValue(name, out entry))
                throw new InvalidDataException("The GitHub release ZIP is missing " + name + ".");
            if (entry.Length < 1 || entry.Length > limit || entry.Length > remaining)
                throw new InvalidDataException("The GitHub release ZIP contains an invalid size for " + name + ".");
            byte[] data;
            using (Stream stream = entry.Open()) data = ReadBounded(stream, (int)entry.Length);
            if (data.Length != entry.Length) throw new InvalidDataException("Truncated release ZIP entry: " + name + ".");
            remaining -= data.Length;
            return data;
        }
    }
}
