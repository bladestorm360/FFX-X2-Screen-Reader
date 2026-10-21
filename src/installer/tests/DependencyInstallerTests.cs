using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using FfxScreenReader.Setup;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

// Installs actual embedded resources only into synthetic workspace folders.
// Neither the PRISM DLL nor game placeholders are loaded or executed.
internal static class DependencyInstallerTests
{
    private static string root;
    private static Assembly package;
    private static int checks;
    private static void Assert(bool condition, string message) {
        if (!condition) throw new Exception(message);
        checks++;
    }
    private static byte[] Resource(string name) {
        using (Stream stream = package.GetManifestResourceStream("Payload." + name)) {
            if (stream == null) throw new Exception("Missing resource: " + name);
            using (var bytes = new MemoryStream()) { stream.CopyTo(bytes); return bytes.ToArray(); }
        }
    }
    private static StockFile Stock(string name) {
        byte[] bytes = Encoding.UTF8.GetBytes("Synthetic stock; never executed");
        using (var sha = SHA1.Create())
            return new StockFile(name, BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", ""), bytes.Length);
    }
    private static string Game(string name) {
        string game = InstallerPaths.InGame(root, name);
        Directory.CreateDirectory(game);
        foreach (string file in new[] { "FFX.exe", "FFX&X-2_LAUNCHER.exe" })
            File.WriteAllText(InstallerPaths.InGame(game, file), "Synthetic stock; never executed");
        return game;
    }
    private static InstallerEngine Engine(string expectedGame, Func<string, byte[]> reader) {
        var hashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (Match line in Regex.Matches(Encoding.UTF8.GetString(Resource("SHA256SUMS.txt")),
            "^([a-fA-F0-9]{64})\\s+install/([^\\r\\n]+)", RegexOptions.Multiline))
            hashes.Add(line.Groups[2].Value, line.Groups[1].Value);
        return new InstallerEngine(reader, hashes, delegate(string game, Action<string> status) {
            Assert(InstallerPaths.Full(game).Equals(expectedGame, StringComparison.OrdinalIgnoreCase), "Wrong fixture target");
            return MigrationPlan.FromCatalog(game, new BaseGameCatalog("synthetic", new[] {
                Stock("FFX.exe"), Stock("FFX&X-2_LAUNCHER.exe") }), status);
        }, delegate {
            Assert(expectedGame.StartsWith(root + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase), "Fixture escapes workspace");
        });
    }
    private static Dictionary<string, string> Snapshot(string directory, bool times) {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (string path in Directory.GetFiles(directory, "*", SearchOption.AllDirectories))
            result.Add(path.Substring(directory.Length), InstallerEngine.Hash(File.ReadAllBytes(path)) +
                (times ? ":" + System.IO.File.GetLastWriteTimeUtc(InstallerPaths.Native(path)).Ticks : ""));
        return result;
    }
    private static void Same(string game, Dictionary<string, string> before, bool times) {
        var after = Snapshot(game, times);
        Assert(before.Count == after.Count, "Unexpected file count change");
        foreach (var file in before) Assert(after.ContainsKey(file.Key) && after[file.Key] == file.Value, "Changed file: " + file.Key);
    }
    private static void Refused(Action action, string message) {
        bool refused = false;
        try { action(); } catch (Exception error) {
            Assert(error.Message.IndexOf(message, StringComparison.OrdinalIgnoreCase) >= 0, error.Message);
            refused = true;
        }
        Assert(refused, "Expected refusal: " + message);
    }
    private static void Installed(string game) {
        PrismPrerequisite.Require(game, Resource("prism.lock.json"));
        Assert(Directory.GetFiles(InstallerPaths.InGame(game, "PrismRuntime")).Length == 16, "PRISM inventory plus receipt");
        foreach (string name in InstallerEngine.PayloadNames)
            Assert(InstallerEngine.Hash(File.ReadAllBytes(InstallerPaths.InGame(game, name))) == InstallerEngine.Hash(Resource(name)), "Installed payload: " + name);
    }
    private static int Main(string[] args) {
        try {
            root = InstallerPaths.Full(args[0]);
            Assert(root.StartsWith(InstallerPaths.Full(args[2]) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase), "Test root outside explicit workspace");
            Assert(!Directory.Exists(root) && !File.Exists(root), "Use a fresh owned test root");
            Directory.CreateDirectory(root);
            package = Assembly.LoadFile(Path.GetFullPath(args[1]));
            Assert(package.GetManifestResourceNames().Length == 26, "Single EXE has 26 resources");
            package.GetType("FfxScreenReader.Setup.Program", true).GetMethod("Engine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, null);
            string game = Game("fresh");
            File.WriteAllText(InstallerPaths.InGame(game, "Tolk.dll"), "old Tolk");
            Directory.CreateDirectory(InstallerPaths.InGame(game, "mods"));
            File.WriteAllText(InstallerPaths.InGame(game, "mods/Tolk.dll"), "nested old Tolk");
            File.WriteAllText(InstallerPaths.InGame(game, "FFXAccessibility.ini"), "settings preserved");
            string backup = Engine(game, Resource).Install(game, delegate { });
            Installed(game);
            Assert(!File.Exists(InstallerPaths.InGame(game, "Tolk.dll")) && !File.Exists(InstallerPaths.InGame(game, "mods/Tolk.dll")), "Tolk removed");
            Assert(File.ReadAllText(InstallerPaths.InGame(backup, "Tolk.dll")) == "old Tolk", "Root Tolk backed up");
            Assert(File.ReadAllText(InstallerPaths.InGame(backup, "mods/Tolk.dll")) == "nested old Tolk", "Nested Tolk backed up");
            Assert(File.ReadAllText(InstallerPaths.InGame(game, "FFXAccessibility.ini")) == "settings preserved", "Settings preserved");
            string runtime = InstallerPaths.InGame(game, "PrismRuntime");
            var sdk = Snapshot(runtime, true);
            Engine(game, Resource).Install(game, delegate { });
            Same(runtime, sdk, true);
            File.WriteAllText(InstallerPaths.InGame(game, "PrismRuntime/prism-install.json"), "invalid receipt");
            var before = Snapshot(game, true);
            Refused(delegate { Engine(game, Resource).Install(game, delegate { }); }, "Prism prerequisite");
            Same(game, before, true);
            string corrupt = Game("corrupt-package");
            before = Snapshot(corrupt, true);
            Refused(delegate { Engine(corrupt, delegate(string name) {
                return name == "PrismRuntime.prism.dll" ? new byte[1] : Resource(name);
            }).Install(corrupt, delegate { }); }, "Embedded PRISM checksum failed");
            Same(corrupt, before, true);
            Assert(Directory.GetDirectories(corrupt).Length == 0, "Corrupt embedded SDK creates no folders");
            string badStock = Game("bad-stock");
            File.WriteAllText(InstallerPaths.InGame(badStock, "FFX.exe"), "modified stock");
            before = Snapshot(badStock, true);
            Refused(delegate { Engine(badStock, Resource).Install(badStock, delegate { }); }, "missing or modified");
            Same(badStock, before, true);
            Assert(Directory.GetDirectories(badStock).Length == 0, "Bad stock creates no SDK or backup folders");
            string foreign = Game("unreceipted");
            Directory.CreateDirectory(InstallerPaths.InGame(foreign, "PrismRuntime"));
            File.WriteAllText(InstallerPaths.InGame(foreign, "PrismRuntime/foreign.dll"), "preserved");
            before = Snapshot(foreign, true);
            Refused(delegate { Engine(foreign, Resource).Install(foreign, delegate { }); }, "Prism prerequisite");
            Same(foreign, before, true);
            string longGame = Game("long/" + new string('a', 90) + "/" + new string('b', 90) + "/" + new string('c', 90));
            Engine(longGame, Resource).Install(InstallerPaths.Native(longGame), delegate { });
            Installed(longGame);
            // Production constructors still use the real closed-process guard.
            // If a real session is open, refuse it; otherwise stock identity refuses.
            string productionGame = Game("production-refusal");
            before = Snapshot(productionGame, true);
            object production = package.GetType("FfxScreenReader.Setup.Program", true).GetMethod("Engine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, null);
            bool denied = false;
            try { production.GetType().GetMethod("Install").Invoke(production, new object[] { productionGame, (Action<string>)delegate { } }); }
            catch (TargetInvocationException error) {
                string message = error.InnerException.Message;
                denied = message.Contains("Close the game and launcher") || message.Contains("Could not identify");
            }
            Assert(denied, "Production process/stock guard must refuse synthetic game");
            Same(productionGame, before, true);
            Assert(Directory.GetDirectories(productionGame).Length == 0, "Production refusal creates no dependency folders");
            Console.WriteLine("PASS: " + checks + " unified-installer assertions; real embedded PRISM/licenses and SDL3, Tolk backups/removal, repeat zero SDK writes, corrupt/foreign/stock/process refusals and long paths. No native DLLs executed.");
            return 0;
        } catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}
