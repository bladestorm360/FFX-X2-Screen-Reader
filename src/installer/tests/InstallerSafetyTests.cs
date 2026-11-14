using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Web.Script.Serialization;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;
using FfxScreenReader.Setup;

// Install safety cases that run without the real PRISM package: a synthetic
// 15-file PRISM lock lets setup stage, install and roll back PrismRuntime itself.
internal static class InstallerSafetyTests
{
    private static Action<bool, string> assert;
    private static string root;
    private static readonly Dictionary<string, byte[]> resources = new Dictionary<string, byte[]>(StringComparer.Ordinal);
    private static readonly Dictionary<string, string> hashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

    public static void Run(string fixture, Action<bool, string> check)
    {
        assert = check;
        root = Path.Combine(fixture, "Safety");
        Directory.CreateDirectory(root);
        foreach (string name in InstallerEngine.PayloadNames) {
            resources["Payload " + name] = Encoding.UTF8.GetBytes("Safety payload " + name);
            hashes[name] = InstallerEngine.Hash(resources["Payload " + name]);
        }
        var files = new Dictionary<string, object>();
        for (int i = 0; i < 15; i++) {
            string name = i == 0 ? "prism.dll" : "prism-file-" + i + ".txt";
            byte[] bytes = i == 0 ? Pe() : Encoding.UTF8.GetBytes("Synthetic PRISM file " + i);
            resources["PrismRuntime." + name] = bytes;
            files[name] = new Dictionary<string, object> { { "source", name }, { "sha256", InstallerEngine.Hash(bytes) } };
        }
        resources["prism.lock.json"] = Encoding.UTF8.GetBytes(new JavaScriptSerializer().Serialize(new Dictionary<string, object> {
            { "schema", 1 }, { "package_id", "synthetic-prism" }, { "version", "0.0.0" }, { "source_commit", "synthetic" },
            { "platform", "windows" }, { "architecture", "x86" }, { "files", files } }));
        OtherModsStay();
        PayloadFailureRollsBack();
        UninstallRestores();
        UpgradeFromOlderSetup();
        UninstallRefuses();
        UninstallFailureKeepsLauncher();
    }

    private static byte[] Pe()
    {
        var bytes = new byte[128];
        bytes[0] = 0x4d; bytes[1] = 0x5a;
        BitConverter.GetBytes(64).CopyTo(bytes, 60);
        BitConverter.GetBytes(0x4550u).CopyTo(bytes, 64);
        BitConverter.GetBytes((ushort)0x14c).CopyTo(bytes, 68);
        return bytes;
    }

    private static StockFile Stock(string name, string content)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(content);
        using (var sha = SHA1.Create()) return new StockFile(name, BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", ""), bytes.Length);
    }

    private static InstallerEngine Engine()
    {
        var catalog = new BaseGameCatalog("synthetic", new[] {
            Stock("FFX.exe", "Synthetic fixture; not a game executable."),
            Stock("FFX&X-2_LAUNCHER.exe", "Synthetic fixture; not a launcher executable.")
        });
        return new InstallerEngine(delegate(string name) {
            byte[] bytes;
            return resources.TryGetValue(name, out bytes) || resources.TryGetValue("Payload " + name, out bytes) ? bytes : null;
        }, hashes, delegate(string game, Action<string> status) { return MigrationPlan.FromCatalog(game, catalog, status); },
            delegate { }); // No process check: these cases must pass while the game is open.
    }

    private static string Game(string name)
    {
        string game = Path.Combine(root, name);
        Directory.CreateDirectory(game);
        File.WriteAllText(Path.Combine(game, "FFX.exe"), "Synthetic fixture; not a game executable.");
        File.WriteAllText(Path.Combine(game, "FFX&X-2_LAUNCHER.exe"), "Synthetic fixture; not a launcher executable.");
        File.WriteAllText(Path.Combine(game, "FFX&X-2_LAUNCHER.exe.config"), "<configuration><appSettings><add key=\"Kept\" value=\"yes\"/></appSettings></configuration>");
        File.WriteAllText(Path.Combine(game, "dinput8.dll"), "older reader dinput8");
        File.WriteAllText(Path.Combine(game, "FFXAccessibility.ini"), "[Accessibility]\r\njecht_assist=0\r\n");
        return game;
    }

