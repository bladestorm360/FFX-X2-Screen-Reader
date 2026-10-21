using System;
using System.Collections.Generic;

namespace FfxScreenReader.Setup
{
    internal static class LaunchRedirects
    {
        internal const string KeyRoot = @"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options";
        public static string EntriesText(IList<LaunchRedirectRepair.Entry> entries) {
            var lines = new List<string>();
            foreach (var entry in entries) lines.Add(entry.View + ": HKEY_LOCAL_MACHINE\\" + KeyRoot + "\\" + entry.RelativeKey +
                "\r\nValue: Debugger (" + entry.Kind + ")\r\nData: " + entry.Data +
                (entry.FilterFullPath == null ? "" : "\r\nOnly this executable: " + entry.FilterFullPath));
            return String.Join("\r\n\r\n", lines.ToArray());
        }
        public static string Describe(string game) {
            if (String.IsNullOrWhiteSpace(game)) return "Select a game installation first, then choose Check launch redirection. No registry values were changed.";
            var entries = LaunchRedirectRepair.Read(game);
            return entries.Count == 0 ? "No FFX Debugger launch redirection entries were found. No registry values were changed. If Steam still reports a missing executable, check its Launch Options and report the exact missing path." :
                "FFX launch redirection entries were found. These may start a custom launcher instead of the original executable. No registry values were changed. Choose Repair launch redirection to review, back up, and remove them. Ctrl+C copies this status text:\r\n\r\n" + EntriesText(entries);
        }
        public static void RequireNone(string game) {
            var entries = LaunchRedirectRepair.Read(game);
            if (entries.Count != 0) throw new InvalidOperationException("FFX has a Windows Debugger launch redirection entry. Choose Repair launch redirection to review and back up the registry values before removal, then choose Install again. No registry values or game files were changed.\r\n" + EntriesText(entries));
        }
    }
}
