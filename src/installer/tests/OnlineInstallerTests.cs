using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using FfxScreenReader.Setup;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

internal static class OnlineInstallerTests
{
    private const string StockContents = "Synthetic placeholder, never executed";

    private static BaseGameCatalog FixtureCatalog()
    {
        byte[] bytes = Encoding.UTF8.GetBytes(StockContents);
        string hash;
        using (var sha = SHA1.Create()) hash = BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "");
        return new BaseGameCatalog("synthetic", new[] {
            new StockFile("FFX.exe", hash, bytes.Length),
            new StockFile("FFX&X-2_LAUNCHER.exe", hash, bytes.Length)
        });
    }

    private static InstallerEngine FixtureEngine(Type setup, Func<string, byte[]> payload)
    {
        object production = setup.GetMethod("CreateEngine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { payload });
        var hashes = (IDictionary<string, string>)production.GetType().GetField("hashes", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(production);
        return new InstallerEngine(payload, hashes, (game, report) => MigrationPlan.FromCatalog(game, FixtureCatalog(), report));
    }

    private static void VerifyInstalledFiles(string game, Func<string, byte[]> payload)
    {
        foreach (string file in InstallerEngine.PayloadNames)
            if (InstallerEngine.Hash(File.ReadAllBytes(Path.Combine(game, file))) != InstallerEngine.Hash(payload(file)))
                throw new Exception("Downloaded release file differs after setup: " + file);
    }

    private static void AssertNoDownload(string game, GitHubRelease release)
    {
        string backups = Path.Combine(game, "FFX_SR_Install_Backups");
        int backupCount = Directory.GetDirectories(backups).Length;
        string receipt = Path.Combine(game, InstalledRelease.FileName);
        long receiptWrite = System.IO.File.GetLastWriteTimeUtc(InstallerPaths.Native(receipt)).Ticks;
        int checks = 0;
        SetupResult result = Program.InstallSelected(game, delegate { }, () => { checks++; return release; },
            delegate { throw new Exception("Already-current setup requested a download."); });
        if (result.Version != release.Version || result.Backup != null || checks != 1 ||
            Directory.GetDirectories(backups).Length != backupCount ||
            System.IO.File.GetLastWriteTimeUtc(InstallerPaths.Native(receipt)).Ticks != receiptWrite)
            throw new Exception("Already-current setup was not a metadata-only, read-only check.");
    }

    public static int Run(string executable, string archive, string version, string nextArchive, string nextVersion, string game)
    {
        InstallerSecurityTests.RunSetup(executable);
        Assembly assembly = Assembly.LoadFile(Path.GetFullPath(executable));
        Type setup = assembly.GetType("FfxScreenReader.Setup.Program", true);
        if (assembly.GetManifestResourceNames().Length != 2)
            throw new Exception("The network installer must contain stock inventories only.");

        byte[] archiveBytes = File.ReadAllBytes(archive);
        Func<string, byte[]> first = GitHubUpdateTests.Packaged(archiveBytes, version);
        setup.GetMethod("VerifyReleaseArchive", BindingFlags.Static | BindingFlags.NonPublic)
            .Invoke(null, new object[] { archiveBytes, "v" + version });
        object production = setup.GetMethod("CreateEngine", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { first });
        Directory.CreateDirectory(game);
        foreach (string name in new[] { "FFX.exe", "FFX&X-2_LAUNCHER.exe" })
            File.WriteAllText(Path.Combine(game, name), StockContents);
        Action<string> quiet = delegate { };
        bool refused = false;
        try { production.GetType().GetMethod("Install").Invoke(production, new object[] { game, quiet }); }
        catch (TargetInvocationException ex) { refused = ex.InnerException.Message.Contains("Could not identify"); }
        if (!refused || Directory.GetFiles(game).Length != 2 || Directory.GetDirectories(game).Length != 0)
            throw new Exception("Production setup must refuse non-stock placeholders before changing the game folder.");

        GitHubRelease firstRelease = GitHubUpdateTests.Release(version, archiveBytes);
        int downloads = 0;
        SetupResult fresh = Program.InstallSelected(game, quiet, () => firstRelease, release => {
            downloads++;
            return FixtureEngine(setup, first);
        });
        if (fresh.Backup == null || downloads != 1 || fresh.Version != firstRelease.Version)
            throw new Exception("A clean game must download and install the current release once.");
        VerifyInstalledFiles(game, first);
        PrismPrerequisite.Require(game, first("prism.lock.json"));
        PrismTestSupport.CaptureExisting(game);

        Directory.CreateDirectory(Path.Combine(game, "mods"));
        string unlisted = Path.Combine(game, "mods", "foreign.dll");
        string protectedFile = Path.Combine(game, "mods", "keeper.asi");
        File.WriteAllText(unlisted, "foreign mod");
        File.WriteAllText(protectedFile, "protected plugin");
        const string safeListEntry = @"mods\keeper.asi";
        string safeList = Path.Combine(game, ModSafeList.FileName);
        File.WriteAllText(safeList, safeListEntry);
        const string savedSettings = "; Keep existing choices\r\n[Accessibility]\r\noverdrive_assist=1\r\n";
        string settings = Path.Combine(game, "FFXAccessibility.ini");
        File.WriteAllText(settings, savedSettings);

        AssertNoDownload(game, firstRelease);
        if (!File.Exists(unlisted)) throw new Exception("The update check ran cleanup on an already-current installation.");

        byte[] upgradeBytes = File.ReadAllBytes(nextArchive);
        Func<string, byte[]> next = GitHubUpdateTests.Packaged(upgradeBytes, nextVersion);
        GitHubRelease nextRelease = GitHubUpdateTests.Release(nextVersion, upgradeBytes);
        try
        {
            Program.InstallSelected(game, phase => {
                if (phase == "Configuring launcher speech") throw new IOException("Synthetic upgrade failure");
            }, () => nextRelease, release => FixtureEngine(setup, next));
            throw new Exception("The interrupted update was expected to fail.");
        }
        catch (IOException ex)
        {
            if (!ex.Message.Contains("Previous files were restored.")) throw;
        }
        AssertNoDownload(game, firstRelease);
        VerifyInstalledFiles(game, first);

        SetupResult newer = Program.InstallSelected(game, quiet, () => nextRelease, release => {
            downloads++;
            return FixtureEngine(setup, next);
        });
        if (newer.Backup == null || downloads != 2 || newer.Version != nextRelease.Version)
            throw new Exception("A newer release must download and update the existing installation once.");
        VerifyInstalledFiles(game, next);
        PrismTestSupport.AssertPreserved(game);
        if (File.Exists(unlisted) || File.ReadAllText(Path.Combine(newer.Backup, "mods", "foreign.dll")) != "foreign mod" ||
            File.ReadAllText(protectedFile) != "protected plugin" || File.Exists(Path.Combine(newer.Backup, "mods", "keeper.asi")) ||
            File.ReadAllText(safeList) != safeListEntry || File.ReadAllText(settings) != savedSettings ||
            File.ReadAllText(Path.Combine(game, InstallerEngine.ReadmeName)) != "README for the newer release." ||
            InstallerEngine.Hash(File.ReadAllBytes(Path.Combine(newer.Backup, "dinput8.dll"))) != InstallerEngine.Hash(first("dinput8.dll")) ||
            !File.ReadAllText(Path.Combine(newer.Backup, "INSTALL_REPORT.txt")).Contains("Install source: GitHub release v" + nextVersion))
            throw new Exception("Hosted update changed the safe list, settings, or expected release files.");

        AssertNoDownload(game, nextRelease);
        try
        {
            Program.InstallSelected(game, quiet, () => firstRelease, delegate { throw new Exception("Downgrade requested a download."); });
            throw new Exception("The older release should have been refused.");
        }
        catch (InvalidOperationException ex)
        {
            if (!ex.Message.Contains("already has screen reader version")) throw;
        }
        File.Delete(Path.Combine(game, InstallerEngine.ReadmeName));
        AssertNoDownload(game, new GitHubRelease(nextRelease.Tag, nextRelease.Version, null, 0, null));
        Console.WriteLine("PASS: fresh install, metadata-only checks and hosted update with README, safe-list and settings preservation.");
        return 0;
    }
}
