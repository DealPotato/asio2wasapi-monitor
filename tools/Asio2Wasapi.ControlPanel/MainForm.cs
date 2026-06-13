using System.Diagnostics;
using System.Globalization;

namespace Asio2Wasapi.ControlPanel;

internal sealed class MainForm : Form
{
    private readonly DriverPaths paths;

    private Label statusLabel = null!;
    private TextBox configPathBox = null!;

    private ComboBox sampleRateBox = null!;
    private ComboBox asioBufferBox = null!;
    private ComboBox wasapiBufferBox = null!;
    private ComboBox inputRingBox = null!;
    private ComboBox outputRingBox = null!;

    private ComboBox preferredAsioInputBox = null!;
    private NumericUpDown hardwareInputChannelBox = null!;
    private NumericUpDown inputGainBox = null!;
    private CheckBox enableTestToneBox = null!;

    private CheckBox useDefaultWasapiBox = null!;
    private ComboBox preferredWasapiBox = null!;
    private NumericUpDown outputGainBox = null!;

    private CheckBox enableLoggingBox = null!;

    private Label restartNoticeLabel = null!;

    public MainForm()
    {
        paths = DriverPaths.Resolve();

        Text = "ASIO2WASAPI Control Panel";
        Width = 860;
        Height = 680;
        MinimumSize = new Size(860, 680);
        StartPosition = FormStartPosition.CenterScreen;

        BuildUi();
        LoadDevicesIntoDropdowns();

        if (!File.Exists(paths.ConfigPath))
        {
            new DriverConfig().Save(paths.ConfigPath);
        }

        LoadConfigIntoUi();
        RefreshDriverStatus();
    }

    private void LoadDevicesIntoDropdowns()
    {
        var devices = DeviceScanner.Scan(paths);

        preferredAsioInputBox.Items.Clear();
        preferredWasapiBox.Items.Clear();

        foreach (var device in devices.AsioInputs)
        {
            preferredAsioInputBox.Items.Add(device.Name);
        }

        foreach (var device in devices.WasapiOutputs)
        {
            preferredWasapiBox.Items.Add(device.Name);
        }

        if (!preferredAsioInputBox.Items.Contains("Focusrite"))
            preferredAsioInputBox.Items.Add("Focusrite");

        if (!preferredAsioInputBox.Items.Contains("Scarlett"))
            preferredAsioInputBox.Items.Add("Scarlett");
    }
    private void BuildUi()
    {
        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(14),
            ColumnCount = 1,
            RowCount = 8,
            AutoScroll = true
        };

        Controls.Add(root);

        statusLabel = new Label
        {
            AutoSize = true,
            Font = new Font(Font, FontStyle.Bold)
        };

        root.Controls.Add(statusLabel);

        configPathBox = new TextBox
        {
            ReadOnly = true,
            Dock = DockStyle.Fill
        };

        restartNoticeLabel = new Label
        {
            AutoSize = true,
            ForeColor = Color.DarkOrange,
            Text = "Most settings apply live within a few seconds. Sample rate and host ASIO buffer changes may require reselecting the driver in the host."
        };

        root.Controls.Add(restartNoticeLabel);
        root.Controls.Add(configPathBox);

