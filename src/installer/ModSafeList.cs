using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using File = FfxScreenReader.Setup.InstallerFiles;

namespace FfxScreenReader.Setup
{
    internal static class ModSafeList
    {
        public const string FileName = "FFX_SR_ModSafeList.txt";
        private const string ChangeLockName = ".ffx-sr-setup.lock";
        private const int MaxBytes = 1024 * 1024;
        private const int MaxEntries = 1024;
        private static readonly Encoding Utf8 = new UTF8Encoding(false, true);

        // The same game directory serializes setup and explicit saves, even when
        // two setup processes reached it using different Windows path spellings.
        public static FileStream LockChanges(string game)
        {
            InstallerPaths.CheckUnlinkedDirectoryPath(game);
            try
            {
                return new FileStream(InstallerPaths.Native(InstallerPaths.InGame(game, ChangeLockName)),
                    FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096, FileOptions.DeleteOnClose);
            }
            catch (IOException ex)
            {
                throw new IOException("Setup could not reserve this game folder for changes. Close other setup windows and try again. " + ex.Message, ex);
            }
        }

        public static bool IsModule(string name)
        {
            string extension = Path.GetExtension(name);
            return extension.Equals(".dll", StringComparison.OrdinalIgnoreCase) ||
                extension.Equals(".asi", StringComparison.OrdinalIgnoreCase);
        }

        public static bool IsSetupTree(string name)
        {
            int separator = name.IndexOf('\\');
            string top = separator < 0 ? name : name.Substring(0, separator);
            return top.Equals("FFX_SR_Install_Backups", StringComparison.OrdinalIgnoreCase) ||
                top.Equals("FFX_SR_LAA_Backups", StringComparison.OrdinalIgnoreCase) ||
                top.Equals("PrismRuntime", StringComparison.OrdinalIgnoreCase) ||
                top.StartsWith(".PrismRuntime.backup.", StringComparison.OrdinalIgnoreCase) ||
                top.StartsWith(".PrismRuntime.stage.", StringComparison.OrdinalIgnoreCase);
        }

        public static void RequireEligible(string name, ICollection<string> stock)
        {
            if (IsSetupTree(name) || Array.Exists(InstallerEngine.PayloadNames,
                item => String.Equals(item, name, StringComparison.OrdinalIgnoreCase)))
                throw new InvalidDataException("Setup needs this path for its own files: " + name + ". Choose a mod that uses a different path.");
            if (Array.Exists(InstallerEngine.LegacyNames,
                item => String.Equals(item, name, StringComparison.OrdinalIgnoreCase)))
                throw new InvalidDataException("Setup must back up and remove this legacy screen reader file: " + name + ".");
            if (stock.Contains(name))
                throw new InvalidDataException("This is an original game path verified by setup: " + name + ".");
        }

