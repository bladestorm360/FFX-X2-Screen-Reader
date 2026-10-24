using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;

// Modern System.IO adds the extended native prefix when it is needed. This
// removes the legacy MAX_PATH check without changing machine registry policy.
[assembly: TargetFramework(".NETFramework,Version=v4.6.2", FrameworkDisplayName = ".NET Framework 4.6.2")]
namespace FfxScreenReader.Setup
{
    internal static class InstallerRuntime
    {
        // Setup copies its embedded mod/runtime DLLs as data; it never loads
        // them. Resolve its own native platform dependencies only from Windows,
        // excluding the download/application directory, current directory and
        // PATH. This is process-local and changes no Windows security policy.
        private const uint LoadLibrarySearchSystem32 = 0x00000800;

        [DllImport("kernel32.dll", SetLastError = true)]
        [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetDefaultDllDirectories(uint directoryFlags);

        public static void Initialize()
        {
            if (!SetDefaultDllDirectories(LoadLibrarySearchSystem32))
                throw new Win32Exception(Marshal.GetLastWin32Error(), "Setup could not secure its Windows DLL search path.");
            // Run before WinForms/discovery can cache the IO compatibility flags.
            AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling", false);
            AppContext.SetSwitch("Switch.System.IO.BlockLongPaths", false);
        }
    }
}
