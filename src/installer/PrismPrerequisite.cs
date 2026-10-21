using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

namespace FfxScreenReader.Setup
{
    // Read-only compatibility check for an existing or newly installed Prism runtime.
    internal static class PrismPrerequisite
    {
        private const string RuntimeDirectory = "PrismRuntime";
        private const string ReceiptName = "prism-install.json";
        private const string ErrorText = "Prism prerequisite missing or incompatible. Existing runtimes are not replaced; see the setup status for the failed check.";
        private static readonly Regex Sha256 = new Regex("^[0-9a-f]{64}$", RegexOptions.CultureInvariant);

        internal sealed class Package
        {
            public string Id, Version, Commit;
            public Dictionary<string, string> Files;
        }

        internal static Package ParseLock(byte[] sourceLock)
        {
            return ReadPackage(Decode(sourceLock), "source lock");
        }

        public static void Require(string game, byte[] sourceLock)
        {
            try { Verify(game, sourceLock); }
            catch (Exception ex)
            {
                if (ex is OutOfMemoryException || ex is StackOverflowException) throw;
                throw new InvalidOperationException(ErrorText + " " + ex.Message, ex);
            }
        }

        private static void Verify(string game, byte[] sourceLock)
        {
            if (sourceLock == null || sourceLock.Length == 0) throw new InvalidDataException("The embedded Prism source lock is empty.");
            Package expected = ReadPackage(Decode(sourceLock), "source lock");
            string root = InstallerPaths.Full(game);
            if ((File.GetAttributes(root) & FileAttributes.ReparsePoint) != 0) throw new IOException("Linked game directory is unsupported.");
            string runtime = Path.Combine(root, RuntimeDirectory);
            string receiptPath = Path.Combine(runtime, ReceiptName);
            InstallerPaths.CheckOrdinaryFile(root, RuntimeDirectory + "\\" + ReceiptName);
            if (!Directory.Exists(runtime)) throw new DirectoryNotFoundException("PrismRuntime is missing.");
            if (!File.Exists(receiptPath)) throw new FileNotFoundException("The Prism install receipt is missing.");

            Dictionary<string, object> receipt = Object(Decode(File.ReadAllBytes(receiptPath)), "Prism receipt");
            RequireNumber(receipt, "schema", 1);
            RequireString(receipt, "platform", "windows");
            RequireString(receipt, "architecture", "x86");
            RequireString(receipt, "game_directory", root);
            RequireString(receipt, "package_id", expected.Id);
            RequireString(receipt, "version", expected.Version);
            RequireString(receipt, "source_commit", expected.Commit);
            RequireOwners(receipt);
            Dictionary<string, string> installed = ReadInstalledFiles(Get(receipt, "files"));
            EqualManifest(expected.Files, installed, "receipt");

            var actualNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string path in Directory.GetFileSystemEntries(runtime)) {
                string name = Path.GetFileName(path);
                if (String.Equals(name, ReceiptName, StringComparison.OrdinalIgnoreCase)) continue;
                if (Directory.Exists(path)) throw new InvalidDataException("Unexpected Prism runtime subdirectory.");
                if (!actualNames.Add(name)) throw new InvalidDataException("Duplicate runtime filename.");
            }
            if (!actualNames.SetEquals(expected.Files.Keys)) throw new InvalidDataException("Runtime file manifest does not match the source lock.");

            foreach (KeyValuePair<string, string> item in expected.Files)
            {
                string relative = RuntimeDirectory + "\\" + item.Key;
                InstallerPaths.CheckOrdinaryFile(root, relative);
                string path = InstallerPaths.InGame(root, relative);
                if (!File.Exists(path)) throw new FileNotFoundException("A locked Prism runtime file is missing: " + item.Key);
                string hash = InstallerEngine.Hash(File.ReadAllBytes(path));
                if (!String.Equals(hash, item.Value, StringComparison.Ordinal))
                    throw new InvalidDataException("Prism runtime checksum failed: " + item.Key);
            }
        }

        private static Package ReadPackage(object value, string label)
        {
            Dictionary<string, object> data = Object(value, label);
            RequireNumber(data, "schema", 1);
            RequireString(data, "platform", "windows");
            RequireString(data, "architecture", "x86");
            var result = new Package {
                Id = Nonempty(data, "package_id"), Version = Nonempty(data, "version"),
                Commit = Nonempty(data, "source_commit"), Files = ReadFiles(Get(data, "files"), label + " files")
            };
            if (result.Files.Count == 0) throw new InvalidDataException(label + " has no files.");
            return result;
        }

