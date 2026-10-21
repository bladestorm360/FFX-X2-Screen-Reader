using System;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

namespace FfxScreenReader.Setup
{
    internal static class InstallerPaths
    {
        public static string Full(string path) { return Path.GetFullPath(Display(path)); }
        public static string Native(string path) {
            // Normalize before applying the extended Win32 prefix. This works
            // regardless of the machine's LongPathsEnabled registry policy.
            string full = Full(path);
            if (full.StartsWith(@"\\?\", StringComparison.Ordinal)) return full;
            return full.StartsWith(@"\\", StringComparison.Ordinal) ? @"\\?\UNC\" + full.Substring(2) : @"\\?\" + full;
        }
        public static string Display(string path) {
            if (String.IsNullOrEmpty(path)) return path;
            if (path.StartsWith(@"\\?\UNC\", StringComparison.OrdinalIgnoreCase)) return @"\\" + path.Substring(8);
            return path.StartsWith(@"\\?\", StringComparison.Ordinal) ? path.Substring(4) : path;
        }
        public static string Relative(string name) {
            string value = name.Replace('/', '\\');
            if (String.IsNullOrWhiteSpace(value) || Path.IsPathRooted(value)) throw new InvalidDataException("Invalid relative installation path.");
            foreach (string part in value.Split('\\'))
                if (part == "." || part == ".." || part.Length == 0 || part.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
                    throw new InvalidDataException("Invalid relative installation path: " + name);
            return value;
        }
        public static string InGame(string game, string name) {
            string root = Full(game).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            string path = Full(Path.Combine(root, Relative(name)));
            if (!path.StartsWith(root, StringComparison.OrdinalIgnoreCase)) throw new IOException("Installation path escapes the game folder.");
            return path;
        }
        public static void CheckOrdinaryFile(string game, string name) {
            string path = InGame(game, name);
            if (Directory.Exists(path)) throw new IOException("Expected a file, but found a directory: " + path);
            string check = path;
            while (!String.Equals(check, Full(game).TrimEnd(Path.DirectorySeparatorChar), StringComparison.OrdinalIgnoreCase)) {
                if ((File.Exists(check) || Directory.Exists(check)) && (File.GetAttributes(check) & FileAttributes.ReparsePoint) != 0)
                    throw new IOException("Setup cannot inspect or change a linked file or subfolder: " + check + ". No files were changed.");
                check = Path.GetDirectoryName(check);
            }
        }
    }
}
