using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;
using System.Xml;
using System.Security.Cryptography;
using Microsoft.Win32;

namespace FfxScreenReader.Setup
{
    internal static class LaunchRedirectRepair
    {
        private const string Root = @"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options";
        private static readonly string[] Images = { "FFX&X-2_LAUNCHER.exe", "FFX.exe", "FFX-2.exe", "FFX-2_LM.exe", "FFX&X-2_Will.exe" };
        internal sealed class Value
        {
            public readonly RegistryValueKind Kind;
            public readonly object Data;
            public Value(RegistryValueKind kind, object data) { Kind = kind; Data = data; }
        }
        internal sealed class Entry
        {
            public readonly RegistryView View;
            public readonly string RelativeKey;
            public readonly RegistryValueKind Kind;
            public readonly string Data;
            public readonly string FilterFullPath;
            public string FilterPath { get { return FilterFullPath; } }
            public Entry(RegistryView view, string key, RegistryValueKind kind, string data)
             : this(view, key, kind, data, null) { }
            public Entry(RegistryView view, string key, RegistryValueKind kind, string data, string filterFullPath)
            { View = view; RelativeKey = key; Kind = kind; Data = data; FilterFullPath = filterFullPath; }
        }
        internal abstract class Store
        {
            public virtual RegistryView[] Views { get { return new[] { RegistryView.Registry32, RegistryView.Registry64 }; } }
            public abstract string[] Children(RegistryView view, string key);
            public abstract Value Get(RegistryView view, string key, string name);
            public abstract void DeleteDebugger(Entry entry);
            public abstract void SetDebugger(Entry entry);
        }
        private sealed class WindowsStore : Store
        {
            // Modern Windows shares IFEO across bitness; enumerate once to avoid alias deletion.
            public override RegistryView[] Views { get { return new[] { Environment.Is64BitOperatingSystem ? RegistryView.Registry64 : RegistryView.Registry32 }; } }
            public override string[] Children(RegistryView view, string key) {
                using (var hive = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, view))
                using (var child = hive.OpenSubKey(Root + "\\" + key, false))
                    return child == null ? new string[0] : child.GetSubKeyNames();
            }
            public override Value Get(RegistryView view, string key, string name) {
                using (var hive = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, view))
                using (var child = hive.OpenSubKey(Root + "\\" + key, false)) {
                    if (child == null) return null;
                    foreach (string value in child.GetValueNames())
                        if (String.Equals(value, name, StringComparison.OrdinalIgnoreCase))
                            return new Value(child.GetValueKind(value), child.GetValue(value, null, RegistryValueOptions.DoNotExpandEnvironmentNames));
                    return null;
                }
            }
            public override void DeleteDebugger(Entry entry) {
                using (var hive = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, entry.View))
                using (var child = hive.OpenSubKey(Root + "\\" + entry.RelativeKey, true)) {
                    if (child == null) throw new IOException("The launch-redirection registry key disappeared.");
                    RequireSame(Get(entry.View, entry.RelativeKey, "Debugger"), entry);
                    child.DeleteValue("Debugger", true);
                }
            }
            public override void SetDebugger(Entry entry) {
                using (var hive = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, entry.View))
                using (var child = hive.OpenSubKey(Root + "\\" + entry.RelativeKey, true)) {
                    if (child == null) throw new IOException("The original launch-redirection registry key is missing.");
                    if (Get(entry.View, entry.RelativeKey, "Debugger") != null)
                        throw new IOException("A launch redirection already exists; it was preserved.");
                    child.SetValue("Debugger", entry.Data, entry.Kind);
                }
            }
        }
        private static bool Same(Value value, Entry entry) {
            return value != null && value.Kind == entry.Kind && value.Data is string &&
                String.Equals((string)value.Data, entry.Data, StringComparison.Ordinal);
        }
        private static void RequireSame(Value value, Entry entry) {
            if (!Same(value, entry)) throw new IOException("Launch redirection changed during setup: " + entry.RelativeKey + ". No changed value was overwritten.");
        }
        private static string Normalize(string path) {
            return InstallerPaths.Full(path).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        }
        private static bool MatchingFilter(Store store, RegistryView view, string key, string expected) {
            Value path = store.Get(view, key, "FilterFullPath");
            if (path == null || path.Kind != RegistryValueKind.String || !(path.Data is string)) return false;
            try { return Path.IsPathRooted((string)path.Data) && String.Equals(Normalize((string)path.Data), expected, StringComparison.OrdinalIgnoreCase); }
            catch (ArgumentException) { return false; }
            catch (NotSupportedException) { return false; }
            catch (PathTooLongException) { return false; }
        }
        private static void Add(Store store, RegistryView view, string key, List<Entry> entries) {
            Value value = store.Get(view, key, "Debugger");
            if (value == null) return;
            if ((value.Kind != RegistryValueKind.String && value.Kind != RegistryValueKind.ExpandString) || !(value.Data is string))
                throw new IOException("Unsupported Debugger registry value at " + key + ". No registry values were changed.");
            Value filter = key.IndexOf('\\') < 0 ? null : store.Get(view, key, "FilterFullPath");
            string data = (string)value.Data; string filterPath = filter == null ? null : filter.Data as string;
            int characters = data.Length + key.Length + (filterPath == null ? 0 : filterPath.Length);
            foreach (Entry entry in entries) characters += entry.Data.Length + entry.RelativeKey.Length + (entry.FilterFullPath == null ? 0 : entry.FilterFullPath.Length);
            if (entries.Count >= 512 || data.Length > 32768 || (filterPath != null && filterPath.Length > 32768) || characters > 131072)
                throw new IOException("Too much launch-redirection data to back up safely. No registry values were changed.");
            entries.Add(new Entry(view, key, value.Kind, data, filterPath));
        }
        public static List<Entry> Read(string game) { return Read(game, new WindowsStore()); }
        internal static List<Entry> Read(string game, Store store) {
            var entries = new List<Entry>();
            foreach (RegistryView view in store.Views)
                foreach (string image in Images) {
                    Add(store, view, image, entries);
                    string[] children = store.Children(view, image);
                    if (children.Length > 4096) throw new IOException("Too many launch-redirection filters to inspect safely. No registry values were changed.");
                    foreach (string child in children) {
                        ValidatePart(child);
                        string key = image + "\\" + child;
                        // Windows enables path-specific IFEO filters on the image root.
                        if (MatchingFilterRoot(store, view, image) && MatchingFilter(store, view, key, Normalize(Path.Combine(game, image))))
                            Add(store, view, key, entries);
                    }
                }
            return entries;
        }
        private static bool MatchingFilterRoot(Store store, RegistryView view, string image) {
            Value value = store.Get(view, image, "UseFilter");
            return value != null && ((value.Kind == RegistryValueKind.DWord && value.Data is int && (int)value.Data != 0) ||
                (value.Kind == RegistryValueKind.QWord && value.Data is long && (long)value.Data != 0));
        }
        private static void ValidatePart(string part) {
            if (String.IsNullOrEmpty(part) || part.Length > 255 || part == "." || part == ".." || part.IndexOf('\\') >= 0 || part.IndexOf('/') >= 0 || part.IndexOf('\0') >= 0)
                throw new InvalidDataException("Invalid launch-redirection registry key.");
        }
        private static void ValidateEntry(Entry entry) {
            string[] parts = entry.RelativeKey.Split('\\');
            if (parts.Length < 1 || parts.Length > 2 || Array.FindIndex(Images, delegate(string image) { return String.Equals(image, parts[0], StringComparison.OrdinalIgnoreCase); }) < 0)
                throw new InvalidDataException("Backup refers to an unrelated registry key.");
            foreach (string part in parts) ValidatePart(part);
            if ((parts.Length == 2 && (String.IsNullOrEmpty(entry.FilterFullPath) || !Path.IsPathRooted(entry.FilterFullPath))) ||
                (parts.Length == 1 && entry.FilterFullPath != null)) throw new InvalidDataException("Invalid backup filter association.");
            if ((entry.View != RegistryView.Registry32 && entry.View != RegistryView.Registry64) ||
                (entry.Kind != RegistryValueKind.String && entry.Kind != RegistryValueKind.ExpandString) || entry.Data == null)
                throw new InvalidDataException("Invalid launch-redirection backup value.");
        }
        private static string BackupPath(string path, bool creating) {
            string full = InstallerPaths.Full(path);
            string parent = Path.GetDirectoryName(full);
            if (!Directory.Exists(parent)) throw new IOException("Create the backup folder before repairing launch redirections.");
            for (string item = full; item != null; item = Path.GetDirectoryName(item))
                if ((File.Exists(item) || Directory.Exists(item)) && (File.GetAttributes(item) & FileAttributes.ReparsePoint) != 0)
                    throw new IOException("Launch-redirection backup cannot use a linked path.");
            if (Directory.Exists(full) || (creating && File.Exists(full))) throw new IOException("Launch-redirection backup path already exists.");
            return full;
        }
        private static string BackupHash(List<Entry> entries) {
            using (var bytes = new MemoryStream()) {
                using (var writer = new BinaryWriter(bytes, System.Text.Encoding.UTF8, true)) {
                    writer.Write(entries.Count);
                    foreach (Entry entry in entries) {
                        writer.Write((int)entry.View); writer.Write(entry.RelativeKey); writer.Write((int)entry.Kind);
                        writer.Write(entry.Data.Length); foreach (char c in entry.Data) writer.Write((ushort)c);
                        writer.Write(entry.FilterFullPath != null); if (entry.FilterFullPath != null) writer.Write(entry.FilterFullPath);
                    }
                }
                using (var sha = SHA256.Create()) return Convert.ToBase64String(sha.ComputeHash(bytes.ToArray()));
            }
        }
        private static void WriteBackup(string path, List<Entry> entries) {
            var xml = new XmlDocument { XmlResolver = null };
            XmlElement root = xml.CreateElement("LaunchRedirectBackup");
            root.SetAttribute("version", "1"); root.SetAttribute("sha256", BackupHash(entries)); xml.AppendChild(root);
            foreach (Entry entry in entries) {
                ValidateEntry(entry);
                XmlElement node = xml.CreateElement("Entry");
                node.SetAttribute("view", ((int)entry.View).ToString());
                node.SetAttribute("key", entry.RelativeKey);
                node.SetAttribute("kind", ((int)entry.Kind).ToString());
                if (entry.FilterFullPath != null) node.SetAttribute("filter", entry.FilterFullPath);
                // Base64 preserves even unusual registry UTF-16 code units exactly.
                byte[] raw = new byte[entry.Data.Length * 2];
                for (int i = 0; i < entry.Data.Length; i++) { raw[i * 2] = (byte)entry.Data[i]; raw[i * 2 + 1] = (byte)(entry.Data[i] >> 8); }
                node.InnerText = Convert.ToBase64String(raw); root.AppendChild(node);
            }
            using (var file = new FileStream(InstallerPaths.Native(path), FileMode.CreateNew, FileAccess.Write, FileShare.None)) xml.Save(file);
        }
        private static List<Entry> ParseBackup(string path) {
            var xml = new XmlDocument { XmlResolver = null };
            using (var file = new FileStream(InstallerPaths.Native(path), FileMode.Open, FileAccess.Read, FileShare.Read)) {
                if (file.Length > 1048576) throw new InvalidDataException("Launch-redirection backup is too large.");
                using (var reader = XmlReader.Create(file, new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null })) xml.Load(reader);
            }
            XmlElement root = xml.DocumentElement;
            if (root == null || root.Name != "LaunchRedirectBackup" || root.GetAttribute("version") != "1" || root.Attributes.Count != 2) throw new InvalidDataException("Invalid launch-redirection backup.");
            var result = new List<Entry>(); var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (XmlNode child in root.ChildNodes) {
                XmlElement node = child as XmlElement;
                if (node == null || node.Name != "Entry" || (node.Attributes.Count != 3 && node.Attributes.Count != 4) || (node.ChildNodes.Count != 0 && (node.ChildNodes.Count != 1 || !(node.FirstChild is XmlText)))) throw new InvalidDataException("Invalid launch-redirection backup entry.");
                int view, kind;
                if (!Int32.TryParse(node.GetAttribute("view"), out view) || !Int32.TryParse(node.GetAttribute("kind"), out kind)) throw new InvalidDataException("Invalid backup registry types.");
                byte[] raw;
                try { raw = Convert.FromBase64String(node.InnerText); }
                catch (FormatException) { throw new InvalidDataException("Invalid backup registry data."); }
                if (raw.Length % 2 != 0) throw new InvalidDataException("Invalid backup registry string.");
                char[] chars = new char[raw.Length / 2];
                for (int i = 0; i < chars.Length; i++) chars[i] = (char)(raw[i * 2] | raw[i * 2 + 1] << 8);
                var entry = new Entry((RegistryView)view, node.GetAttribute("key"), (RegistryValueKind)kind, new string(chars), node.HasAttribute("filter") ? node.GetAttribute("filter") : null);
                ValidateEntry(entry);
                if (!seen.Add(view + "|" + entry.RelativeKey) || result.Count >= 10000) throw new InvalidDataException("Duplicate or excessive backup registry entries.");
                result.Add(entry);
            }
            if (!String.Equals(root.GetAttribute("sha256"), BackupHash(result), StringComparison.Ordinal)) throw new InvalidDataException("Launch-redirection backup checksum failed. No registry values were changed.");
            return result;
        }
        public static List<Entry> ReadBackup(string backupPath) { return ParseBackup(BackupPath(backupPath, false)); }
        public static int Repair(string game, string backupPath) { return Repair(game, backupPath, new WindowsStore()); }
        internal static int Repair(string game, string backupPath, Store store) { return Repair(game, Read(game, store), backupPath, store); }
        public static int Repair(string game, IList<Entry> expectedEntries, string backupPath) { return Repair(game, expectedEntries, backupPath, new WindowsStore()); }
        private static void RequireSet(List<Entry> entries, List<Entry> current) {
            var expected = new Dictionary<string, Entry>(StringComparer.OrdinalIgnoreCase);
            foreach (Entry entry in entries) {
                ValidateEntry(entry); string key = (int)entry.View + "|" + entry.RelativeKey;
                if (expected.ContainsKey(key)) throw new IOException("Duplicate launch-redirection entries.");
                expected.Add(key, entry);
            }
            if (current.Count != entries.Count) throw new IOException("Launch redirections changed. Review the current list before repair; no registry values were changed.");
            foreach (Entry entry in current) {
                Entry old;
                if (!expected.TryGetValue((int)entry.View + "|" + entry.RelativeKey, out old) || (!Same(new Value(entry.Kind, entry.Data), old) || !String.Equals(entry.FilterFullPath, old.FilterFullPath, StringComparison.Ordinal)))
                    throw new IOException("Launch redirections changed. Review the current list before repair; no registry values were changed.");
            }
        }
        internal static int Repair(string game, IList<Entry> expectedEntries, string backupPath, Store store) {
            if (expectedEntries == null) throw new ArgumentNullException("expectedEntries");
            var entries = new List<Entry>(expectedEntries);
            RequireSet(entries, Read(game, store));
            string path = BackupPath(backupPath, true);
            foreach (Entry entry in entries) RequireSame(store.Get(entry.View, entry.RelativeKey, "Debugger"), entry);
            WriteBackup(path, entries);
            List<Entry> verified = ParseBackup(path);
            RequireSet(entries, verified);
            // Reread the complete eligible set, including filter state, before any mutation.
            List<Entry> current = Read(game, store);
            RequireSet(entries, current);
            var deleted = new List<Entry>();
            try {
                foreach (Entry entry in entries) {
                    RequireSame(store.Get(entry.View, entry.RelativeKey, "Debugger"), entry);
                    CheckRestoreFilter(store, entry);
                    string[] parts = entry.RelativeKey.Split('\\');
                    if (parts.Length == 2 && (!MatchingFilterRoot(store, entry.View, parts[0]) || !MatchingFilter(store, entry.View, entry.RelativeKey, Normalize(Path.Combine(game, parts[0])))))
                        throw new IOException("Launch-redirection filter changed: " + entry.RelativeKey + ". The changed value was preserved.");
                    store.DeleteDebugger(entry); deleted.Add(entry);
                }
            } catch (Exception failure) {
                var errors = new List<string>();
                foreach (Entry entry in deleted) try { CheckRestoreFilter(store, entry); store.SetDebugger(entry); } catch (Exception ex) { errors.Add(entry.RelativeKey + ": " + ex.Message); }
                throw new IOException("Launch-redirection repair failed. " + (errors.Count == 0 ? "Deleted values were restored." : "Some values could not be restored: " + String.Join("; ", errors.ToArray())) + " Backup: " + path + ". " + failure.Message, failure);
            }
            return entries.Count;
        }
        private static void CheckRestoreFilter(Store store, Entry entry) {
            if (entry.FilterFullPath == null) return;
            string image = entry.RelativeKey.Split('\\')[0];
            Value filter = store.Get(entry.View, entry.RelativeKey, "FilterFullPath");
            if (!MatchingFilterRoot(store, entry.View, image) || filter == null || filter.Kind != RegistryValueKind.String ||
                !String.Equals(filter.Data as string, entry.FilterFullPath, StringComparison.Ordinal))
                throw new IOException("Launch-redirection filter changed: " + entry.RelativeKey + ". No changed filter was overwritten.");
        }
        public static int Restore(string backupPath) { return Restore(backupPath, new WindowsStore()); }
        internal static int Restore(string backupPath, Store store) { return Restore(backupPath, ReadBackup(backupPath), store); }
        public static int Restore(string backupPath, IList<Entry> expectedEntries) { return Restore(backupPath, expectedEntries, new WindowsStore()); }
        internal static int Restore(string backupPath, IList<Entry> expectedEntries, Store store) {
            if (expectedEntries == null) throw new ArgumentNullException("expectedEntries");
            string path = BackupPath(backupPath, false);
            List<Entry> entries = ParseBackup(path);
            RequireSet(new List<Entry>(expectedEntries), entries);
            foreach (Entry entry in entries) CheckRestoreFilter(store, entry);
            foreach (Entry entry in entries) if (store.Get(entry.View, entry.RelativeKey, "Debugger") != null)
                throw new IOException("A launch redirection already exists at " + entry.RelativeKey + "; no values were overwritten.");
            var restored = new List<Entry>();
            try { foreach (Entry entry in entries) { CheckRestoreFilter(store, entry); store.SetDebugger(entry); restored.Add(entry); } }
            catch (Exception failure) {
                var errors = new List<string>();
                foreach (Entry entry in restored) try { RequireSame(store.Get(entry.View, entry.RelativeKey, "Debugger"), entry); store.DeleteDebugger(entry); }
                    catch (Exception ex) { errors.Add(entry.RelativeKey + ": " + ex.Message); }
                throw new IOException("Launch-redirection restore failed. " + (errors.Count == 0 ? "Newly restored values were removed." : "Some values could not be rolled back: " + String.Join("; ", errors.ToArray())) + " Backup: " + path + ". " + failure.Message, failure);
            }
            return entries.Count;
        }
    }
}
