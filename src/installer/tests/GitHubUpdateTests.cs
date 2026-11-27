using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
using System.Web.Script.Serialization;
using FfxScreenReader.Setup;

internal static class GitHubUpdateTests
{
    private const string Repository = "https://github.com/bladestorm360/FFX-X2-Screen-Reader/releases/download/";

    private static Dictionary<string, object> Asset(string version, int size, string digest)
    {
        string name = "ffx-x2-screen-reader-" + version + "-payload.zip";
        return new Dictionary<string, object> {
            { "name", name }, { "size", size }, { "digest", "sha256:" + digest },
            { "state", "uploaded" }, { "browser_download_url", Repository + "v" + version + "/" + name }
        };
    }

    private static string Latest(string version, bool prerelease, bool draft, params Dictionary<string, object>[] assets)
    {
        return new JavaScriptSerializer().Serialize(new { tag_name = "v" + version, prerelease, draft, assets });
    }

    public static void Run(Action<bool, string> assert, Action<Action, string> reject)
    {
        string digest = new string('a', 64);
        var asset = Asset("1.1", 1200, digest);
        string current = Latest("1.1", false, false, asset);
        GitHubRelease release = GitHubUpdates.ParseLatest(current);
        assert(release.Version == new Version(1, 1, 0, 0) && release.Digest == digest && release.Size == 1200,
            "The stable GitHub release is selected by version, fixed path, and SHA-256");
        assert(GitHubUpdates.ParseLatest(Latest("1.0.2", false, false, Asset("1.0.2", 1200, digest))).Version == new Version(1, 0, 2, 0),
            "Setup uses the published release independently of the client version");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", true, false, asset)), "published stable release");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, true, asset)), "published stable release");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1-rc.1", false, false, asset)), "unsupported release tag");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false)), "has no setup payload");
        var legacy = Asset("1.1", 1200, digest);
        legacy["name"] = "ffx-x2-screen-reader-1.1.zip";
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false, legacy)), "has no setup payload");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false, asset, asset)), "ambiguous ZIP");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false, Asset("1.1", -1, digest))), "ambiguous ZIP");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false, Asset("1.1", 90 * 1024 * 1024, digest))),
            "ambiguous ZIP");
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false, Asset("1.1", 1200, "invalid"))), "ambiguous ZIP");
        asset["browser_download_url"] = "https://github.com/someone-else/FFX-X2-Screen-Reader/releases/download/v1.1/ffx-x2-screen-reader-1.1-payload.zip";
        reject(() => GitHubUpdates.ParseLatest(Latest("1.1", false, false, asset)), "ambiguous ZIP");
        foreach (string url in new[] { "http://release-assets.githubusercontent.com/test", "https://release-assets.githubusercontent.com:8080/test",
            "https://release-assets.githubusercontent.com.attacker.invalid/test", "https://example.invalid/test", "file:///C:/test.zip" })
            reject(() => GitHubUpdates.AssetRedirect(url), "unexpected download location");
        assert(GitHubUpdates.AssetRedirect("https://release-assets.githubusercontent.com/asset?key=fixture").Host ==
            "release-assets.githubusercontent.com", "Only the GitHub release CDN receives redirected download requests");
        reject(() => GitHubUpdates.OpenPayload(release, new byte[1200]), "SHA-256 published by GitHub");
    }

    private static void Write(ZipArchive zip, string name, byte[] bytes)
    {
        using (Stream stream = zip.CreateEntry(name).Open()) stream.Write(bytes, 0, bytes.Length);
    }

    private static byte[] Change(byte[] archive, string name, string replacement, byte[] content)
    {
        using (var file = new MemoryStream())
        {
            file.Write(archive, 0, archive.Length);
            file.Position = 0;
            using (var zip = new ZipArchive(file, ZipArchiveMode.Update, true))
            {
                zip.GetEntry(name).Delete();
                Write(zip, replacement, content);
            }
            return file.ToArray();
        }
    }

    private static Func<string, byte[]> FromArchive(string version, byte[] archive)
    {
        string metadata = Latest(version, false, false, Asset(version, archive.Length, InstallerEngine.Hash(archive)));
        GitHubRelease release = GitHubUpdates.ParseLatest(metadata);
        return GitHubUpdates.OpenPayload(release, archive);
    }

    public static Func<string, byte[]> Packaged(byte[] archive, string version)
    {
        ExpectInvalid(() => FromArchive(version, Change(archive, "install/dinput8.dll", "../unexpected.exe", new byte[] { 1 })), "missing install/dinput8.dll");
        ExpectInvalid(() => FromArchive(version, Change(archive, "install/dinput8.dll", "SHA256SUMS.txt", new byte[] { 1 })), "duplicate entry");
        ExpectInvalid(() => FromArchive(version, Change(archive, "install/dinput8.dll", "install/dinput8.dll", new byte[] { 1 })), "invalid x86 DLL");
        ExpectInvalid(() => FromArchive("99.0.0", archive), "does not match the GitHub tag");
        Func<string, byte[]> first = FromArchive(version, archive);
        Func<string, byte[]> retry = FromArchive(version, archive);
        if (InstallerEngine.Hash(first("SHA256SUMS.txt")) != InstallerEngine.Hash(retry("SHA256SUMS.txt")))
            throw new Exception("Read-only retry changed the verified payload.");
        ExpectInvalid(() => retry("unexpected.dll"), "missing a payload file");
        return retry;
    }

    private static void ExpectInvalid(Action action, string message)
    {
        try { action(); }
        catch (InvalidDataException ex)
        {
            if (ex.Message.IndexOf(message, StringComparison.OrdinalIgnoreCase) >= 0) return;
            throw;
        }
        throw new Exception("Expected update refusal: " + message);
    }

    public static int Live(string fixture)
    {
        GitHubRelease release;
        try { release = GitHubUpdates.Check(); }
        catch (InvalidDataException ex)
        {
            if (!ex.Message.Contains("has no setup payload")) throw;
            Console.WriteLine("PASS: read-only GitHub API check refused the published legacy release. " + ex.Message);
            return 0;
        }
        Func<string, byte[]> payload = GitHubUpdates.FetchPayload(release);
        string sums = Encoding.UTF8.GetString(payload("SHA256SUMS.txt"));
        byte[] proxy = payload("dinput8.dll");
        if (sums.IndexOf(InstallerEngine.Hash(proxy) + "  install/dinput8.dll", StringComparison.Ordinal) < 0)
            throw new Exception("Live release proxy checksum differs.");
        System.IO.Directory.CreateDirectory(fixture);
        System.IO.File.WriteAllBytes(Path.Combine(fixture, "dinput8.dll"), proxy);
        var guard = typeof(InstallerEngine).GetMethod("RequireNoDowngrade", BindingFlags.Static | BindingFlags.NonPublic);
        guard.Invoke(null, new object[] { fixture, release.Version });
        try
        {
            guard.Invoke(null, new object[] { fixture, new Version(0, 0, 0, 0) });
            throw new Exception("Downgrade guard accepted a real newer screen reader proxy.");
        }
        catch (TargetInvocationException ex)
        {
            if (!(ex.InnerException is InvalidOperationException) || !ex.InnerException.Message.Contains("already has screen reader version")) throw;
        }
        Console.WriteLine("PASS: read-only GitHub API and CDN download for " + release.Tag + ", exact ZIP and payload verification, expected-version and downgrade guard on a fixture.");
        return 0;
    }
}
