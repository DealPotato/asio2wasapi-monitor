using Microsoft.Win32;
using System.Diagnostics;
using System.Security.Principal;

namespace Asio2Wasapi.ControlPanel;

internal static class DriverInstaller
{
    private const string DriverName = "ASIO2WASAPI Virtual ASIO";
    private const string Clsid = "{85B9BDB2-2F44-4D13-9C7A-2F2863A0D1D0}";
    private const string AsioRegistryPath = @"SOFTWARE\ASIO\ASIO2WASAPI Virtual ASIO";
    private const string ClsidRegistryPath = @"CLSID\" + Clsid;

    public static bool IsInstalled(string expectedDllPath)
    {
        try
        {
            using var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64);
            using var asioKey = hklm.OpenSubKey(AsioRegistryPath);

            if (asioKey == null)
                return false;

            var clsid = asioKey.GetValue("CLSID") as string;

            if (!string.Equals(clsid, Clsid, StringComparison.OrdinalIgnoreCase))
                return false;

            using var hkcr = RegistryKey.OpenBaseKey(RegistryHive.ClassesRoot, RegistryView.Registry64);
            using var inproc = hkcr.OpenSubKey($@"{ClsidRegistryPath}\InprocServer32");

            if (inproc == null)
                return false;

            var registeredPath = inproc.GetValue("") as string;

            return string.Equals(
                Path.GetFullPath(registeredPath ?? ""),
                Path.GetFullPath(expectedDllPath),
                StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    public static void InstallWithElevation(string dllPath)
    {
        if (IsAdministrator())
        {
            Install(dllPath);
            return;
        }

        RunElevated("--install", dllPath);
    }

    public static void UninstallWithElevation()
    {
        if (IsAdministrator())
        {
            Uninstall();
            return;
        }

        RunElevated("--uninstall", "");
    }

    public static void Install(string dllPath)
    {
        if (!File.Exists(dllPath))
            throw new FileNotFoundException("Driver DLL was not found.", dllPath);

        dllPath = Path.GetFullPath(dllPath);

        using var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64);
        using var asioKey = hklm.CreateSubKey(AsioRegistryPath, true);

        if (asioKey == null)
            throw new InvalidOperationException("Could not create ASIO registry key.");

        asioKey.SetValue("CLSID", Clsid, RegistryValueKind.String);
        asioKey.SetValue("Description", DriverName, RegistryValueKind.String);

        using var hkcr = RegistryKey.OpenBaseKey(RegistryHive.ClassesRoot, RegistryView.Registry64);
        using var clsidKey = hkcr.CreateSubKey(ClsidRegistryPath, true);

        if (clsidKey == null)
            throw new InvalidOperationException("Could not create COM registry key.");

        clsidKey.SetValue("", DriverName, RegistryValueKind.String);

        using var inproc = clsidKey.CreateSubKey("InprocServer32", true);

        if (inproc == null)
            throw new InvalidOperationException("Could not create InprocServer32 registry key.");

        inproc.SetValue("", dllPath, RegistryValueKind.String);
        inproc.SetValue("ThreadingModel", "Both", RegistryValueKind.String);
    }

    public static void Uninstall()
    {
        using var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64);
        hklm.DeleteSubKeyTree(AsioRegistryPath, false);

        using var hkcr = RegistryKey.OpenBaseKey(RegistryHive.ClassesRoot, RegistryView.Registry64);
        hkcr.DeleteSubKeyTree(ClsidRegistryPath, false);
    }

    private static bool IsAdministrator()
    {
        using var identity = WindowsIdentity.GetCurrent();
        var principal = new WindowsPrincipal(identity);

        return principal.IsInRole(WindowsBuiltInRole.Administrator);
    }

    private static void RunElevated(string command, string argument)
    {
        var args = string.IsNullOrWhiteSpace(argument)
            ? command
            : $"{command} \"{argument}\"";

        var process = Process.Start(new ProcessStartInfo
        {
            FileName = Application.ExecutablePath,
            Arguments = args,
            Verb = "runas",
            UseShellExecute = true
        });

        process?.WaitForExit();
    }
}