using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.Win32;
using FfxScreenReader.Setup;

internal static class LaunchRedirectRepairTests
{
    private sealed class MemoryStore : LaunchRedirectRepair.Store
    {
        internal readonly Dictionary<string, LaunchRedirectRepair.Value> Values = new Dictionary<string, LaunchRedirectRepair.Value>(StringComparer.OrdinalIgnoreCase);
        internal int Deletes, FailDelete, Sets, FailSet;
        internal Action AfterRead;
        private static string Id(RegistryView view, string key, string name) { return (int)view + "|" + key + "|" + name; }
        public override string[] Children(RegistryView view, string key) {
            var children = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string prefix = (int)view + "|" + key + "\\";
            foreach (string item in Values.Keys) if (item.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) {
                string rest = item.Substring(prefix.Length); children.Add(rest.Split('|')[0]);
            }
            return new List<string>(children).ToArray();
        }
        public override LaunchRedirectRepair.Value Get(RegistryView view, string key, string name) {
            LaunchRedirectRepair.Value value; Values.TryGetValue(Id(view, key, name), out value);
            if (AfterRead != null) { Action action = AfterRead; AfterRead = null; action(); }
            return value;
        }
        internal void Put(RegistryView view, string key, string name, RegistryValueKind kind, object data) { Values[Id(view, key, name)] = new LaunchRedirectRepair.Value(kind, data); }
        public override void DeleteDebugger(LaunchRedirectRepair.Entry entry) {
            if (++Deletes == FailDelete) throw new IOException("Synthetic delete failure: " + entry.RelativeKey);
            LaunchRedirectRepair.Value value = Get(entry.View, entry.RelativeKey, "Debugger");
            if (value == null || value.Kind != entry.Kind || !Object.Equals(value.Data, entry.Data)) throw new IOException("Synthetic changed value");
            Values.Remove(Id(entry.View, entry.RelativeKey, "Debugger"));
        }
        public override void SetDebugger(LaunchRedirectRepair.Entry entry) {
            if (++Sets == FailSet) throw new IOException("Synthetic set failure: " + entry.RelativeKey);
            if (Get(entry.View, entry.RelativeKey, "Debugger") != null) throw new IOException("Synthetic existing value preserved");
            Put(entry.View, entry.RelativeKey, "Debugger", entry.Kind, entry.Data);
        }
    }
    private static void Reject(Action action, Action<bool, string> assert, string text) {
        bool failed = false;
        try { action(); }
        catch (Exception ex) { if (ex.Message.IndexOf(text, StringComparison.OrdinalIgnoreCase) < 0) throw; failed = true; }
        assert(failed, "Expected repair refusal: " + text);
    }
    private static MemoryStore Fixture(string game) {
        var store = new MemoryStore();
        store.Put(RegistryView.Registry32, "FFX.exe", "Debugger", RegistryValueKind.ExpandString, "%ROOT%\\old.exe --flag");
        store.Put(RegistryView.Registry32, "FFX.exe", "UseFilter", RegistryValueKind.DWord, 1);
        store.Put(RegistryView.Registry32, "FFX.exe\\target", "FilterFullPath", RegistryValueKind.String, Path.Combine(game, "FFX.exe"));
        store.Put(RegistryView.Registry32, "FFX.exe\\target", "Debugger", RegistryValueKind.String, "filtered.exe");
        store.Put(RegistryView.Registry32, "FFX.exe\\other", "FilterFullPath", RegistryValueKind.String, Path.Combine(game, "other", "FFX.exe"));
        store.Put(RegistryView.Registry32, "FFX.exe\\other", "Debugger", RegistryValueKind.String, "different installation");
        store.Put(RegistryView.Registry64, "FFX&X-2_LAUNCHER.exe", "Debugger", RegistryValueKind.String, "launcher redirect");
        store.Put(RegistryView.Registry64, "Unrelated.exe", "Debugger", RegistryValueKind.String, "unrelated");
        store.Put(RegistryView.Registry64, "FFX.exe", "GlobalFlag", RegistryValueKind.DWord, 123);
        return store;
    }
    public static void Run(string fixture, Action<bool, string> assert) {
        string folder = Path.Combine(fixture, "launch-repair"); Directory.CreateDirectory(folder);
        string game = Path.Combine(folder, "selected game");
        var store = Fixture(game); int originalCount = store.Values.Count;
        List<LaunchRedirectRepair.Entry> preview = LaunchRedirectRepair.Read(game, store);
        assert(preview.Count == 3, "Read root and exact enabled path filters across synthetic views");
        string backup = Path.Combine(folder, "success.xml");
        assert(LaunchRedirectRepair.Repair(game, preview, backup, store) == 3, "Repair exact confirmed set");
        assert(store.Values.Count == originalCount - 3, "Only three Debugger values deleted");
        assert(store.Get(RegistryView.Registry32, "FFX.exe", "UseFilter") != null, "UseFilter preserved");
        assert(store.Get(RegistryView.Registry32, "FFX.exe\\target", "FilterFullPath") != null, "FilterFullPath preserved");
        assert(store.Get(RegistryView.Registry32, "FFX.exe\\other", "Debugger") != null, "Different-installation redirect preserved");
        assert(store.Get(RegistryView.Registry64, "Unrelated.exe", "Debugger") != null, "Unrelated image preserved");
        assert(store.Get(RegistryView.Registry64, "FFX.exe", "GlobalFlag") != null, "Other values preserved");
        assert(LaunchRedirectRepair.Restore(backup, store) == 3 && store.Values.Count == originalCount, "Restore exact removed values");
        assert((string)store.Get(RegistryView.Registry32, "FFX.exe", "Debugger").Data == "%ROOT%\\old.exe --flag" && store.Get(RegistryView.Registry32, "FFX.exe", "Debugger").Kind == RegistryValueKind.ExpandString, "Raw ExpandString restored unchanged");
        Reject(delegate { LaunchRedirectRepair.Restore(backup, store); }, assert, "already exists");
        assert(store.Values.Count == originalCount, "Existing restored destination untouched");
        store = Fixture(game); preview = LaunchRedirectRepair.Read(game, store);
        store.Put(RegistryView.Registry32, "FFX.exe", "Debugger", RegistryValueKind.String, "changed");
        Reject(delegate { LaunchRedirectRepair.Repair(game, preview, Path.Combine(folder, "changed.xml"), store); }, assert, "changed");
        assert(store.Deletes == 0, "Changed confirmed value refuses before deletion");
        store = Fixture(game); preview = LaunchRedirectRepair.Read(game, store);
        store.Put(RegistryView.Registry64, "FFX-2.exe", "Debugger", RegistryValueKind.String, "new override");
        Reject(delegate { LaunchRedirectRepair.Repair(game, preview, Path.Combine(folder, "new.xml"), store); }, assert, "changed");
        assert(store.Deletes == 0, "New override requires renewed preview");
        store = Fixture(game); preview = LaunchRedirectRepair.Read(game, store); preview.Add(preview[0]);
        Reject(delegate { LaunchRedirectRepair.Repair(game, preview, Path.Combine(folder, "duplicate.xml"), store); }, assert, "Duplicate");
        assert(store.Deletes == 0, "Duplicate confirmed entry rejected");
        store = Fixture(game); store.FailDelete = 2;
        Reject(delegate { LaunchRedirectRepair.Repair(game, Path.Combine(folder, "rollback.xml"), store); }, assert, "Deleted values were restored");
        assert(store.Values.Count == originalCount && store.Get(RegistryView.Registry32, "FFX.exe", "Debugger") != null, "Partial deletion rolls back prior entries");
        store = Fixture(game); store.FailDelete = 2; store.FailSet = 1;
        Reject(delegate { LaunchRedirectRepair.Repair(game, Path.Combine(folder, "partial.xml"), store); }, assert, "Some values could not be restored: FFX.exe");
        assert(File.Exists(Path.Combine(folder, "partial.xml")), "Partial rollback retains complete recovery backup");
        store = Fixture(game);
        Reject(delegate { LaunchRedirectRepair.Repair(game, backup, store); }, assert, "already exists");
        assert(store.Deletes == 0, "Backup overwrite refused before deletion");
        Reject(delegate { LaunchRedirectRepair.Repair(game, Path.Combine(folder, "missing", "backup.xml"), store); }, assert, "backup folder");
        assert(store.Deletes == 0, "Backup path failure makes no registry mutation");
        store.Put(RegistryView.Registry32, "FFX.exe", "UseFilter", RegistryValueKind.DWord, 0);
        assert(LaunchRedirectRepair.Read(game, store).Count == 2, "Disabled filter excluded");
        string bad = Path.Combine(folder, "corrupt.xml"); File.WriteAllText(bad, "<LaunchRedirectBackup version=\"1\"><Entry/></LaunchRedirectBackup>");
        Reject(delegate { LaunchRedirectRepair.Restore(bad, new MemoryStore()); }, assert, "Invalid launch-redirection backup");
        File.WriteAllText(bad, File.ReadAllText(backup).Replace("FFX.exe", "Unrelated.exe"));
        Reject(delegate { LaunchRedirectRepair.Restore(bad, new MemoryStore()); }, assert, "unrelated registry key");
        File.WriteAllText(bad, File.ReadAllText(backup).Replace("ZgBpAGwAdABlAHIAZQBkAC4AZQB4AGUA", "ZgBpAGwAdABlAHIAZQBkAC4AZQB4AGUB"));
        Reject(delegate { LaunchRedirectRepair.ReadBackup(bad); }, assert, "checksum failed");
        assert(LaunchRedirectRepair.ReadBackup(backup).Count == 3, "Read-only backup preview");
        var blank = new MemoryStore(); blank.Put(RegistryView.Registry32, "FFX.exe", "Debugger", RegistryValueKind.String, "");
        string blankBackup = Path.Combine(folder, "blank.xml");
        assert(LaunchRedirectRepair.Repair(game, blankBackup, blank) == 1 && LaunchRedirectRepair.Restore(blankBackup, blank) == 1, "Empty Debugger value round trips");
        var raw = new MemoryStore(); string unusual = new string(new[] { '\ud800', '\0', '\udfff' });
        raw.Put(RegistryView.Registry64, "FFX.exe", "Debugger", RegistryValueKind.String, unusual);
        string rawBackup = Path.Combine(folder, "raw.xml");
        LaunchRedirectRepair.Repair(game, rawBackup, raw); LaunchRedirectRepair.Restore(rawBackup, raw);
        assert((string)raw.Get(RegistryView.Registry64, "FFX.exe", "Debugger").Data == unusual, "Exact UTF-16 including unusual units preserved");
        var restoreFailure = Fixture(game);
        LaunchRedirectRepair.Repair(game, Path.Combine(folder, "restore-failure-source.xml"), restoreFailure);
        int preserved = restoreFailure.Values.Count; restoreFailure.FailSet = 2;
        Reject(delegate { LaunchRedirectRepair.Restore(backup, restoreFailure); }, assert, "Newly restored values were removed");
        assert(restoreFailure.Values.Count == preserved, "Failed restore rolls back completed writes");
        var changedBackupStore = Fixture(game); string changedPath = Path.Combine(folder, "confirmed-restore.xml");
        LaunchRedirectRepair.Repair(game, changedPath, changedBackupStore);
        var capturedBackup = LaunchRedirectRepair.ReadBackup(changedPath);
        var newer = Fixture(game); newer.Put(RegistryView.Registry32, "FFX.exe", "Debugger", RegistryValueKind.String, "new valid backup data");
        string newerPath = Path.Combine(folder, "newer-backup.xml"); LaunchRedirectRepair.Repair(game, newerPath, newer);
        File.Copy(newerPath, changedPath, true);
        Reject(delegate { LaunchRedirectRepair.Restore(changedPath, capturedBackup, changedBackupStore); }, assert, "changed");
        assert(changedBackupStore.Sets == 0, "Changed backup after preview refuses all writes");
        var changedFilter = Fixture(game); string filterBackup = Path.Combine(folder, "filter-restore.xml");
        LaunchRedirectRepair.Repair(game, filterBackup, changedFilter);
        changedFilter.Put(RegistryView.Registry32, "FFX.exe\\target", "FilterFullPath", RegistryValueKind.String, Path.Combine(game, "other", "FFX.exe"));
        Reject(delegate { LaunchRedirectRepair.Restore(filterBackup, changedFilter); }, assert, "filter changed");
        assert(changedFilter.Sets == 0, "Changed filter association refuses all restore writes");
        var disabledFilter = Fixture(game); string disabledBackup = Path.Combine(folder, "disabled-filter.xml");
        LaunchRedirectRepair.Repair(game, disabledBackup, disabledFilter);
        disabledFilter.Put(RegistryView.Registry32, "FFX.exe", "UseFilter", RegistryValueKind.DWord, 0);
        Reject(delegate { LaunchRedirectRepair.Restore(disabledBackup, disabledFilter); }, assert, "filter changed");
        assert(disabledFilter.Sets == 0, "Disabled filter refuses all restore writes");
        var large = new MemoryStore(); large.Put(RegistryView.Registry32, "FFX.exe", "Debugger", RegistryValueKind.String, new string('x', 32769));
        Reject(delegate { LaunchRedirectRepair.Read(game, large); }, assert, "Too much");
        assert(large.Deletes == 0, "Oversized redirection refuses before mutation");
        store = Fixture(game); store.Put(RegistryView.Registry32, "FFX.exe", "Debugger", RegistryValueKind.Binary, new byte[] { 1 });
        Reject(delegate { LaunchRedirectRepair.Read(game, store); }, assert, "Unsupported Debugger");
    }
}
