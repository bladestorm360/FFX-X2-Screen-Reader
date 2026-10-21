using System;
using System.IO;
using System.Collections.Generic;

namespace FfxScreenReader.Setup
{
    // Ordinary paths stay in UI/comparisons. Only IO receives an extended path.
    internal static class InstallerFiles
    {
        public static bool Exists(string path) {
            try { return File.Exists(InstallerPaths.Native(path)); }
            catch (ArgumentException) { return false; }
            catch (NotSupportedException) { return false; }
            catch (PathTooLongException) { return false; }
        }
        public static FileAttributes GetAttributes(string path) { return File.GetAttributes(InstallerPaths.Native(path)); }
        public static void SetAttributes(string path, FileAttributes value) { File.SetAttributes(InstallerPaths.Native(path), value); }
        public static byte[] ReadAllBytes(string path) { return File.ReadAllBytes(InstallerPaths.Native(path)); }
        public static string ReadAllText(string path) { return File.ReadAllText(InstallerPaths.Native(path)); }
        public static FileStream OpenRead(string path) { return File.OpenRead(InstallerPaths.Native(path)); }
        public static long Length(string path) { return new FileInfo(InstallerPaths.Native(path)).Length; }
        public static void WriteAllBytes(string path, byte[] value) { File.WriteAllBytes(InstallerPaths.Native(path), value); }
        public static void WriteAllText(string path, string value) { File.WriteAllText(InstallerPaths.Native(path), value); }
        public static void WriteAllLines(string path, IEnumerable<string> value) { File.WriteAllLines(InstallerPaths.Native(path), value); }
        public static void Copy(string source, string target) { Copy(source, target, false); }
        public static void Copy(string source, string target, bool overwrite) { File.Copy(InstallerPaths.Native(source), InstallerPaths.Native(target), overwrite); }
        public static void Delete(string path) { File.Delete(InstallerPaths.Native(path)); }
    }
    internal static class InstallerDirectories
    {
        public static bool Exists(string path) {
            try { return Directory.Exists(InstallerPaths.Native(path)); }
            catch (ArgumentException) { return false; }
            catch (NotSupportedException) { return false; }
            catch (PathTooLongException) { return false; }
        }
        public static DirectoryInfo CreateDirectory(string path) { return Directory.CreateDirectory(InstallerPaths.Native(path)); }
        public static string[] GetFileSystemEntries(string path) { return Display(Directory.GetFileSystemEntries(InstallerPaths.Native(path))); }
        public static string[] GetDirectories(string path) { return Display(Directory.GetDirectories(InstallerPaths.Native(path))); }
        public static string[] GetFiles(string path) { return Display(Directory.GetFiles(InstallerPaths.Native(path))); }
        public static string[] GetFiles(string path, string pattern, SearchOption option) { return Display(Directory.GetFiles(InstallerPaths.Native(path), pattern, option)); }
        public static void Delete(string path) { Directory.Delete(InstallerPaths.Native(path)); }
        public static void Move(string source, string target) {
            Directory.Move(InstallerPaths.Native(source), InstallerPaths.Native(target));
        }
        private static string[] Display(string[] paths) {
            for (int i=0;i<paths.Length;++i) paths[i]=InstallerPaths.Display(paths[i]);
            return paths;
        }
    }
}
