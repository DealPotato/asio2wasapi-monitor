$driverName = "ASIO2WASAPI Virtual ASIO"
$clsid = "{85B9BDB2-2F44-4D13-9C7A-2F2863A0D1D0}"

Write-Host "Unregistering $driverName"
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

# Remove ASIO registry entry.
$localMachine64.DeleteSubKeyTree(
    "SOFTWARE\ASIO\$driverName",
    $false
)

# Remove COM CLSID registry entry.
$classesRoot64.DeleteSubKeyTree(
    "CLSID\$clsid",
    $false
)

Write-Host "Unregistered successfully."