using System;
using System.Runtime.Versioning;

// Modern System.IO adds the extended native prefix when it is needed. This
// removes the legacy MAX_PATH check without changing machine registry policy.
[assembly: TargetFramework(".NETFramework,Version=v4.6.2", FrameworkDisplayName = ".NET Framework 4.6.2")]
namespace FfxScreenReader.Setup
{
    internal static class InstallerRuntime
    {
        public static void Initialize()
        {
            // Run before WinForms/discovery can cache the IO compatibility flags.
            AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling", false);
            AppContext.SetSwitch("Switch.System.IO.BlockLongPaths", false);
        }
    }
}
