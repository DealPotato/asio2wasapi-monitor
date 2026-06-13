using System.Globalization;

namespace Asio2Wasapi.ControlPanel;

internal sealed class DriverConfig
{
    public int SampleRate { get; set; } = 48000;
    public int AsioBufferFrames { get; set; } = 128;
    public int WasapiBufferFrames { get; set; } = 256;
    public int InputRingFrames { get; set; } = 2048;
    public int OutputRingFrames { get; set; } = 2048;

    public string PreferredAsioInputDevice { get; set; } = "Focusrite";
    public int HardwareInputChannel { get; set; } = 1;
    public float InputGain { get; set; } = 1.0f;
    public bool EnableTestTone { get; set; } = false;

    public bool UseDefaultWasapiDevice { get; set; } = true;
    public string PreferredWasapiDevice { get; set; } = "";
    public float OutputGain { get; set; } = 1.0f;

    public bool EnableLogging { get; set; } = true;

    public static DriverConfig Load(string path)
    {
        var ini = IniFile.Load(path);
        var config = new DriverConfig();

        config.SampleRate = ini.GetInt("Audio", "sampleRate", config.SampleRate);
        config.AsioBufferFrames = ini.GetInt("Audio", "asioBufferFrames", config.AsioBufferFrames);
        config.WasapiBufferFrames = ini.GetInt("Audio", "wasapiBufferFrames", config.WasapiBufferFrames);
        config.InputRingFrames = ini.GetInt("Audio", "inputRingFrames", config.InputRingFrames);
        config.OutputRingFrames = ini.GetInt("Audio", "outputRingFrames", config.OutputRingFrames);

        config.PreferredAsioInputDevice = ini.GetString("Input", "preferredAsioInputDevice", config.PreferredAsioInputDevice);
        config.HardwareInputChannel = ini.GetInt("Input", "hardwareInputChannel", config.HardwareInputChannel);
        config.InputGain = ini.GetFloat("Input", "inputGain", config.InputGain);
        config.EnableTestTone = ini.GetBool("Input", "enableTestTone", config.EnableTestTone);

        config.UseDefaultWasapiDevice = ini.GetBool("Output", "useDefaultWasapiDevice", config.UseDefaultWasapiDevice);
        config.PreferredWasapiDevice = ini.GetString("Output", "preferredWasapiDevice", config.PreferredWasapiDevice);
        config.OutputGain = ini.GetFloat("Output", "outputGain", config.OutputGain);

        config.EnableLogging = ini.GetBool("Debug", "enableLogging", config.EnableLogging);

        return config;
    }

    public void Save(string path)
    {
        var ini = new IniFile();

        ini.Set("Audio", "sampleRate", SampleRate.ToString(CultureInfo.InvariantCulture));
        ini.Set("Audio", "asioBufferFrames", AsioBufferFrames.ToString(CultureInfo.InvariantCulture));
        ini.Set("Audio", "wasapiBufferFrames", WasapiBufferFrames.ToString(CultureInfo.InvariantCulture));
        ini.Set("Audio", "inputRingFrames", InputRingFrames.ToString(CultureInfo.InvariantCulture));
        ini.Set("Audio", "outputRingFrames", OutputRingFrames.ToString(CultureInfo.InvariantCulture));

        ini.Set("Input", "preferredAsioInputDevice", PreferredAsioInputDevice);
        ini.Set("Input", "hardwareInputChannel", HardwareInputChannel.ToString(CultureInfo.InvariantCulture));
        ini.Set("Input", "inputGain", InputGain.ToString(CultureInfo.InvariantCulture));
        ini.Set("Input", "enableTestTone", EnableTestTone ? "true" : "false");

        ini.Set("Output", "useDefaultWasapiDevice", UseDefaultWasapiDevice ? "true" : "false");
        ini.Set("Output", "preferredWasapiDevice", PreferredWasapiDevice);
        ini.Set("Output", "outputGain", OutputGain.ToString(CultureInfo.InvariantCulture));

        ini.Set("Debug", "enableLogging", EnableLogging ? "true" : "false");

        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        ini.Save(path);
    }
}