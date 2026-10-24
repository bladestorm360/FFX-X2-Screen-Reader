using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Windows.Forms;

// Exercise the actual process-wide loader policy, including a baseline child
// without initialization. The fixture is a copy of Windows' inert version
// resource library, not a game DLL or a generated malicious sample.
internal static class InstallerSecurityTests
{
    private const string FixtureName = "FfxInstallerSearchFixture.dll";
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr LoadLibrary(string name);
    [DllImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool FreeLibrary(IntPtr module);

    internal static int CheckLoader(bool hardened)
    {
        IntPtr fixture = LoadLibrary(FixtureName);
        bool loaded = fixture != IntPtr.Zero;
        if (loaded) FreeLibrary(fixture);
        if (loaded == hardened) return 1;
        IntPtr platform = LoadLibrary("version.dll");
        if (platform == IntPtr.Zero) return 2;
        FreeLibrary(platform);
        // The same restricted process must retain ordinary WinForms controls.
        using (var control = new TextBox()) { if (control.Handle == IntPtr.Zero) return 3; }
        return 0;
    }

    internal static int CheckEmbedded(string executable)
    {
        // Use the shipped assembly's initializer, before the source-test host
        // initializes any policy, so a stale installer cannot inherit a pass.
        Assembly setup = Assembly.LoadFile(Path.GetFullPath(executable));
        setup.GetType("FfxScreenReader.Setup.InstallerRuntime", true).GetMethod("Initialize").Invoke(null, null);
        return CheckLoader(true);
    }

    internal static void RunEmbedded(string setup)
    {
        // Keep this child-process fixture beside the owned test host. Game
        // fixtures can deliberately exceed MAX_PATH; ProcessStartInfo and this
        // loader-specific check must not impose that separate test dependency.
        string folder = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "embedded-loader-policy-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(folder);
        string executable = Path.Combine(folder, "InstallerSearchTests.exe");
        File.Copy(Assembly.GetExecutingAssembly().Location, executable);
        File.Copy(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "version.dll"), Path.Combine(folder, FixtureName));
        var start = new ProcessStartInfo(executable, "--loader-embedded \"" + Path.GetFullPath(setup) + "\"") {
            WorkingDirectory = folder, UseShellExecute = false, CreateNoWindow = true
        };
        start.EnvironmentVariables["PATH"] = folder + ";" + start.EnvironmentVariables["PATH"];
        using (var child = Process.Start(start))
        {
            if (!child.WaitForExit(15000)) { child.Kill(); throw new Exception("Embedded installer loader fixture timed out."); }
            if (child.ExitCode != 0) throw new Exception("The packaged installer did not enforce its native DLL search restriction.");
        }
    }

    internal static void Run(string root, Action<bool, string> assert)
    {
        string folder = Path.Combine(root, "loader-policy");
        Directory.CreateDirectory(folder);
        string executable = Path.Combine(folder, "InstallerSearchTests.exe");
        File.Copy(Assembly.GetExecutingAssembly().Location, executable);
        File.Copy(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "version.dll"), Path.Combine(folder, FixtureName));
        foreach (bool hardened in new[] { false, true })
        {
            var start = new ProcessStartInfo(executable, hardened ? "--loader-hardened" : "--loader-baseline") {
                WorkingDirectory = folder, UseShellExecute = false, CreateNoWindow = true
            };
            start.EnvironmentVariables["PATH"] = folder + ";" + start.EnvironmentVariables["PATH"];
            using (var child = Process.Start(start))
            {
                if (!child.WaitForExit(15000)) { child.Kill(); throw new Exception("Installer loader fixture timed out."); }
                assert(child.ExitCode == 0, hardened ? "Secured setup refuses application/current/PATH DLL and retains system DLL/WinForms loading" : "Baseline reproduces application/current/PATH DLL loading");
            }
        }
    }
}
