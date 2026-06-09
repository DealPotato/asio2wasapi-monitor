param(
    [string]$DllPath = "$PSScriptRoot\..\build\Release\asio2wasapi-virtual-asio.dll"
)

$driverName = "ASIO2WASAPI Virtual ASIO"
$clsid = "{85B9BDB2-2F44-4D13-9C7A-2F2863A0D1D0}"

if (-not (Test-Path $DllPath)) {
    Write-Error "DLL not found: $DllPath"
    exit 1
}

$resolvedDllPath = (Resolve-Path $DllPath).Path

Write-Host "Registering $driverName"
Write-Host "DLL: $resolvedDllPath"
Write-Host "CLSID: $clsid"

# Force 64-bit registry view for x64 ASIO hosts.
$localMachine64 = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
    [Microsoft.Win32.RegistryHive]::LocalMachine,
    [Microsoft.Win32.RegistryView]::Registry64
)

$classesRoot64 = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
    [Microsoft.Win32.RegistryHive]::ClassesRoot,
    [Microsoft.Win32.RegistryView]::Registry64
)

$asioKey = $localMachine64.CreateSubKey("SOFTWARE\ASIO\$driverName")
$asioKey.SetValue("CLSID", $clsid, [Microsoft.Win32.RegistryValueKind]::String)
$asioKey.SetValue("Description", $driverName, [Microsoft.Win32.RegistryValueKind]::String)
$asioKey.Close()

$clsidKey = $classesRoot64.CreateSubKey("CLSID\$clsid")
$clsidKey.SetValue("", $driverName, [Microsoft.Win32.RegistryValueKind]::String)
$clsidKey.Close()

$inprocKey = $classesRoot64.CreateSubKey("CLSID\$clsid\InprocServer32")
$inprocKey.SetValue("", $resolvedDllPath, [Microsoft.Win32.RegistryValueKind]::String)
$inprocKey.SetValue("ThreadingModel", "Both", [Microsoft.Win32.RegistryValueKind]::String)
$inprocKey.Close()

Write-Host "Registered successfully."