        private static Dictionary<string, string> ReadFiles(object value, string label)
        {
            Dictionary<string, object> entries = Object(value, label);
            var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (KeyValuePair<string, object> entry in entries)
            {
                string name = FlatName(entry.Key);
                Dictionary<string, object> descriptor = Object(entry.Value, label + " entry " + name);
                RequireString(descriptor, "source", name);
                string hash = Nonempty(descriptor, "sha256");
                if (!Sha256.IsMatch(hash)) throw new InvalidDataException("Invalid SHA-256 for " + name + ".");
                if (result.ContainsKey(name)) throw new InvalidDataException("Duplicate filename in " + label + ": " + name);
                result.Add(name, hash);
            }
            return result;
        }

        private static Dictionary<string, string> ReadInstalledFiles(object value)
        {
            Dictionary<string, object> entries = Object(value, "receipt files");
            var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (KeyValuePair<string, object> entry in entries)
            {
                string name = FlatName(entry.Key);
                string hash = entry.Value as string;
                if (hash == null || !Sha256.IsMatch(hash)) throw new InvalidDataException("Invalid receipt checksum for " + name + ".");
                if (result.ContainsKey(name)) throw new InvalidDataException("Duplicate receipt filename: " + name);
                result.Add(name, hash);
            }
            return result;
        }

        private static string FlatName(string name)
        {
            if (String.IsNullOrWhiteSpace(name) || name == "." || name == ".." || name.IndexOfAny(new[] { '/', '\\', ':' }) >= 0 ||
                name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
                throw new InvalidDataException("Prism manifests must use flat filenames: " + name);
            return name;
        }

        private static void RequireOwners(Dictionary<string, object> receipt)
        {
            object raw = Get(receipt, "owners");
            IEnumerable owners = raw as IEnumerable;
            if (owners == null || raw is string) throw new InvalidDataException("Receipt owners must be an array.");
            var found = new HashSet<string>(StringComparer.Ordinal);
            foreach (object owner in owners)
            {
                string text = owner as string;
                if (String.IsNullOrWhiteSpace(text)) throw new InvalidDataException("Receipt contains an invalid owner.");
                found.Add(text);
            }
            if (!found.Contains("FFX-native") || !found.Contains("FFX-launcher"))
                throw new InvalidDataException("Receipt does not identify both FFX owners.");
        }

        private static void EqualManifest(Dictionary<string, string> expected, Dictionary<string, string> actual, string label)
        {
            if (expected.Count != actual.Count) throw new InvalidDataException(label + " file manifest does not match the source lock.");
            foreach (KeyValuePair<string, string> item in expected)
            {
                string hash;
                if (!actual.TryGetValue(item.Key, out hash) || !String.Equals(hash, item.Value, StringComparison.Ordinal))
                    throw new InvalidDataException(label + " file manifest does not match the source lock: " + item.Key);
            }
        }

        private static object Decode(byte[] bytes)
        {
            string json = System.Text.Encoding.UTF8.GetString(bytes).TrimStart('\uFEFF');
            return new JavaScriptSerializer().DeserializeObject(json);
        }
        private static Dictionary<string, object> Object(object value, string label)
        {
            Dictionary<string, object> data = value as Dictionary<string, object>;
            if (data == null) throw new InvalidDataException(label + " must be a JSON object.");
            return data;
        }
        private static object Get(Dictionary<string, object> data, string key)
        {
            object value;
            if (!data.TryGetValue(key, out value)) throw new InvalidDataException("Missing required field: " + key + ".");
            return value;
        }
        private static string Nonempty(Dictionary<string, object> data, string key)
        {
            string value = Get(data, key) as string;
            if (String.IsNullOrWhiteSpace(value)) throw new InvalidDataException("Invalid required field: " + key + ".");
            return value;
        }
        private static void RequireString(Dictionary<string, object> data, string key, string expected)
        {
            string actual = Get(data, key) as string;
            if (!String.Equals(actual, expected, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Invalid " + key + " value.");
        }
        private static void RequireNumber(Dictionary<string, object> data, string key, int expected)
        {
            object raw = Get(data, key);
            if (!(raw is int) || (int)raw != expected) throw new InvalidDataException("Unsupported " + key + " value.");
        }
    }
}
