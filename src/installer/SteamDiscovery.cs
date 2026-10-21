using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace FfxScreenReader.Setup
{
    internal static class SteamDiscovery
    {
        private static string Normalize(string path)
        {
            string full = InstallerPaths.Full(path);
            return full.Length == Path.GetPathRoot(full).Length ? full : full.TrimEnd(Path.DirectorySeparatorChar);
        }

        public static bool IsGame(string path)
        {
            return File.Exists(Path.Combine(path, "FFX.exe")) &&
                File.Exists(Path.Combine(path, "FFX&X-2_LAUNCHER.exe"));
        }

        public static List<string> Find(IEnumerable<string> roots)
        {
            var libraries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var games = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string supplied in roots)
            {
                if (String.IsNullOrWhiteSpace(supplied)) continue;
                string root = Normalize(supplied);
                if (IsGame(root) || SteamAppManifest.MatchesGameLocation(root)) games.Add(root);
                if (String.Equals(Path.GetFileName(root), "steamapps", StringComparison.OrdinalIgnoreCase))
                    root = Path.GetDirectoryName(root);
                libraries.Add(root);
                string vdf = Path.Combine(root, "steamapps", "libraryfolders.vdf");
                if (!File.Exists(vdf)) continue;
                foreach (Match match in Regex.Matches(File.ReadAllText(vdf), "\"(?:path|[0-9]+)\"\\s*\"([^\"\\r\\n]+)\""))
                {
                    string path = match.Groups[1].Value.Replace("\\\\", "\\");
                    if (Path.IsPathRooted(path)) libraries.Add(Normalize(path));
                }
            }
            foreach (string library in libraries)
            {
                string apps = Path.Combine(library, "steamapps");
                string folder = "FINAL FANTASY FFX&FFX-2 HD Remaster";
                string manifest = Path.Combine(apps, "appmanifest_359870.acf");
                if (File.Exists(manifest))
                {
                    Match match = Regex.Match(File.ReadAllText(manifest), "\"installdir\"\\s*\"([^\"\\r\\n]+)\"");
                    if (match.Success) folder = match.Groups[1].Value;
                }
                if (folder == "." || folder == ".." || folder.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) continue;
                string candidate = InstallerPaths.Full(Path.Combine(apps, "common", folder));
                if (IsGame(candidate) || SteamAppManifest.MatchesGameLocation(candidate)) games.Add(candidate);
            }
            var result = new List<string>(games);
            result.Sort(StringComparer.OrdinalIgnoreCase);
            return result;
        }

        public static List<string> DefaultRoots()
        {
            var roots = new List<string>();
            foreach (RegistryKey hive in new[] { Registry.CurrentUser, Registry.LocalMachine })
            {
                foreach (string name in new[] { @"Software\Valve\Steam", @"Software\WOW6432Node\Valve\Steam" })
                {
                    try
                    {
                        using (RegistryKey key = hive.OpenSubKey(name, false))
                        {
                            if (key == null) continue;
                            foreach (string value in new[] { "SteamPath", "InstallPath" })
                            {
                                string path = key.GetValue(value) as string;
                                if (!String.IsNullOrWhiteSpace(path)) roots.Add(path);
                            }
                        }
                    }
                    catch (System.Security.SecurityException) { }
                    catch (UnauthorizedAccessException) { }
                }
            }
            foreach (string variable in new[] { "ProgramFiles(x86)", "ProgramFiles" })
            {
                string path = Environment.GetEnvironmentVariable(variable);
                if (!String.IsNullOrWhiteSpace(path)) roots.Add(Path.Combine(path, "Steam"));
            }
            return roots;
        }
    }
}
