using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using System.Security.Cryptography;
using System.Text;

namespace FfxScreenReader.Setup
{
    internal sealed class StockFile
    {
        public readonly string Name, Sha1;
        public readonly long Size;
        public StockFile(string name, string sha1, long size) { Name = InstallerPaths.Relative(name); Sha1 = sha1; Size = size; }
        public bool Matches(string game) {
            string path = InstallerPaths.InGame(game, Name);
            InstallerPaths.CheckOrdinaryFile(game, Name);
            if (!File.Exists(path) || File.Length(path) != Size) return false;
            using (var file = File.OpenRead(path)) using (var sha = SHA1.Create()) {
                if (BitConverter.ToString(sha.ComputeHash(file)).Replace("-", "").Equals(Sha1, StringComparison.OrdinalIgnoreCase)) return true;
                return Name.Equals("FFX.exe", StringComparison.OrdinalIgnoreCase) && LargeAddressIdentity.MatchesFlagOnly(file, Sha1);
            }
        }
    }

    internal sealed class BaseGameCatalog
    {
        public readonly string Manifest;
        public readonly Dictionary<string, StockFile> Files = new Dictionary<string, StockFile>(StringComparer.OrdinalIgnoreCase);
        public BaseGameCatalog(string manifest, IEnumerable<StockFile> files) {
            Manifest = manifest;
            foreach (var file in files) Files.Add(file.Name, file);
        }
        // Derived from official local Steam depot metadata; no game binaries are distributed.
        public static readonly string[] ResourceNames = {"Baseline.1235377778411650212", "Baseline.5168047039601173807"};
        private static readonly string[] ResourceHashes = {"094195a8d58d667893d6fc05bfc71a5ced1067113e0cb72d16fb31cc9fa8820f", "b27f00290612c241d6ce6325e330157a713381fc6a178ba0af8f8f26fe0f7a43"};
        public static List<BaseGameCatalog> Load() {
            var result = new List<BaseGameCatalog>();
            for (int i = 0; i < ResourceNames.Length; i++) {
                byte[] bytes = Program.ReadResource(ResourceNames[i]);
                if (InstallerEngine.Hash(bytes) != ResourceHashes[i]) throw new InvalidDataException("Installer base-game inventory is damaged. Download a fresh installer.");
                var lines = new UTF8Encoding(false, true).GetString(bytes).Split('\n');
                string id = ResourceNames[i].Substring("Baseline.".Length);
                if (lines[0] != "359871\t" + id) throw new InvalidDataException("Installer base-game inventory identity is invalid.");
                var files = new List<StockFile>();
                for (int line = 1; line < lines.Length; line++) {
                    if (lines[line].Length == 0) continue;
                    string[] parts = lines[line].Split('	');
                    long size;
                    if (parts.Length != 3 || parts[0].Length != 40 || !Int64.TryParse(parts[1], out size) || size < 0)
                        throw new InvalidDataException("Invalid base-game inventory entry.");
                    files.Add(new StockFile(parts[2], parts[0], size));
                }
                if (files.Count != 1435) throw new InvalidDataException("Incomplete base-game inventory.");
                result.Add(new BaseGameCatalog(id, files));
            }
            return result;
        }
    }
}
