using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;
using System.Linq.Expressions;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using FfxScreenReader.Setup;

internal static class OnlineInstallerTests
{
    private static object Plan(string game, Action<string> status, Assembly assembly) {
        Type stock = assembly.GetType("FfxScreenReader.Setup.StockFile", true);
        Array files = Array.CreateInstance(stock, 2);
        int index = 0;
        foreach (string name in new[] { "FFX.exe", "FFX&X-2_LAUNCHER.exe" }) {
            byte[] bytes = Encoding.UTF8.GetBytes("Synthetic placeholder, never executed");
            using (var sha = SHA1.Create()) files.SetValue(Activator.CreateInstance(stock, new object[] { name, BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", ""), (long)bytes.Length }), index++);
        }
        Type catalog = assembly.GetType("FfxScreenReader.Setup.BaseGameCatalog", true);
        object fixture = Activator.CreateInstance(catalog, new object[] { "synthetic", files });
        return assembly.GetType("FfxScreenReader.Setup.MigrationPlan", true).GetMethod("FromCatalog").Invoke(null, new object[] { game, fixture, status });
    }
    public static int Run(string executable, string archive, string version, string nextArchive, string nextVersion, string game) {
        InstallerSecurityTests.RunSetup(executable);
        Assembly assembly = Assembly.LoadFile(Path.GetFullPath(executable));
        Type program = assembly.GetType("FfxScreenReader.Setup.Program", true);
        if (assembly.GetManifestResourceNames().Length != 2)
            throw new Exception("The network installer must contain stock inventories only.");
        byte[] archiveBytes = File.ReadAllBytes(archive);
        var releaseVersion = Version.Parse(version);
        Func<string, byte[]> reader = GitHubUpdateTests.Packaged(archiveBytes, version);
        program.GetMethod("VerifyReleaseArchive", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { archiveBytes, "v" + version });
        object production = program.GetMethod("CreateEngine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { reader });
        Type engine = production.GetType();
        Directory.CreateDirectory(game);
        PrismTestSupport.Prepare(game);
        foreach (string name in new[] { "FFX.exe", "FFX&X-2_LAUNCHER.exe" }) File.WriteAllText(Path.Combine(game, name), "Synthetic placeholder, never executed");
        Action<string> quiet = delegate { };
        bool refused = false;
        try { engine.GetMethod("Install").Invoke(production, new object[] { game, quiet }); }
        catch (TargetInvocationException ex) { refused = ex.InnerException.Message.Contains("Could not identify"); }
        if (!refused || Directory.GetFiles(game).Length != 2 || Directory.GetDirectories(game).Length != 1 || !Directory.Exists(Path.Combine(game, "PrismRuntime"))) throw new Exception("Production engine must refuse non-stock placeholders without changes");
        PrismTestSupport.AssertPreserved(game);
        Type plan = assembly.GetType("FfxScreenReader.Setup.MigrationPlan", true);
        Type factoryType = typeof(Func<,,>).MakeGenericType(typeof(string), typeof(Action<string>), plan);
        var pathParameter = Expression.Parameter(typeof(string), "game");
        var statusParameter = Expression.Parameter(typeof(Action<string>), "status");
        var call = Expression.Call(typeof(OnlineInstallerTests).GetMethod("Plan", BindingFlags.Static | BindingFlags.NonPublic), pathParameter, statusParameter, Expression.Constant(assembly));
        Delegate factory = Expression.Lambda(factoryType, Expression.Convert(call, plan), pathParameter, statusParameter).Compile();
        object hashes = engine.GetField("hashes", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(production);
        var constructor = engine.GetConstructor(BindingFlags.Instance | BindingFlags.NonPublic, null,
            new[] { typeof(Func<string, byte[]>), typeof(IDictionary<string, string>), factoryType }, null);
        object fixtureEngine = constructor.Invoke(new object[] { reader, hashes, factory });
        Directory.CreateDirectory(Path.Combine(game, "mods"));
        File.WriteAllText(Path.Combine(game, "mods", "foreign.dll"), "foreign mod");
        File.WriteAllText(Path.Combine(game, "mods", "keeper.asi"), "protected plugin");
        File.WriteAllText(Path.Combine(game, "FFX_SR_ModSafeList.txt"), @"mods\keeper.asi");
        const string savedSettings = "; Keep existing choices, not new defaults\r\n[Accessibility]\r\noverdrive_assist=1\r\njecht_assist=0\r\nlightning_auto_dodge=1\r\n[Controller]\r\nrepeat=7\r\n";
        File.WriteAllText(Path.Combine(game, "FFXAccessibility.ini"), savedSettings);
        string source = "GitHub release v" + version;
        string backup = (string)engine.GetMethod("InstallVersion").Invoke(fixtureEngine, new object[] { game, quiet, releaseVersion, source });
        byte[] initialProxy = File.ReadAllBytes(Path.Combine(game, "dinput8.dll"));
        Func<string, byte[]> nextReader = GitHubUpdateTests.Packaged(File.ReadAllBytes(nextArchive), nextVersion);
        object nextEngine = program.GetMethod("CreateEngine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { nextReader });
        var nextHashes = (IDictionary<string, string>)engine.GetField("hashes", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(nextEngine);
        object fixtureUpgrade = constructor.Invoke(new object[] { nextReader, nextHashes, factory });
        string nextSource = "GitHub release v" + nextVersion;
        string upgradeBackup = (string)engine.GetMethod("InstallVersion").Invoke(fixtureUpgrade, new object[] { game, quiet, Version.Parse(nextVersion), nextSource });
        PrismTestSupport.AssertPreserved(game);
        foreach (var entry in nextHashes) {
            using (var sha = SHA256.Create())
                if (BitConverter.ToString(sha.ComputeHash(File.ReadAllBytes(Path.Combine(game, entry.Key)))).Replace("-", "").ToLowerInvariant() != entry.Value.ToLowerInvariant()) throw new Exception("Actual packaged payload mismatch: " + entry.Key);
        }
        if (File.Exists(Path.Combine(game, "mods", "foreign.dll")) || File.ReadAllText(Path.Combine(backup, "mods", "foreign.dll")) != "foreign mod" ||
            File.ReadAllText(Path.Combine(game, "mods", "keeper.asi")) != "protected plugin" ||
            File.Exists(Path.Combine(backup, "mods", "keeper.asi")) ||
            File.ReadAllText(Path.Combine(game, "FFXAccessibility.ini")) != savedSettings ||
            !File.ReadAllText(Path.Combine(backup, "INSTALL_REPORT.txt")).Contains("Install source: " + source) ||
            !File.ReadAllText(Path.Combine(upgradeBackup, "INSTALL_REPORT.txt")).Contains("Install source: " + nextSource) ||
            InstallerEngine.Hash(initialProxy) == nextHashes["dinput8.dll"] ||
            InstallerEngine.Hash(File.ReadAllBytes(Path.Combine(upgradeBackup, "dinput8.dll"))) != InstallerEngine.Hash(initialProxy))
            throw new Exception("Packaged update cleanup or preservation failed");
        Console.WriteLine("PASS: one standalone setup installs and upgrades between two verified ZIP releases. Settings, safe-listed files and PRISM are preserved.");
        return 0;
    }
}