        root.Controls.Add(CreateAudioGroup());
        root.Controls.Add(CreateInputGroup());
        root.Controls.Add(CreateOutputGroup());
        root.Controls.Add(CreateDebugGroup());
        root.Controls.Add(CreateButtons());
    }

    private GroupBox CreateAudioGroup()
    {
        sampleRateBox = CreateCombo("44100", "48000", "96000");
        asioBufferBox = CreateCombo("64", "128", "256", "512", "1024");
        wasapiBufferBox = CreateCombo("128", "256", "512", "1024");
        inputRingBox = CreateCombo("1024", "2048", "4096", "8192");
        outputRingBox = CreateCombo("1024", "2048", "4096", "8192");

        return CreateGroup("Audio", new ControlRow[]
        {
            new("Sample Rate", sampleRateBox),
            new("ASIO Buffer Frames", asioBufferBox),
            new("WASAPI Buffer Frames", wasapiBufferBox),
            new("Input Ring Frames", inputRingBox),
            new("Output Ring Frames", outputRingBox)
        });
    }

    private GroupBox CreateInputGroup()
    {
        preferredAsioInputBox = new ComboBox
        {
            Dock = DockStyle.Fill,
            DropDownWidth = 520,
            DropDownStyle = ComboBoxStyle.DropDown
        };
        hardwareInputChannelBox = new NumericUpDown
        {
            Minimum = 0,
            Maximum = 63,
            Dock = DockStyle.Left,
            Width = 100
        };

        inputGainBox = CreateGainBox();

        enableTestToneBox = new CheckBox
        {
            Text = "Enable test tone",
            AutoSize = true
        };

        return CreateGroup("Input", new ControlRow[]
        {
            new("Preferred ASIO Input Device", preferredAsioInputBox),
            new("Hardware Input Channel", hardwareInputChannelBox),
            new("Input Gain", inputGainBox),
            new("", enableTestToneBox)
        });
    }

    private GroupBox CreateOutputGroup()
    {
        useDefaultWasapiBox = new CheckBox
        {
            Text = "Use Windows default WASAPI output device",
            AutoSize = true
        };

        preferredWasapiBox = new ComboBox
        {
            Dock = DockStyle.Fill,
            DropDownWidth = 520,
            DropDownStyle = ComboBoxStyle.DropDown
        };

        outputGainBox = CreateGainBox();

        useDefaultWasapiBox.CheckedChanged += (_, _) =>
        {
            if (preferredWasapiBox != null)
            {
                preferredWasapiBox.Enabled = !useDefaultWasapiBox.Checked;
            }
        };

        return CreateGroup("Output", new ControlRow[]
        {
            new("", useDefaultWasapiBox),
            new("Preferred WASAPI Output Device", preferredWasapiBox),
            new("Output Gain", outputGainBox)
        });
    }

    private GroupBox CreateDebugGroup()
    {
        enableLoggingBox = new CheckBox
        {
            Text = "Enable debug logging",
            AutoSize = true
        };

        return CreateGroup("Debug", new ControlRow[]
        {
            new("", enableLoggingBox)
        });
    }

    private FlowLayoutPanel CreateButtons()
    {
        var panel = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true
        };

        var saveButton = new Button { Text = "Save / Apply Live", AutoSize = true };
        var reloadButton = new Button { Text = "Reload", AutoSize = true };
        var installButton = new Button { Text = "Install Driver", AutoSize = true };
        var uninstallButton = new Button { Text = "Uninstall Driver", AutoSize = true };
        var openFolderButton = new Button { Text = "Open Config Folder", AutoSize = true };

        saveButton.Click += (_, _) => SaveConfigFromUi();
        reloadButton.Click += (_, _) => LoadConfigIntoUi();

        installButton.Click += (_, _) =>
        {
            try
            {
                DriverInstaller.InstallWithElevation(paths.DriverDllPath);
                RefreshDriverStatus();
            }
            catch (Exception ex)
            {
                ShowError(ex);
            }
        };

        uninstallButton.Click += (_, _) =>
        {
            try
            {
                DriverInstaller.UninstallWithElevation();
                RefreshDriverStatus();
            }
            catch (Exception ex)
            {
                ShowError(ex);
            }
        };

        openFolderButton.Click += (_, _) =>
        {
            Directory.CreateDirectory(paths.InstalledDriverDirectory);
            Process.Start("explorer.exe", paths.InstalledDriverDirectory);
        };

        panel.Controls.Add(saveButton);
        panel.Controls.Add(reloadButton);
        panel.Controls.Add(installButton);
        panel.Controls.Add(uninstallButton);
        panel.Controls.Add(openFolderButton);

        return panel;
    }

    private void LoadConfigIntoUi()
    {
        var config = DriverConfig.Load(paths.ConfigPath);

        configPathBox.Text = paths.ConfigPath;

        SetComboValue(sampleRateBox, config.SampleRate);
        SetComboValue(asioBufferBox, config.AsioBufferFrames);
        SetComboValue(wasapiBufferBox, config.WasapiBufferFrames);
        SetComboValue(inputRingBox, config.InputRingFrames);
        SetComboValue(outputRingBox, config.OutputRingFrames);

        preferredAsioInputBox.Text = config.PreferredAsioInputDevice;
        hardwareInputChannelBox.Value = config.HardwareInputChannel;
        inputGainBox.Value = ToDecimal(config.InputGain);
        enableTestToneBox.Checked = config.EnableTestTone;

        useDefaultWasapiBox.Checked = config.UseDefaultWasapiDevice;
        preferredWasapiBox.Text = config.PreferredWasapiDevice;
        preferredWasapiBox.Enabled = !useDefaultWasapiBox.Checked;
        outputGainBox.Value = ToDecimal(config.OutputGain);

        enableLoggingBox.Checked = config.EnableLogging;
    }

    private void SaveConfigFromUi()
    {
        try
        {
            var config = new DriverConfig
            {
                SampleRate = ComboInt(sampleRateBox, 48000),
                AsioBufferFrames = ComboInt(asioBufferBox, 128),
                WasapiBufferFrames = ComboInt(wasapiBufferBox, 256),
                InputRingFrames = ComboInt(inputRingBox, 2048),
                OutputRingFrames = ComboInt(outputRingBox, 2048),

                PreferredAsioInputDevice = preferredAsioInputBox.Text.Trim(),
                HardwareInputChannel = (int)hardwareInputChannelBox.Value,
                InputGain = (float)inputGainBox.Value,
                EnableTestTone = enableTestToneBox.Checked,

                UseDefaultWasapiDevice = useDefaultWasapiBox.Checked,
                PreferredWasapiDevice = preferredWasapiBox.Text.Trim(),
                OutputGain = (float)outputGainBox.Value,

                EnableLogging = enableLoggingBox.Checked
            };

            config.Save(paths.ConfigPath);

            MessageBox.Show(
                "Settings saved. Running driver instances will reload most internal settings automatically within a few seconds. Sample rate and host ASIO buffer changes may still require reselecting the driver in the host.",
                "ASIO2WASAPI Control Panel",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            ShowError(ex);
        }
    }

    private void RefreshDriverStatus()
    {
        var installed = DriverInstaller.IsInstalled(paths.DriverDllPath);

        statusLabel.Text = installed
            ? "Driver Status: Installed"
            : "Driver Status: Not Installed";

        statusLabel.ForeColor = installed
            ? Color.DarkGreen
            : Color.DarkRed;
    }

    private static ComboBox CreateCombo(params string[] values)
    {
        var combo = new ComboBox
        {
            Dock = DockStyle.Fill,
            Width = 260,
            DropDownWidth = 360,
            DropDownStyle = ComboBoxStyle.DropDown
        };

        combo.Items.AddRange(values);

        return combo;
    }

    private static NumericUpDown CreateGainBox()
    {
        return new NumericUpDown
        {
            Minimum = 0,
            Maximum = 10,
            DecimalPlaces = 2,
            Increment = 0.05M,
            Width = 100,
            Dock = DockStyle.Left
        };
    }

    private static GroupBox CreateGroup(string title, IReadOnlyList<ControlRow> rows)
    {
        var group = new GroupBox
        {
            Text = title,
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(10)
        };

        var table = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            ColumnCount = 2
        };

        table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 250));
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        foreach (var row in rows)
        {
            table.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            var label = new Label
            {
                Text = row.Label,
                AutoSize = true,
                TextAlign = ContentAlignment.MiddleLeft,
                Dock = DockStyle.Fill,
                Padding = new Padding(0, 6, 0, 6)
            };

            var controlPanel = new Panel
            {
                Dock = DockStyle.Fill,
                Height = 34
            };

            if (row.Control is TextBox || row.Control is ComboBox)
            {
                row.Control.Dock = DockStyle.Fill;
            }
            else
            {
                row.Control.Dock = DockStyle.Left;
}
            controlPanel.Controls.Add(row.Control);

            table.Controls.Add(label);
            table.Controls.Add(controlPanel);
        }

        group.Controls.Add(table);

        return group;
    }

    private static void SetComboValue(ComboBox combo, int value)
    {
        var text = value.ToString(CultureInfo.InvariantCulture);

        if (!combo.Items.Contains(text))
            combo.Items.Add(text);

        combo.Text = text;
    }

    private static int ComboInt(ComboBox combo, int fallback)
    {
        return int.TryParse(combo.Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value)
            ? value
            : fallback;
    }

    private static decimal ToDecimal(float value)
    {
        return Math.Max(0, Math.Min(10, (decimal)value));
    }

    private static void ShowError(Exception ex)
    {
        MessageBox.Show(
            ex.Message,
            "ASIO2WASAPI Control Panel",
            MessageBoxButtons.OK,
            MessageBoxIcon.Error);
    }

    private sealed record ControlRow(string Label, Control Control);
}