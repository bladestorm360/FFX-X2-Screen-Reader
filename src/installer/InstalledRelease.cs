using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using File = FfxScreenReader.Setup.InstallerFiles;

namespace FfxScreenReader.Setup
{
    internal static class InstalledRelease
    {
        public const string FileName = "FFX_SR_InstalledRelease.txt";
        public const string PendingName = FileName + ".pending";
        private const string ProxyName = "dinput8.dll";

        public static bool RequiresDownload(string game, Version latest)
        {
            Version installed = FindVersion(game);
            if (installed != null && installed > latest)
                throw new InvalidOperationException("This folder already has screen reader version " + installed +
                    ". GitHub currently offers version " + latest + ". No game files were changed.");
            return installed == null || installed < latest;
        }

        private static Version FindVersion(string game)
        {
            InstallerPaths.CheckOrdinaryFile(game, ProxyName);
            string proxy = InstallerPaths.InGame(game, ProxyName);
            if (!File.Exists(proxy)) return null;

            // Older releases and developer deployments already identify their
            // installed version in this proxy's Windows version resource.
            FileVersionInfo details = FileVersionInfo.GetVersionInfo(InstallerPaths.Native(proxy));
            if (details.CompanyName == "bladestorm360" && details.InternalName == "FFXScreenReader" &&
                details.ProductName == "FFX / FFX-2 Screen Reader")
                return new Version(details.FileMajorPart, details.FileMinorPart, details.FileBuildPart, details.FilePrivatePart);

            // The completion receipt also ties a version to the exact proxy
            // bytes when a package has no Windows version resource.
            InstallerPaths.CheckOrdinaryFile(game, FileName);
            string path = InstallerPaths.InGame(game, FileName);
            if (!File.Exists(path)) return null;
            string[] fields;
            using (var receipt = new FileStream(InstallerPaths.Native(path), FileMode.Open, FileAccess.Read, FileShare.Read | FileShare.Delete))
            using (var reader = new BinaryReader(receipt))
            {
                byte[] bytes = reader.ReadBytes(257);
                if (bytes.Length > 256) return null;
                fields = Encoding.UTF8.GetString(bytes).Split('\n');
            }

            Version version;
            if (fields.Length != 2 || fields[1].Length != 64 || !Version.TryParse(fields[0], out version) || version.Revision < 0)
                return null;
            using (Stream file = File.OpenRead(proxy))
            {
                if (file.Length > 64 * 1024 * 1024 || InstallerEngine.Hash(file) != fields[1]) return null;
            }
            return version;
        }

        public static void Write(string game, Version version, string proxyHash)
        {
            InstallerPaths.CheckOrdinaryFile(game, FileName);
            InstallerPaths.CheckOrdinaryFile(game, PendingName);
            string path = InstallerPaths.InGame(game, FileName);
            string pending = InstallerPaths.Native(InstallerPaths.InGame(game, PendingName));
            byte[] bytes = Encoding.UTF8.GetBytes(version + "\n" + proxyHash.ToLowerInvariant());
            using (var file = new FileStream(pending, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                file.Write(bytes, 0, bytes.Length);
                file.Flush(true);
            }
            if (File.Exists(path)) System.IO.File.Replace(pending, InstallerPaths.Native(path), null);
            else System.IO.File.Move(pending, InstallerPaths.Native(path));
        }
    }
}
