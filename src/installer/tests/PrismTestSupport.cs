using System;
using FfxScreenReader.Setup;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Web.Script.Serialization;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

internal static class PrismTestSupport
{
    private sealed class RuntimeSnapshot
    {
        public readonly Dictionary<string, string> Hashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        public readonly Dictionary<string, long> Times = new Dictionary<string, long>(StringComparer.OrdinalIgnoreCase);
    }
    private static readonly Dictionary<string, RuntimeSnapshot> snapshots = new Dictionary<string, RuntimeSnapshot>(StringComparer.OrdinalIgnoreCase);

    public static byte[] Resource(string name)
    {
        if (name == "prism.lock.json") return LockBytes();
        const string prefix = "PrismRuntime.";
        if (!name.StartsWith(prefix, StringComparison.Ordinal)) throw new InvalidDataException("Unknown Prism resource");
        string package = Environment.GetEnvironmentVariable("FFX_PRISM_TEST_PACKAGE");
        return File.ReadAllBytes(InstallerPaths.InGame(package, name.Substring(prefix.Length)));
    }

    public static byte[] LockBytes()
    {
        string path = Environment.GetEnvironmentVariable("FFX_PRISM_TEST_LOCK");
        if (String.IsNullOrWhiteSpace(path)) throw new InvalidOperationException("FFX_PRISM_TEST_LOCK is required.");
        return System.IO.File.ReadAllBytes(path);
    }

    public static void Prepare(string game)
    {
        byte[] lockBytes = LockBytes();
        var serializer = new JavaScriptSerializer();
        var source = serializer.DeserializeObject(Encoding.UTF8.GetString(lockBytes).TrimStart('\uFEFF')) as Dictionary<string, object>;
        if (source == null) throw new InvalidDataException("Prism test lock must be a JSON object.");
        string packageId = StringValue(source, "package_id");
        string version = StringValue(source, "version");
        string commit = StringValue(source, "source_commit");
        var entries = source["files"] as Dictionary<string, object>;
        if (entries == null || entries.Count == 0) throw new InvalidDataException("Prism test lock has no files.");
        string package = Environment.GetEnvironmentVariable("FFX_PRISM_TEST_PACKAGE");
        if (String.IsNullOrWhiteSpace(package)) throw new InvalidOperationException("FFX_PRISM_TEST_PACKAGE is required.");
        string runtime = InstallerPaths.InGame(game, "PrismRuntime");
        var fileHashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        var verified = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
        foreach (KeyValuePair<string, object> pair in entries)
        {
            var descriptor = pair.Value as Dictionary<string, object>;
            if (descriptor == null) throw new InvalidDataException("Invalid locked Prism file: " + pair.Key);
            string expected = StringValue(descriptor, "sha256");
            string sourceName = StringValue(descriptor, "source");
            if (!String.Equals(sourceName, pair.Key, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Unexpected locked Prism source name.");
            string sourcePath = InstallerPaths.InGame(package, pair.Key);
            byte[] bytes = File.ReadAllBytes(sourcePath);
            string actual = Hash(bytes);
            if (!String.Equals(actual, expected, StringComparison.Ordinal)) throw new InvalidDataException("Prism package hash mismatch: " + pair.Key);
            verified.Add(pair.Key, bytes);
            fileHashes.Add(pair.Key, expected);
        }
        Directory.CreateDirectory(runtime);
        foreach (KeyValuePair<string, byte[]> pair in verified)
            File.WriteAllBytes(InstallerPaths.InGame(runtime, pair.Key), pair.Value);
        var receipt = new Dictionary<string, object> {
            { "schema", 1 }, { "platform", "windows" }, { "architecture", "x86" },
            { "game_directory", InstallerPaths.Full(game) }, { "package_id", packageId },
            { "version", version }, { "source_commit", commit },
            { "owners", new[] { "FFX-native", "FFX-launcher" } }, { "files", fileHashes }
        };
        File.WriteAllText(InstallerPaths.InGame(runtime, "prism-install.json"), serializer.Serialize(receipt));
        var snapshot = new RuntimeSnapshot();
        foreach (string path in Directory.GetFiles(runtime))
        {
            string name = Path.GetFileName(path);
            snapshot.Hashes.Add(name, Hash(File.ReadAllBytes(path)));
            snapshot.Times.Add(name, System.IO.File.GetLastWriteTimeUtc(InstallerPaths.Native(path)).Ticks);
        }
        snapshots[InstallerPaths.Full(game)] = snapshot;
    }

    public static void AssertPreserved(string game)
    {
        RuntimeSnapshot before;
        if (!snapshots.TryGetValue(InstallerPaths.Full(game), out before)) throw new InvalidOperationException("No Prism runtime snapshot for fixture.");
        string runtime = InstallerPaths.InGame(game, "PrismRuntime");
        string[] paths = Directory.GetFiles(runtime);
        if (paths.Length != before.Hashes.Count) throw new Exception("Installer changed Prism runtime file count.");
        foreach (string path in paths)
        {
            string name = Path.GetFileName(path);
            string hash;
            long time;
            if (!before.Hashes.TryGetValue(name, out hash) || !before.Times.TryGetValue(name, out time) ||
                !String.Equals(Hash(File.ReadAllBytes(path)), hash, StringComparison.Ordinal) ||
                System.IO.File.GetLastWriteTimeUtc(InstallerPaths.Native(path)).Ticks != time)
                throw new Exception("Installer changed Prism runtime file: " + name);
        }
    }

    private static string StringValue(Dictionary<string, object> values, string key)
    {
        object value;
        if (!values.TryGetValue(key, out value) || !(value is string) || String.IsNullOrWhiteSpace((string)value))
            throw new InvalidDataException("Invalid Prism lock field: " + key);
        return (string)value;
    }
    private static string Hash(byte[] bytes)
    {
        using (var sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
    }
}
