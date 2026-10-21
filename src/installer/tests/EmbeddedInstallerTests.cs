using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;
using System.Linq.Expressions;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

internal static class EmbeddedInstallerTests
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
    public static int Run(string executable, string game) {
        Assembly assembly = Assembly.LoadFile(Path.GetFullPath(executable));
        Type program = assembly.GetType("FfxScreenReader.Setup.Program", true);
        object production = program.GetMethod("Engine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, null);
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
        var call = Expression.Call(typeof(EmbeddedInstallerTests).GetMethod("Plan", BindingFlags.Static | BindingFlags.NonPublic), pathParameter, statusParameter, Expression.Constant(assembly));
        Delegate factory = Expression.Lambda(factoryType, Expression.Convert(call, plan), pathParameter, statusParameter).Compile();
        object reader = engine.GetField("readPayload", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(production);
        object hashes = engine.GetField("hashes", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(production);
        object fixtureEngine = engine.GetConstructor(BindingFlags.Instance | BindingFlags.NonPublic, null,
            new[] { typeof(Func<string, byte[]>), typeof(IDictionary<string, string>), factoryType }, null).Invoke(new object[] { reader, hashes, factory });
        Directory.CreateDirectory(Path.Combine(game, "mods"));
        File.WriteAllText(Path.Combine(game, "mods", "foreign.dll"), "foreign mod");
        const string savedSettings = "; Keep existing choices, not new defaults\r\n[Accessibility]\r\noverdrive_assist=1\r\njecht_assist=0\r\nlightning_auto_dodge=1\r\n[Controller]\r\nrepeat=7\r\n";
        File.WriteAllText(Path.Combine(game, "FFXAccessibility.ini"), savedSettings);
        string backup = (string)engine.GetMethod("Install").Invoke(fixtureEngine, new object[] { game, quiet });
        engine.GetMethod("Install").Invoke(fixtureEngine, new object[] { game, quiet });
        PrismTestSupport.AssertPreserved(game);
        var expected = (IDictionary<string, string>)hashes;
        foreach (var entry in expected) {
            using (var sha = SHA256.Create())
                if (BitConverter.ToString(sha.ComputeHash(File.ReadAllBytes(Path.Combine(game, entry.Key)))).Replace("-", "").ToLowerInvariant() != entry.Value.ToLowerInvariant()) throw new Exception("Actual packaged payload mismatch: " + entry.Key);
        }
        if (File.Exists(Path.Combine(game, "mods", "foreign.dll")) || File.ReadAllText(Path.Combine(backup, "mods", "foreign.dll")) != "foreign mod" ||
            File.ReadAllText(Path.Combine(game, "FFXAccessibility.ini")) != savedSettings) throw new Exception("Packaged cleanup/preservation failed");
        Console.WriteLine("PASS: actual packaged EXE validates both pinned inventories, refuses non-stock files, and installs 7 verified payloads twice with synthetic stock validation and module cleanup; Prism runtime preserved.");
        return 0;
    }
}