    // Every file and folder outside FFX_SR_Install_Backups, with file hashes.
    private static Dictionary<string, string> Snapshot(string game)
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (string path in Directory.GetFileSystemEntries(game)) {
            string name = Path.GetFileName(path);
            if (name.Equals("FFX_SR_Install_Backups", StringComparison.OrdinalIgnoreCase)) continue;
            if (Directory.Exists(path)) {
                result.Add(name + "\\", "folder");
                foreach (string file in Directory.GetFiles(path, "*", SearchOption.AllDirectories))
                    result.Add(file.Substring(game.Length + 1), InstallerEngine.Hash(File.ReadAllBytes(file)));
                foreach (string folder in System.IO.Directory.GetDirectories(path, "*", SearchOption.AllDirectories))
                    result.Add(folder.Substring(game.Length + 1) + "\\", "folder");
            }
            else result.Add(name, InstallerEngine.Hash(File.ReadAllBytes(path)));
        }
        return result;
    }

    private static void Unchanged(string game, Dictionary<string, string> before, string label)
    {
        var after = Snapshot(game);
        foreach (var entry in before) {
            string hash;
            assert(after.TryGetValue(entry.Key, out hash) && hash == entry.Value, label + ": changed or missing " + entry.Key);
        }
        foreach (string name in after.Keys) assert(before.ContainsKey(name), label + ": unexpected " + name);
    }

    private static void Reject(Action action, string message, string label)
    {
        string error = null;
        try { action(); }
        catch (Exception ex) { error = ex.Message; }
        assert(error != null && error.IndexOf(message, StringComparison.OrdinalIgnoreCase) >= 0,
            label + ": expected refusal containing '" + message + "', got: " + (error ?? "success"));
    }

    private static void OtherModsStay()
    {
        string game = Game("Other mods stay");
        Directory.CreateDirectory(Path.Combine(game, "mods", "loader"));
        var others = new[] { "dxgi.dll", "foo.asi", "mods\\loader\\plugin.asi" };
        foreach (string name in others) File.WriteAllText(Path.Combine(game, name), "other mod " + name);
        foreach (string name in new[] { "Tolk.dll", "frida-gadget.dll", "ffx_sr.js" }) File.WriteAllText(Path.Combine(game, name), "legacy " + name);
        var messages = new List<string>();
        InstallerEngine engine = Engine();
        string backup = engine.Install(game, delegate(string text) {
            messages.Add(text);
            if (text == "Configuring launcher speech") File.WriteAllText(Path.Combine(game, "late.dll"), "unrelated module added during setup");
        });
        foreach (string name in others)
            assert(File.ReadAllText(Path.Combine(game, name)) == "other mod " + name && !File.Exists(Path.Combine(backup, name)), "Other mod left untouched: " + name);
        assert(File.Exists(Path.Combine(game, "late.dll")), "Unrelated module added during setup is not an error");
        foreach (string name in new[] { "Tolk.dll", "frida-gadget.dll", "ffx_sr.js" })
            assert(!File.Exists(Path.Combine(game, name)) && File.ReadAllText(Path.Combine(backup, name)) == "legacy " + name, "Legacy file backed up and removed: " + name);
        string report = File.ReadAllText(Path.Combine(backup, "INSTALL_REPORT.txt"));
        foreach (string name in others) assert(report.Contains(name), "Report lists other mod: " + name);
        assert(report.Contains("Left in place"), "Report names left-in-place mods");
        assert(engine.LeftInPlace.Count == 3 && engine.LeftInPlace.Contains("dxgi.dll") && engine.LeftInPlace.Contains("foo.asi") &&
            engine.LeftInPlace.Contains("mods\\loader\\plugin.asi"), "Final status lists left-in-place mods");
        assert(messages.Exists(delegate(string text) { return text.StartsWith("Left in place: ") && text.Contains("dxgi.dll"); }), "Status reports left-in-place mods");
        var before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text == "Configuring launcher speech") File.WriteAllText(Path.Combine(game, "frida-gadget.dll"), "legacy file restored by another tool");
        }); }, "appeared during setup", "Legacy file reappearing during setup");
        Unchanged(game, before, "Legacy reappearance rollback");
    }

    private static void PayloadFailureRollsBack()
    {
        string game = Game("Payload failure");
        File.WriteAllText(Path.Combine(game, "Tolk.dll"), "legacy Tolk");
        var before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text == "Installing SDL3.dll") throw new IOException("Synthetic write failure");
        }); }, "Previous files were restored", "Payload failure");
        Unchanged(game, before, "Payload failure rollback");

        // Fail after PRISM, old-file removal and every payload move: rollback must undo all of it.
        foreach (string name in new[] { "SDL3.dll", "MinHook.x86.dll", "frida-gadget.dll", "ffx_sr.js" })
            File.WriteAllText(Path.Combine(game, name), "older file " + name);
        before = Snapshot(game);
        bool prismWritten = false, payloadWritten = false;
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text != "Configuring launcher speech") return;
            prismWritten = Directory.Exists(Path.Combine(game, "PrismRuntime"));
            payloadWritten = File.ReadAllText(Path.Combine(game, "SDL3.dll")) == "Safety payload SDL3.dll" &&
                !File.Exists(Path.Combine(game, "frida-gadget.dll"));
            throw new IOException("Synthetic failure after payload");
        }); }, "Previous files were restored", "Late failure");
        assert(prismWritten && payloadWritten, "Late failure happens after PRISM and payload were written");
        Unchanged(game, before, "Late failure rollback restores every file and removes PRISM");

        // A staged file that fails its checksum stops setup before any live file is replaced.
        byte[] sdl = resources["Payload SDL3.dll"];
        before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text.StartsWith("Removing old mod file: ", StringComparison.Ordinal)) sdl[0] = (byte)'X';
        }); }, "Staged file checksum failed: SDL3.dll", "Staged write failure");
        sdl[0] = (byte)'S';
        Unchanged(game, before, "Staged write failure leaves live files and no stage folder");

        // A payload file that was absent when setup planned must not be replaced.
        string appeared = Game("Payload appears during setup");
        before = Snapshot(appeared);
        Reject(delegate { Engine().Install(appeared, delegate(string text) {
            if (text == "Installing SDL3.dll") File.WriteAllText(Path.Combine(appeared, "SDL3.dll"), "another program's SDL3");
        }); }, "SDL3.dll appeared during setup", "Payload appears during setup");
        Unchanged(appeared, before, "Payload that appeared during setup is rolled back");
    }


    private static void UninstallRestores()
    {
        string game = Game("Uninstall");
        File.WriteAllText(Path.Combine(game, "Tolk.dll"), "legacy Tolk");
        File.WriteAllText(Path.Combine(game, "dxgi.dll"), "ReShade");
        var before = Snapshot(game);
        before.Remove("Tolk.dll"); // An old screen reader file; uninstall leaves it removed.
        var engine = Engine();
        engine.Install(game, delegate { });
        assert(engine.ReplacedLoader, "Another mod's dinput8.dll is reported as replaced");
        assert(File.ReadAllText(Path.Combine(Directory.GetDirectories(Path.Combine(game, "FFX_SR_Install_Backups"))[0], "INSTALL_REPORT.txt"))
            .Contains(InstallerEngine.ReplacedLoaderText), "Install report names the replaced dinput8.dll");
        string message = Engine().Uninstall(game, delegate { });
        Unchanged(game, before, "Install then uninstall");
        assert(message.StartsWith("Uninstall complete", StringComparison.Ordinal), "Uninstall reports success");
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "Nothing was changed", "Second uninstall");
        Unchanged(game, before, "Second uninstall changes nothing");

        Engine().Install(game, delegate { });
        Engine().Install(game, delegate { });
        Engine().Uninstall(game, delegate { });
        Unchanged(game, before, "Two installs then one uninstall restores the other mod's dinput8.dll");
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "not installed by this setup", "Uninstall after two installs");

        // PRISM stays from an install whose backups and files were removed by hand.
        string prismKept = Game("Uninstall keeps existing PRISM");
        string keptConfig = Path.Combine(prismKept, "FFX&X-2_LAUNCHER.exe.config");
        string stockConfig = File.ReadAllText(keptConfig);
        Engine().Install(prismKept, delegate { });
        System.IO.Directory.Move(Path.Combine(prismKept, "FFX_SR_Install_Backups"), Path.Combine(root, "Spent backups"));
        foreach (string name in InstallerEngine.PayloadNames) File.Delete(Path.Combine(prismKept, name));
        File.Delete(keptConfig + ".ffx_sr_original");
        File.WriteAllText(keptConfig, stockConfig);
        before = Snapshot(prismKept);
        assert(before.ContainsKey("PrismRuntime\\prism.dll"), "Fixture has PRISM that this setup did not install");
        Engine().Install(prismKept, delegate { });
        Engine().Uninstall(prismKept, delegate { });
        Unchanged(prismKept, before, "PRISM that setup did not install stays");
    }

    // A 1.x setup left its payload, Tolk, old Frida files and its saved launcher
    // configuration, with a backup folder that has no uninstall record.
    private static void UpgradeFromOlderSetup()
    {
        string game = Game("Upgrade from 1.x");
        string config = Path.Combine(game, "FFX&X-2_LAUNCHER.exe.config");
        string stock = File.ReadAllText(config);
        var stockSnapshot = Snapshot(game);
        stockSnapshot.Remove("dinput8.dll"); // The fixture's dinput8.dll stands in for 1.x below.
        File.WriteAllText(Path.Combine(game, "dinput8.dll"), "1.x dinput8");
        foreach (string name in new[] { "MinHook.x86.dll", "SDL3.dll", "FFXLauncherReader.dll", "FFXLauncherNative.dll",
            "FFX_SR_SDL3_LICENSE.txt", "FFX_SR_Native_Dependencies.md", "Tolk.dll", "frida-gadget.dll", "ffx_sr.js" })
            File.WriteAllText(Path.Combine(game, name), "1.x " + name);
        File.WriteAllText(config + ".ffx_sr_original", stock);
        File.WriteAllText(config, "<configuration><runtime><appDomainManagerAssembly value=\"FFXLauncherReader, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null\"/>" +
            "<appDomainManagerType value=\"FfxLauncher.ReaderDomainManager\"/></runtime><appSettings><add key=\"Kept\" value=\"yes\"/></appSettings></configuration>");
        string old = Path.Combine(game, "FFX_SR_Install_Backups", "20261021-120000-0123456789abcdef0123456789abcdef");
        Directory.CreateDirectory(old);
        File.WriteAllText(Path.Combine(old, "dinput8.dll"), "pre-1.x dinput8");

        var engine = Engine();
        engine.Install(game, delegate { });
        assert(!engine.ReplacedLoader, "The 1.x dinput8.dll is not reported as another mod's loader");
        Engine().Install(game, delegate { });
        Engine().Uninstall(game, delegate { });
        Unchanged(game, stockSnapshot, "Uninstall after a 1.x upgrade leaves the stock game");
        assert(File.ReadAllText(config) == stock, "Uninstall restores the launcher configuration saved by 1.x");
        assert(File.ReadAllText(Path.Combine(old, "dinput8.dll")) == "pre-1.x dinput8", "1.x backup stays untouched");

        var before = Snapshot(game);
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "not installed by this setup", "Uninstall after the last one");
        Unchanged(game, before, "Second uninstall after a 1.x upgrade");

        File.WriteAllText(Path.Combine(game, "dinput8.dll"), "1.x dinput8");
        File.WriteAllText(Path.Combine(game, "FFXLauncherReader.dll"), "1.x reader");
        Engine().Install(game, delegate { });
        File.Delete(config + ".ffx_sr_original");
        File.WriteAllText(config, "<configuration><runtime><appDomainManagerType value=\"FfxLauncher.ReaderDomainManager\"/></runtime></configuration>");
        before = Snapshot(game);
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "Nothing was changed", "Uninstall after the launcher configuration changed");
        Unchanged(game, before, "Uninstall after the launcher configuration changed");
    }

    private static void UninstallRefuses()
    {
        string game = Game("Uninstall refusal");
        var before = Snapshot(game);
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "Nothing was changed", "Uninstall without backups");
        Unchanged(game, before, "Uninstall without backups");

        Engine().Install(game, delegate { });
        string backups = Path.Combine(game, "FFX_SR_Install_Backups");
        string newest = Directory.GetDirectories(backups)[0];
        string report = Path.Combine(newest, "INSTALL_REPORT.txt");
        byte[] saved = File.ReadAllBytes(report);
        File.Delete(report);
        before = Snapshot(game);
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "Install with this version first, then choose Uninstall again", "Uninstall with missing report");
        Unchanged(game, before, "Uninstall with missing report");
        File.WriteAllBytes(report, saved);

        File.WriteAllText(Path.Combine(game, "SDL3.dll"), "replaced after setup");
        before = Snapshot(game);
        Reject(delegate { Engine().Uninstall(game, delegate { }); }, "Nothing was changed", "Uninstall after a later change");
        Unchanged(game, before, "Uninstall after a later change");

        // Setup 1.0.2 wrote INSTALL_REPORT.txt without an uninstall record.
        string older = Game("Uninstall after 1.0.2");
        string olderBackup = Path.Combine(older, "FFX_SR_Install_Backups", "20261025-120000-0123456789abcdef0123456789abcdef");
        Directory.CreateDirectory(olderBackup);
        File.WriteAllLines(Path.Combine(olderBackup, "INSTALL_REPORT.txt"), new[] {
            "Steam depot: 359871", "Verified manifest: 1234567890", "Added DLL/ASI files removed (backups preserve their relative paths):" });
        before = Snapshot(older);
        Reject(delegate { Engine().Uninstall(older, delegate { }); }, "Install with this version first, then choose Uninstall again", "Uninstall after 1.0.2");
        Unchanged(older, before, "Uninstall after 1.0.2");
    }

    // A failure part way through uninstall must not leave the launcher
    // configuration naming a reader DLL that is already gone.
    private static void UninstallFailureKeepsLauncher()
    {
        string game = Game("Uninstall failure");
        string config = Path.Combine(game, "FFX&X-2_LAUNCHER.exe.config");
        Engine().Install(game, delegate { });
        Reject(delegate { Engine().Uninstall(game, delegate(string text) {
            if (text == "Removing FFXLauncherNative.dll") throw new IOException("Synthetic uninstall failure");
        }); }, "Uninstall did not finish", "Uninstall failure");
        assert(File.Exists(Path.Combine(game, "FFXLauncherReader.dll")) ||
            File.ReadAllText(config).IndexOf("FfxLauncher.ReaderDomainManager", StringComparison.Ordinal) < 0,
            "Launcher configuration does not name a deleted reader DLL");
    }
}
