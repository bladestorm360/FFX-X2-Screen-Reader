using System;
using System.Collections.Generic;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using System.Text;

namespace FfxScreenReader.Setup
{
    // Only metadata beside the selected steamapps/common installation is consulted.
    internal static class SteamAppManifest
    {
        private const int MaximumBytes = 1048576;
        private const int MaximumDepth = 32;
        private const int MaximumNodes = 16384;
        private sealed class Entry
        {
            public string Value;
            public Dictionary<string, Entry> Children;
        }

        private sealed class Parser
        {
            private readonly string text;
            private int position;
            private int nodes;
            public Parser(string text) { this.text = text; }
            private void Skip()
            {
                while (position < text.Length)
                {
                    if (Char.IsWhiteSpace(text[position])) { position++; continue; }
                    if (text[position] == '/' && position + 1 < text.Length && text[position + 1] == '/')
                    {
                        position += 2;
                        while (position < text.Length && text[position] != '\n') position++;
                        continue;
                    }
                    break;
                }
            }
            private string String()
            {
                Skip();
                if (position >= text.Length || text[position++] != '"') throw Error("Expected a quoted metadata string.");
                var value = new StringBuilder();
                while (position < text.Length)
                {
                    char c = text[position++];
                    if (c == '"') return value.ToString();
                    if (c == '\\')
                    {
                        if (position >= text.Length) throw Error("Truncated metadata escape.");
                        c = text[position++];
                        if (c != '\\' && c != '"' && c != '/') throw Error("Unsupported metadata escape.");
                    }
                    if (Char.IsControl(c)) throw Error("Unexpected control character in metadata.");
                    value.Append(c);
                }
                throw Error("Truncated metadata string.");
            }
            public Dictionary<string, Entry> Read(int depth, bool nested)
            {
                if (depth > MaximumDepth) throw Error("Steam metadata nesting is too deep.");
                var entries = new Dictionary<string, Entry>(StringComparer.OrdinalIgnoreCase);
                while (true)
                {
                    Skip();
                    if (position == text.Length)
                    {
                        if (nested) throw Error("Truncated metadata section.");
                        return entries;
                    }
                    if (text[position] == '}')
                    {
                        if (!nested) throw Error("Unexpected closing metadata brace.");
                        position++;
                        return entries;
                    }
                    string key = String();
                    if (++nodes > MaximumNodes) throw Error("Steam metadata has too many entries.");
                    if (entries.ContainsKey(key)) throw Error("Duplicate metadata key: " + key);
                    Skip();
                    var entry = new Entry();
                    if (position < text.Length && text[position] == '{')
                    {
                        position++;
                        entry.Children = Read(depth + 1, true);
                    }
                    else entry.Value = String();
                    entries.Add(key, entry);
                }
            }
        }

        private static InvalidOperationException Error(string detail)
        {
            return new InvalidOperationException("Steam installation metadata could not be verified. " + detail +
                " Use Steam to verify the game files, then run setup again. No game files were changed.");
        }
        private static string Normalize(string path)
        {
            string full = InstallerPaths.Full(path);
            return full.Length == Path.GetPathRoot(full).Length ? full : full.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        }
        private static string ManifestPath(string game)
        {
            string full = Normalize(game);
            string common = Path.GetDirectoryName(full);
            if (common == null || !String.Equals(Path.GetFileName(common), "common", StringComparison.OrdinalIgnoreCase)) return null;
            string apps = Path.GetDirectoryName(common);
            if (apps == null || !String.Equals(Path.GetFileName(apps), "steamapps", StringComparison.OrdinalIgnoreCase)) return null;
            return Path.Combine(apps, "appmanifest_359870.acf");
        }
        private static Dictionary<string, Entry> Section(Dictionary<string, Entry> entries, string name)
        {
            Entry entry;
            if (!entries.TryGetValue(name, out entry) || entry.Children == null) throw Error("Missing metadata section: " + name + ".");
            return entry.Children;
        }
        private static string Value(Dictionary<string, Entry> entries, string name)
        {
            Entry entry;
            if (!entries.TryGetValue(name, out entry) || entry.Value == null) throw Error("Missing metadata value: " + name + ".");
            return entry.Value;
        }
        private static bool Numeric(string value)
        {
            if (String.IsNullOrEmpty(value)) return false;
            foreach (char c in value) if (c < '0' || c > '9') return false;
            ulong number;
            return UInt64.TryParse(value, out number) && number != 0;
        }
        private static Dictionary<string, Entry> ReadApp(string path)
        {
            try
            {
                using (var stream = new FileStream(InstallerPaths.Native(path), FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    if (stream.Length > MaximumBytes) throw Error("Steam metadata exceeds the size limit.");
                    byte[] bytes = new byte[(int)stream.Length];
                    int count = 0;
                    while (count < bytes.Length)
                    {
                        int read = stream.Read(bytes, count, bytes.Length - count);
                        if (read == 0) throw Error("Steam metadata changed while it was read.");
                        count += read;
                    }
                    if (stream.ReadByte() != -1) throw Error("Steam metadata changed while it was read.");
                    string text = new UTF8Encoding(false, true).GetString(bytes).TrimStart('﻿');
                    var root = new Parser(text).Read(0, false);
                    var app = Section(root, "AppState");
                    Entry id;
                    if (app.TryGetValue("appid", out id) && id.Value != "359870") throw Error("The app ID is not 359870.");
                    return app;
                }
            }
            catch (InvalidOperationException) { throw; }
            catch (IOException ex) { throw Error("Could not read the app manifest: " + ex.Message); }
            catch (UnauthorizedAccessException ex) { throw Error("Could not read the app manifest: " + ex.Message); }
            catch (DecoderFallbackException ex) { throw Error("Could not read the app manifest: " + ex.Message); }
        }
        private static bool SameLocation(string game, string manifest, Dictionary<string, Entry> app)
        {
            string directory = Value(app, "installdir");
            if (directory == "." || directory == ".." || String.IsNullOrWhiteSpace(directory) ||
                directory.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || directory.IndexOf('/') >= 0 || directory.IndexOf('\\') >= 0)
                throw Error("The installation directory is invalid.");
            string installed = Normalize(Path.Combine(Path.GetDirectoryName(manifest), "common", directory));
            return String.Equals(installed, Normalize(game), StringComparison.OrdinalIgnoreCase);
        }
        public static bool MatchesGameLocation(string game)
        {
            string path = ManifestPath(game);
            return path != null && File.Exists(path) && SameLocation(game, path, ReadApp(path));
        }
        public static string GetInstalledManifest(string game)
        {
            string path = ManifestPath(game);
            if (path == null || !File.Exists(path)) return null;
            var app = ReadApp(path);
            if (!SameLocation(game, path, app)) throw Error("The app manifest refers to a different game folder.");
            var depots = Section(app, "InstalledDepots");
            if (depots.Count != 1 || !depots.ContainsKey("359871"))
                throw Error("The installed depots do not match the supported complete game depot 359871.");
            var depot = Section(depots, "359871");
            string manifest = Value(depot, "manifest");
            if (!Numeric(manifest)) throw Error("The installed depot manifest ID is invalid.");
            return manifest;
        }
    }
}
