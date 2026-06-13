using System.Diagnostics;

namespace Asio2Wasapi.ControlPanel;

internal sealed class AudioDeviceInfo
{
    public required string Name { get; init; }
    public int Channels { get; init; }
}

internal sealed class AudioDeviceList
{
    public List<AudioDeviceInfo> AsioInputs { get; } = new();
    public List<AudioDeviceInfo> WasapiOutputs { get; } = new();
}

internal static class DeviceScanner
{
    public static AudioDeviceList Scan(DriverPaths paths)
    {
        var result = new AudioDeviceList();

        var exePath = Path.Combine(paths.InstalledDriverDirectory, "asio2wasapi-devices.exe");

        if (!File.Exists(exePath))
            return result;

        try
        {
            using var process = Process.Start(new ProcessStartInfo
            {
                FileName = exePath,
                WorkingDirectory = paths.InstalledDriverDirectory,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            });

            if (process == null)
                return result;

            if (!process.WaitForExit(3000))
            {
                try
                {
                    process.Kill(true);
                }
                catch
                {
                }

                return result;
            }

            var output = process.StandardOutput.ReadToEnd();

            foreach (var rawLine in output.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries))
            {
                var parts = rawLine.Split('|');

                if (parts.Length < 3)
                    continue;

                var type = parts[0].Trim();
                var name = parts[1].Trim();

                _ = int.TryParse(parts[2], out var channels);

                if (type == "ASIO_INPUT")
                {
                    result.AsioInputs.Add(new AudioDeviceInfo
                    {
                        Name = name,
                        Channels = channels
                    });
                }

                if (type == "WASAPI_OUTPUT")
                {
                    result.WasapiOutputs.Add(new AudioDeviceInfo
                    {
                        Name = name,
                        Channels = channels
                    });
                }
            }
        }
        catch
        {
            return result;
        }

        return result;
    }
}