        private static string NormalizeEntry(string entry)
        {
            string name = InstallerPaths.Relative(entry);
            foreach (string part in name.Split('\\'))
                if (part.EndsWith(" ", StringComparison.Ordinal) || part.EndsWith(".", StringComparison.Ordinal))
                    throw new InvalidDataException("Safe list paths cannot end a name with a space or period: " + name + ".");
            if (!IsModule(name)) throw new InvalidDataException("Choose a DLL or ASI file: " + name + ".");
            return name;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
        private static extern uint GetLongPathNameW(string path, StringBuilder buffer, uint length);

        private static string LongPath(string path)
        {
            string native = InstallerPaths.Native(path);
            var buffer = new StringBuilder(native.Length + 1);
            uint length = GetLongPathNameW(native, buffer, (uint)buffer.Capacity);
            if (length >= buffer.Capacity)
            {
                buffer = new StringBuilder((int)length);
                length = GetLongPathNameW(native, buffer, (uint)buffer.Capacity);
            }
            if (length == 0) throw new Win32Exception(Marshal.GetLastWin32Error(), "Could not resolve the selected path: " + path + ".");
            if (length >= buffer.Capacity) throw new IOException("The selected path changed. Try again.");
            return InstallerPaths.Display(buffer.ToString());
        }

        private static string RelativeTo(string root, string path)
        {
            if (!path.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Choose a file inside the selected game folder: " + path + ".");
            return NormalizeEntry(path.Substring(root.Length));
        }

        private static string OnDiskName(string game, string path)
        {
            // Resolve 8.3 names to the same on-disk names the cleanup scan observes.
            string canonicalRoot = LongPath(game).TrimEnd('\\') + "\\";
            return RelativeTo(canonicalRoot, LongPath(path));
        }

        public static string SelectFile(string game, string path, ICollection<string> stock)
        {
            InstallerPaths.CheckUnlinkedDirectoryPath(game);
            string root = InstallerPaths.Full(game).TrimEnd('\\') + "\\";
            string selected = InstallerPaths.Full(path);
            if (!selected.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                root = LongPath(game).TrimEnd('\\') + "\\";
            string relative = RelativeTo(root, selected);
            InstallerPaths.CheckOrdinaryFile(root, relative);
            if (!File.Exists(selected)) throw new FileNotFoundException("The selected file is missing: " + selected + ".");
            relative = OnDiskName(game, path);
            RequireEligible(relative, stock);
            return relative;
        }

        public static HashSet<string> ForInstallation(string game, ICollection<string> stock)
        {
            var protectedPaths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string entry in Read(game))
            {
                InstallerPaths.CheckOrdinaryFile(game, entry);
                string path = InstallerPaths.InGame(game, entry);
                string name = File.Exists(path) ? OnDiskName(game, path) : entry;
                try { RequireEligible(name, stock); }
                catch (InvalidDataException ex)
                {
                    throw new InvalidDataException(ex.Message + " Open Mod safe list and remove this entry before installing.", ex);
                }
                protectedPaths.Add(name);
            }
            return protectedPaths;
        }

        public static HashSet<string> Read(string game)
        {
            var entries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            InstallerPaths.CheckUnlinkedDirectoryPath(game);
            string path = InstallerPaths.InGame(game, FileName);
            try
            {
                FileAttributes attributes = File.GetAttributes(path);
                if ((attributes & (FileAttributes.Directory | FileAttributes.ReparsePoint)) != 0)
                    throw new InvalidDataException("The mod safe list must be an ordinary file.");
                byte[] bytes;
                using (var file = new FileStream(InstallerPaths.Native(path), FileMode.Open, FileAccess.Read, FileShare.Read | FileShare.Delete))
                using (var reader = new BinaryReader(file))
                    bytes = reader.ReadBytes(MaxBytes + 1);
                if (bytes.Length > MaxBytes) throw new InvalidDataException("The mod safe list is too large.");
                int count = 0;
                string text = Utf8.GetString(bytes);
                using (var lines = new StringReader(text.TrimStart('\uFEFF')))
                {
                    string line;
                    while ((line = lines.ReadLine()) != null)
                    {
                        if (line.Length == 0) continue;
                        if (++count > MaxEntries) throw new InvalidDataException("The mod safe list has too many entries.");
                        entries.Add(NormalizeEntry(line));
                    }
                }
                return entries;
            }
            catch (FileNotFoundException) { return entries; }
            catch (Exception ex)
            {
                if (!(ex is InvalidDataException || ex is ArgumentException)) throw;
                throw new InvalidDataException("Could not read " + FileName + ". Restore or correct the file before installing. " + ex.Message, ex);
            }
        }

        public static void Save(string game, HashSet<string> original, IEnumerable<string> paths, ICollection<string> stock)
        {
            var entries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string path in paths)
            {
                string name = NormalizeEntry(path);
                RequireEligible(name, stock);
                entries.Add(name);
            }
            if (entries.Count > MaxEntries) throw new InvalidDataException("The mod safe list has too many entries.");
            var ordered = new List<string>(entries);
            ordered.Sort(StringComparer.OrdinalIgnoreCase);
            byte[] data = Utf8.GetBytes(String.Join("\r\n", ordered));
            if (data.Length > MaxBytes) throw new InvalidDataException("The mod safe list is too large.");
            using (LockChanges(game))
            {
                HashSet<string> current = Read(game);
                if (!current.SetEquals(original))
                    throw new IOException("The mod safe list changed in another setup window. Close and reopen the safe list to see the latest entries.");
                if (current.SetEquals(entries)) return;
                string destination = InstallerPaths.InGame(game, FileName);
                string temp = destination + "." + Guid.NewGuid().ToString("N") + ".tmp";
                bool created = false;
                try
                {
                    using (var output = new FileStream(InstallerPaths.Native(temp), FileMode.CreateNew, FileAccess.Write, FileShare.None))
                    {
                        created = true;
                        output.Write(data, 0, data.Length);
                        output.Flush(true);
                    }
                    InstallerPaths.CheckOrdinaryFile(game, FileName);
                    if (File.Exists(destination))
                        System.IO.File.Replace(InstallerPaths.Native(temp), InstallerPaths.Native(destination), null);
                    else
                        System.IO.File.Move(InstallerPaths.Native(temp), InstallerPaths.Native(destination));
                }
                finally { if (created && File.Exists(temp)) File.Delete(temp); }
            }
        }
    }
}
