using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

namespace FfxScreenReader.Setup
{
    internal sealed class MigrationPlan
    {
        public readonly BaseGameCatalog Catalog;
        public readonly List<string> Remove;
        private MigrationPlan(BaseGameCatalog catalog, List<string> remove) { Catalog = catalog; Remove = remove; }
        public static MigrationPlan Build(string game, Action<string> status) {
            LaunchRedirects.RequireNone(game);
            var catalogs = BaseGameCatalog.Load();
            string id = SteamAppManifest.GetInstalledManifest(game);
            BaseGameCatalog selected = null;
            if (id != null) {
                selected = catalogs.Find(delegate(BaseGameCatalog item) { return item.Manifest == id; });
                if (selected == null) throw new InvalidOperationException("This Steam game version is not supported by this installer. Download an updated installer. No files were changed.");
            } else {
                foreach (var catalog in catalogs)
                    if (catalog.Files["FFX.exe"].Matches(game) || catalog.Files["FFX&X-2_LAUNCHER.exe"].Matches(game)) {
                        if (selected != null) throw new InvalidOperationException("Game executables belong to different Steam versions. Verify game files in Steam, then run setup again. No files were changed.");
                        selected = catalog;
                    }
                if (selected == null) throw new InvalidOperationException("Could not identify the original game executables in this folder. Select the Steam game installation and verify its game files in Steam. No files were changed.");
            }
            return FromCatalog(game, selected, status);
        }
        // Tests supply small synthetic catalogs; the production entry point always uses pinned inventories.
        public static MigrationPlan FromCatalog(string game, BaseGameCatalog catalog, Action<string> status) {
            ValidateStock(game, catalog, status);
            return new MigrationPlan(catalog, AddedModules(game, catalog));
        }
        private static void ValidateStock(string game, BaseGameCatalog catalog, Action<string> status) {
            int checkedFiles = 0;
            foreach (var file in catalog.Files.Values) {
                if (checkedFiles++ % 100 == 0) status("Checking original game files: " + checkedFiles + " of " + catalog.Files.Count);
                if (!file.Matches(game)) throw new InvalidOperationException("Original game file is missing or modified: " + file.Name +
                    ". In Steam, open game Properties, Installed Files, Verify integrity of game files. Then run setup again. No original game files were replaced by setup.");
            }
        }
        private static List<string> AddedModules(string game, BaseGameCatalog catalog) {
            var result = new List<string>();
            var ours = new HashSet<string>(InstallerEngine.PayloadNames, StringComparer.OrdinalIgnoreCase);
            // Tolk removal is explicitly requested for the PRISM migration.
            // Its old helper libraries remain separately owned.
            ours.UnionWith(new[] { "nvdaControllerClient32.dll", "SAAPI32.dll", "dolapi32.dll" });
            var folders = new Queue<string>();
            folders.Enqueue(InstallerPaths.Full(game));
            string root = InstallerPaths.Full(game).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            int entries = 0;
            while (folders.Count != 0) {
                foreach (string path in Directory.GetFileSystemEntries(folders.Dequeue())) {
                    string name = InstallerPaths.Relative(path.Substring(root.Length));
                    if (name.Equals("FFX_SR_Install_Backups", StringComparison.OrdinalIgnoreCase) ||
                        name.Equals("FFX_SR_LAA_Backups", StringComparison.OrdinalIgnoreCase) ||
                        name.Equals("PrismRuntime", StringComparison.OrdinalIgnoreCase) ||
                        name.StartsWith(".PrismRuntime.backup.", StringComparison.OrdinalIgnoreCase) ||
                        name.StartsWith(".PrismRuntime.stage.", StringComparison.OrdinalIgnoreCase)) continue;
                    if (++entries > 200000) throw new IOException("Too many files to inspect safely. No files were changed.");
                    FileAttributes attributes = File.GetAttributes(path);
                    if ((attributes & FileAttributes.ReparsePoint) != 0) throw new IOException("Setup cannot inspect a linked file or subfolder: " + name + ". No files were changed.");
                    if ((attributes & FileAttributes.Directory) != 0) { folders.Enqueue(path); continue; }
                    string extension = Path.GetExtension(name);
                    if ((extension.Equals(".dll", StringComparison.OrdinalIgnoreCase) || extension.Equals(".asi", StringComparison.OrdinalIgnoreCase)) &&
                        !ours.Contains(name) && !catalog.Files.ContainsKey(name)) result.Add(name);
                }
            }
            result.Sort(StringComparer.OrdinalIgnoreCase);
            return result;
        }
        public void Verify(string game, Action<string> status) {
            ValidateStock(game, Catalog, status);
            var remaining = AddedModules(game, Catalog);
            if (remaining.Count != 0) throw new IOException("Another added DLL or ASI appeared during setup: " + remaining[0] + ". Close the mod manager and try again.");
        }
        public void WriteReport(string game, string backup) {
            var lines = new List<string> { "Steam depot: 359871", "Verified manifest: " + Catalog.Manifest, "Added DLL/ASI files removed (backups preserve their relative paths):" };
            foreach (string name in Remove) lines.Add(name + "  SHA256 " + InstallerEngine.Hash(File.ReadAllBytes(InstallerPaths.InGame(game, name))));
            File.WriteAllLines(Path.Combine(backup, "INSTALL_REPORT.txt"), lines.ToArray());
        }
    }
}
