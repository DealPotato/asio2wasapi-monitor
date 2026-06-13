namespace Asio2Wasapi.ControlPanel;

internal sealed class DriverPaths
{
    public required string InstalledDriverDirectory { get; init; }
    public required string DriverDllPath { get; init; }
    public required string ConfigPath { get; init; }

    public static DriverPaths Resolve()
    {
        var baseDirectory = AppContext.BaseDirectory;
        var deployedDriverDll = Path.Combine(baseDirectory, "asio2wasapi-virtual-asio.dll");

        if (File.Exists(deployedDriverDll))
        {
            return FromInstalledDirectory(baseDirectory);
        }

        var current = new DirectoryInfo(baseDirectory);

        while (current != null)
        {
            var cmakeLists = Path.Combine(current.FullName, "CMakeLists.txt");
            var installedDriver = Path.Combine(current.FullName, "installed-driver");

            if (File.Exists(cmakeLists))
            {
                Directory.CreateDirectory(installedDriver);
                return FromInstalledDirectory(installedDriver);
            }

            current = current.Parent;
        }

        var fallback = Path.Combine(baseDirectory, "installed-driver");
        Directory.CreateDirectory(fallback);

        return FromInstalledDirectory(fallback);
    }

    private static DriverPaths FromInstalledDirectory(string directory)
    {
        directory = Path.GetFullPath(directory);

        return new DriverPaths
        {
            InstalledDriverDirectory = directory,
            DriverDllPath = Path.Combine(directory, "asio2wasapi-virtual-asio.dll"),
            ConfigPath = Path.Combine(directory, "asio2wasapi-monitor.ini")
        };
    }
}