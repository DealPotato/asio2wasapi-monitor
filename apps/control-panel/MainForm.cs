using System.Diagnostics;
using System.Globalization;

namespace Asio2Wasapi.ControlPanel;

internal sealed class MainForm : Form
{
    private static readonly Color Background = Color.FromArgb(15, 18, 24);
    private static readonly Color Sidebar = Color.FromArgb(11, 14, 20);
    private static readonly Color Card = Color.FromArgb(24, 28, 37);
    private static readonly Color CardSoft = Color.FromArgb(29, 35, 47);
    private static readonly Color Border = Color.FromArgb(48, 57, 74);
    private static readonly Color TextPrimary = Color.FromArgb(234, 238, 247);
    private static readonly Color TextMuted = Color.FromArgb(157, 166, 181);
    private static readonly Color Accent = Color.FromArgb(70, 145, 255);
    private static readonly Color Good = Color.FromArgb(72, 222, 128);
    private static readonly Color Warning = Color.FromArgb(250, 204, 21);
    private static readonly Color Danger = Color.FromArgb(248, 113, 113);

    private readonly DriverPaths paths;

    private Label driverStatusValue = null!;
    private Label headerStatusLabel = null!;
    private Label signalInputLabel = null!;
    private Label signalChannelLabel = null!;
    private Label signalOutputLabel = null!;
    private Label signalModeLabel = null!;
    private Label presetDetailsLabel = null!;
    private Label settingsSummaryLabel = null!;
    private Label estimatedLatencyLabel = null!;
    private Label configPathLabel = null!;

    private ComboBox sampleRateBox = null!;
    private ComboBox asioBufferBox = null!;
    private ComboBox wasapiBufferBox = null!;
    private ComboBox inputRingBox = null!;
    private ComboBox outputRingBox = null!;

    private ComboBox preferredAsioInputBox = null!;
    private ComboBox hardwareInputChannelBox = null!;
    private NumericUpDown inputGainBox = null!;
    private CheckBox enableTestToneBox = null!;

    private CheckBox wasapiExclusiveModeBox = null!;
    private CheckBox useDefaultWasapiBox = null!;
    private ComboBox preferredWasapiBox = null!;
    private NumericUpDown outputGainBox = null!;

    private CheckBox enableLoggingBox = null!;

    private Button installButton = null!;
    private Button uninstallButton = null!;

    public MainForm()
    {
        paths = DriverPaths.Resolve();

        Text = "ASIO2WASAPI Monitor";
        Width = 1280;
        Height = 860;
        MinimumSize = new Size(1100, 760);
        AutoScaleMode = AutoScaleMode.Dpi;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Background;
        ForeColor = TextPrimary;
        Font = new Font("Segoe UI", 9.5f);

        BuildUi();
        LoadDevicesIntoDropdowns();

        if (!File.Exists(paths.ConfigPath))
        {
            new DriverConfig().Save(paths.ConfigPath);
        }

        LoadConfigIntoUi();
        RefreshDriverStatus();
    }

    private void BuildUi()
    {
        // Single-screen layout. The old left navigation looked nice, but it had
        // no real pages behind it and wasted horizontal room, so keep the app
        // focused on the controls that actually work.
        Controls.Clear();
        Controls.Add(CreateMainArea());
    }

    private Control CreateSidebar()
    {
        var panel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            BackColor = Sidebar,
            Padding = new Padding(18),
            RowCount = 4,
            ColumnCount = 1
        };
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 135));
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 235));
        panel.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 160));

        var title = new Label
        {
            Text = "ASIO2WASAPI\nMONITOR",
            Dock = DockStyle.Fill,
            Font = new Font("Segoe UI", 18f, FontStyle.Bold),
            ForeColor = TextPrimary,
            TextAlign = ContentAlignment.MiddleLeft
        };
        panel.Controls.Add(title, 0, 0);

        var nav = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            FlowDirection = FlowDirection.TopDown,
            WrapContents = false,
            BackColor = Sidebar
        };
        nav.Controls.Add(CreateNavItem("Overview", selected: true));
        nav.Controls.Add(CreateNavItem("Devices"));
        nav.Controls.Add(CreateNavItem("Latency"));
        nav.Controls.Add(CreateNavItem("Diagnostics"));
        nav.Controls.Add(CreateNavItem("About"));
        panel.Controls.Add(nav, 0, 1);

        var statusCard = CreateCardPanel();
        statusCard.Padding = new Padding(14);
        var statusLayout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 6,
            ColumnCount = 1,
            BackColor = Card
        };
        statusLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 28));
        statusLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 24));
        statusLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 26));
        statusLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 24));
        statusLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 26));
        statusLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        statusCard.Controls.Add(statusLayout);

        statusLayout.Controls.Add(CreateTextLabel("Driver Status", 10.5f, FontStyle.Bold), 0, 0);
        driverStatusValue = CreateTextLabel("Checking...", 10f, FontStyle.Bold, Warning);
        statusLayout.Controls.Add(driverStatusValue, 0, 1);
        statusLayout.Controls.Add(CreateTextLabel("Version", 9f, FontStyle.Regular, TextMuted), 0, 2);
        statusLayout.Controls.Add(CreateTextLabel("0.1.0", 10f, FontStyle.Bold), 0, 3);
        uninstallButton = CreateButton("Uninstall Driver", danger: true);
        uninstallButton.Dock = DockStyle.Top;
        uninstallButton.Click += (_, _) => UninstallDriver();
        statusLayout.Controls.Add(uninstallButton, 0, 5);

        panel.Controls.Add(statusCard, 0, 3);
        return panel;
    }

    private static Control CreateNavItem(string text, bool selected = false)
    {
        var item = new Label
        {
            Text = selected ? $"  ●  {text}" : $"     {text}",
            Width = 205,
            Height = 42,
            TextAlign = ContentAlignment.MiddleLeft,
            Font = new Font("Segoe UI", 10f, selected ? FontStyle.Bold : FontStyle.Regular),
            ForeColor = selected ? TextPrimary : TextMuted,
            BackColor = selected ? Color.FromArgb(30, 69, 123) : Sidebar,
            Margin = new Padding(0, 0, 0, 8)
        };
        return item;
    }

    private Control CreateMainArea()
    {
        var host = new Panel
        {
            Dock = DockStyle.Fill,
            AutoScroll = true,
            Padding = new Padding(18),
            BackColor = Background
        };

        var main = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            ColumnCount = 1,
            RowCount = 6,
            BackColor = Background
        };
        main.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        for (var i = 0; i < 6; ++i)
            main.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        host.Resize += (_, _) =>
        {
            main.Width = Math.Max(900, host.ClientSize.Width - SystemInformation.VerticalScrollBarWidth - 8);
        };

        host.Controls.Add(main);

        main.Controls.Add(WithPreferredHeight(CreateHeader(), 82), 0, 0);
        main.Controls.Add(WithPreferredHeight(CreateSignalPathCard(), 176), 0, 1);

        var twoColumns = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            BackColor = Background,
            Margin = new Padding(0, 14, 0, 0)
        };
        twoColumns.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 45));
        twoColumns.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 55));
        twoColumns.Controls.Add(CreateDevicesCard(), 0, 0);
        twoColumns.Controls.Add(CreateLatencyCard(), 1, 0);
        main.Controls.Add(WithPreferredHeight(twoColumns, 390), 0, 2);

        main.Controls.Add(WithPreferredHeight(CreateCurrentSettingsCard(), 215), 0, 3);
        main.Controls.Add(WithPreferredHeight(CreateToolsCard(), 180), 0, 4);
        main.Controls.Add(WithPreferredHeight(CreateFooterButtons(), 72), 0, 5);

        return host;
    }

    private static Control WithPreferredHeight(Control control, int height)
    {
        control.Height = height;
        control.MinimumSize = new Size(0, height);
        control.Margin = new Padding(0, 0, 0, 14);
        control.Dock = DockStyle.Top;
        return control;
    }

    private Control CreateHeader()
    {
        var header = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            BackColor = Background
        };
        header.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 60));
        header.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 40));

        var titlePanel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            BackColor = Background
        };
        titlePanel.RowStyles.Add(new RowStyle(SizeType.Absolute, 46));
        titlePanel.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        titlePanel.Controls.Add(new Label
        {
            Text = "ASIO2WASAPI Monitor",
            Dock = DockStyle.Fill,
            Font = new Font("Segoe UI", 22f, FontStyle.Bold),
            ForeColor = TextPrimary
        }, 0, 0);
        titlePanel.Controls.Add(new Label
        {
            Text = "Virtual ASIO input to WASAPI output for DAWs, amp sims and plugin hosts.",
            Dock = DockStyle.Fill,
            Font = new Font("Segoe UI", 10f),
            ForeColor = TextMuted
        }, 0, 1);

        headerStatusLabel = new Label
        {
            Text = "● Checking driver",
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleRight,
            Font = new Font("Segoe UI", 10.5f, FontStyle.Bold),
            ForeColor = Warning
        };
        driverStatusValue = headerStatusLabel;

        header.Controls.Add(titlePanel, 0, 0);
        header.Controls.Add(headerStatusLabel, 1, 0);
        return header;
    }

    private Control CreateSignalPathCard()
    {
        var card = CreateCardPanel();
        card.Padding = new Padding(18);

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 5,
            RowCount = 2,
            BackColor = Card
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 52));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 31));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 44));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 31));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 44));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 31));
        card.Controls.Add(layout);

        var header = CreateCardHeader("Signal Path", "Audio flow from hardware input to your output device");
        layout.Controls.Add(header, 0, 0);
        layout.SetColumnSpan(header, 5);

        signalInputLabel = CreateTextLabel("Focusrite USB ASIO", 10f, FontStyle.Bold);
        signalChannelLabel = CreateBadge("Input 2");
        layout.Controls.Add(CreatePathBox("ASIO Input", signalInputLabel, signalChannelLabel), 0, 1);
        layout.Controls.Add(CreateArrow(), 1, 1);
        layout.Controls.Add(CreatePathBox("Virtual ASIO Driver", CreateTextLabel("ASIO2WASAPI", 10f, FontStyle.Bold), CreateBadge("Host active")), 2, 1);
        layout.Controls.Add(CreateArrow(), 3, 1);
        signalOutputLabel = CreateTextLabel("Windows default", 10f, FontStyle.Bold);
        signalModeLabel = CreateBadge("Shared Mode");
        layout.Controls.Add(CreatePathBox("WASAPI Output", signalOutputLabel, signalModeLabel), 4, 1);

        return card;
    }

    private Control CreateDevicesCard()
    {
        var card = CreateCardPanel();
        card.Margin = new Padding(0, 0, 8, 0);
        card.Padding = new Padding(18);

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 7,
            ColumnCount = 1,
            BackColor = Card
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 60));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 60));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 60));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        card.Controls.Add(layout);

        layout.Controls.Add(CreateCardHeader("Devices", "Choose the hardware input and listening output"), 0, 0);

        preferredAsioInputBox = CreateCombo();
        layout.Controls.Add(CreateLabeledControl("ASIO Input Device", preferredAsioInputBox), 0, 1);

        hardwareInputChannelBox = CreateCombo("Input 1", "Input 2", "Input 3", "Input 4", "Input 5", "Input 6", "Input 7", "Input 8");
        layout.Controls.Add(CreateLabeledControl("Guitar Input Channel", hardwareInputChannelBox), 0, 2);

        preferredWasapiBox = CreateCombo();
        layout.Controls.Add(CreateLabeledControl("WASAPI Output Device", preferredWasapiBox), 0, 3);

        useDefaultWasapiBox = CreateCheckBox("Use Windows default output device");
        useDefaultWasapiBox.CheckedChanged += (_, _) =>
        {
            preferredWasapiBox.Enabled = !useDefaultWasapiBox.Checked;
            UpdateSettingsSummary();
        };
        layout.Controls.Add(useDefaultWasapiBox, 0, 4);

        wasapiExclusiveModeBox = CreateCheckBox("Use WASAPI exclusive mode");
        wasapiExclusiveModeBox.CheckedChanged += (_, _) => UpdateSettingsSummary();
        layout.Controls.Add(wasapiExclusiveModeBox, 0, 5);

        preferredAsioInputBox.TextChanged += (_, _) => UpdateSettingsSummary();
        preferredWasapiBox.TextChanged += (_, _) => UpdateSettingsSummary();
        hardwareInputChannelBox.TextChanged += (_, _) => UpdateSettingsSummary();

        return card;
    }

    private Control CreateLatencyCard()
    {
        var card = CreateCardPanel();
        card.Margin = new Padding(8, 0, 0, 0);
        card.Padding = new Padding(18);

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 5,
            ColumnCount = 1,
            BackColor = Card
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 70));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 78));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 145));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        card.Controls.Add(layout);

        layout.Controls.Add(CreateCardHeader("Latency Presets", "Start stable, then move lower only if your system stays clean"), 0, 0);

        var presets = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 4,
            RowCount = 1,
            BackColor = Card
        };
        presets.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        presets.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        presets.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        presets.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));

        presets.Controls.Add(CreatePresetButton("Safe", Preset.Safe), 0, 0);
        presets.Controls.Add(CreatePresetButton("Balanced", Preset.Balanced), 1, 0);
        presets.Controls.Add(CreatePresetButton("Low Latency", Preset.LowLatency), 2, 0);
        presets.Controls.Add(CreatePresetButton("Experimental", Preset.Experimental), 3, 0);
        layout.Controls.Add(presets, 0, 1);

        presetDetailsLabel = CreateInfoBox("Balanced", "128-frame WASAPI buffer, 1024-frame safety buffers, exclusive mode enabled.");
        layout.Controls.Add(presetDetailsLabel, 0, 2);

        var grid = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 4,
            RowCount = 2,
            BackColor = CardSoft,
            Padding = new Padding(10),
            Margin = new Padding(0, 4, 0, 0)
        };
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
        grid.RowStyles.Add(new RowStyle(SizeType.Percent, 50));
        grid.RowStyles.Add(new RowStyle(SizeType.Percent, 50));

        wasapiBufferBox = CreateCombo("64", "128", "256", "512");
        inputRingBox = CreateCombo("512", "768", "1024", "2048", "4096");
        outputRingBox = CreateCombo("512", "768", "1024", "2048", "4096");
        outputGainBox = CreateGainBox();
        inputGainBox = CreateGainBox();

        grid.Controls.Add(CreateMiniControl("WASAPI Buffer", wasapiBufferBox), 0, 0);
        grid.Controls.Add(CreateMiniControl("Input Safety", inputRingBox), 1, 0);
        grid.Controls.Add(CreateMiniControl("Output Safety", outputRingBox), 2, 0);
        grid.Controls.Add(CreateMiniControl("Output Gain", outputGainBox), 3, 0);
        grid.Controls.Add(CreateMiniControl("Input Gain", inputGainBox), 0, 1);

        sampleRateBox = CreateCombo("44100", "48000");
        asioBufferBox = CreateCombo("64", "128", "256", "512");
        grid.Controls.Add(CreateMiniControl("Sample Rate", sampleRateBox), 1, 1);
        grid.Controls.Add(CreateMiniControl("Host Buffer Hint", asioBufferBox), 2, 1);

        enableTestToneBox = CreateCheckBox("Test tone");
        enableLoggingBox = CreateCheckBox("Debug log");
        var checks = new FlowLayoutPanel { Dock = DockStyle.Fill, BackColor = CardSoft };
        checks.Controls.Add(enableTestToneBox);
        checks.Controls.Add(enableLoggingBox);
        grid.Controls.Add(checks, 3, 1);

        layout.Controls.Add(grid, 0, 3);

        sampleRateBox.TextChanged += (_, _) => UpdateSettingsSummary();
        wasapiBufferBox.TextChanged += (_, _) => UpdateSettingsSummary();
        inputRingBox.TextChanged += (_, _) => UpdateSettingsSummary();
        outputRingBox.TextChanged += (_, _) => UpdateSettingsSummary();
        inputGainBox.ValueChanged += (_, _) => UpdateSettingsSummary();
        outputGainBox.ValueChanged += (_, _) => UpdateSettingsSummary();

        return card;
    }

    private Control CreateCurrentSettingsCard()
    {
        var card = CreateCardPanel();
        card.Margin = new Padding(0, 14, 0, 0);
        card.Padding = new Padding(18);

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 3,
            ColumnCount = 1,
            BackColor = Card
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 58));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        card.Controls.Add(layout);

        layout.Controls.Add(CreateCardHeader("Current Settings", "Saved to asio2wasapi-monitor.ini next to the driver DLL"), 0, 0);
        settingsSummaryLabel = CreateTextLabel("", 10.5f, FontStyle.Bold);
        settingsSummaryLabel.AutoEllipsis = false;
        settingsSummaryLabel.TextAlign = ContentAlignment.MiddleLeft;

        estimatedLatencyLabel = CreateTextLabel("", 9.3f, FontStyle.Regular, TextMuted);
        estimatedLatencyLabel.AutoEllipsis = false;
        estimatedLatencyLabel.TextAlign = ContentAlignment.TopLeft;
        estimatedLatencyLabel.Padding = new Padding(0, 4, 0, 0);

        layout.Controls.Add(settingsSummaryLabel, 0, 1);
        layout.Controls.Add(estimatedLatencyLabel, 0, 2);
        return card;
    }

    private Control CreateToolsCard()
    {
        var card = CreateCardPanel();
        card.Margin = new Padding(0, 14, 0, 0);
        card.Padding = new Padding(18);

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            BackColor = Card
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 58));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        card.Controls.Add(layout);

        layout.Controls.Add(CreateCardHeader("Tools & Diagnostics", "Logging is off by default for cleaner low-latency audio"), 0, 0);

        var body = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 1,
            ColumnCount = 2,
            BackColor = Card
        };
        body.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        body.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 500));

        configPathLabel = CreateTextLabel(paths.ConfigPath, 9f, FontStyle.Regular, TextMuted);
        configPathLabel.Dock = DockStyle.Fill;
        configPathLabel.AutoEllipsis = true;
        configPathLabel.TextAlign = ContentAlignment.MiddleLeft;
        body.Controls.Add(configPathLabel, 0, 0);

        var buttons = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            FlowDirection = FlowDirection.RightToLeft,
            WrapContents = false,
            Padding = new Padding(0, 14, 0, 0),
            BackColor = Card
        };
        var openFolderButton = CreateButton("Open Folder");
        var openLogButton = CreateButton("Open Log");
        var reloadButton = CreateButton("Reload");

        openFolderButton.Click += (_, _) => OpenConfigFolder();
        openLogButton.Click += (_, _) => OpenLogFile();
        reloadButton.Click += (_, _) => LoadConfigIntoUi();

        buttons.Controls.Add(openFolderButton);
        buttons.Controls.Add(openLogButton);
        buttons.Controls.Add(reloadButton);
        body.Controls.Add(buttons, 1, 0);

        layout.Controls.Add(body, 0, 1);

        return card;
    }

    private Control CreateFooterButtons()
    {
        var footer = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            FlowDirection = FlowDirection.RightToLeft,
            WrapContents = false,
            Padding = new Padding(0, 8, 0, 0),
            BackColor = Background
        };

        var saveButton = CreateButton("Save Settings", primary: true);
        var restartButton = CreateButton("Restart in Host");
        installButton = CreateButton("Install Driver");
        uninstallButton = CreateButton("Uninstall Driver", danger: true);

        saveButton.Click += (_, _) => SaveConfigFromUi();
        restartButton.Click += (_, _) => MessageBox.Show(
            "To restart the audio engine, reselect ASIO2WASAPI Virtual ASIO in your DAW/plugin host or restart the host application.",
            "ASIO2WASAPI Monitor",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);
        installButton.Click += (_, _) => InstallDriver();
        uninstallButton.Click += (_, _) => UninstallDriver();

        footer.Controls.Add(saveButton);
        footer.Controls.Add(restartButton);
        footer.Controls.Add(uninstallButton);
        footer.Controls.Add(installButton);
        return footer;
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

    private void LoadConfigIntoUi()
    {
        var config = DriverConfig.Load(paths.ConfigPath);

        SetComboValue(sampleRateBox, config.SampleRate);
        SetComboValue(asioBufferBox, config.AsioBufferFrames);
        SetComboValue(wasapiBufferBox, config.WasapiBufferFrames);
        SetComboValue(inputRingBox, config.InputRingFrames);
        SetComboValue(outputRingBox, config.OutputRingFrames);

        preferredAsioInputBox.Text = config.PreferredAsioInputDevice;
        hardwareInputChannelBox.SelectedIndex = Math.Max(0, Math.Min(7, config.HardwareInputChannel));
        inputGainBox.Value = ToDecimal(config.InputGain);
        enableTestToneBox.Checked = config.EnableTestTone;

        useDefaultWasapiBox.Checked = config.UseDefaultWasapiDevice;
        preferredWasapiBox.Text = config.PreferredWasapiDevice;
        preferredWasapiBox.Enabled = !useDefaultWasapiBox.Checked;
        wasapiExclusiveModeBox.Checked = config.WasapiExclusiveMode;
        outputGainBox.Value = ToDecimal(config.OutputGain);

        enableLoggingBox.Checked = config.EnableLogging;

        UpdateSettingsSummary();
    }

    private void SaveConfigFromUi()
    {
        try
        {
            var config = new DriverConfig
            {
                SampleRate = ComboInt(sampleRateBox, 48000),
                AsioBufferFrames = ComboInt(asioBufferBox, 128),
                WasapiBufferFrames = ComboInt(wasapiBufferBox, 128),
                InputRingFrames = ComboInt(inputRingBox, 1024),
                OutputRingFrames = ComboInt(outputRingBox, 1024),

                PreferredAsioInputDevice = preferredAsioInputBox.Text.Trim(),
                HardwareInputChannel = Math.Max(0, hardwareInputChannelBox.SelectedIndex),
                InputGain = (float)inputGainBox.Value,
                EnableTestTone = enableTestToneBox.Checked,

                UseDefaultWasapiDevice = useDefaultWasapiBox.Checked,
                PreferredWasapiDevice = preferredWasapiBox.Text.Trim(),
                WasapiExclusiveMode = wasapiExclusiveModeBox.Checked,
                OutputGain = (float)outputGainBox.Value,

                EnableLogging = enableLoggingBox.Checked
            };

            config.Save(paths.ConfigPath);
            UpdateSettingsSummary();

            MessageBox.Show(
                "Settings saved. Restart or reselect the ASIO2WASAPI driver in your host for driver-level changes to take effect.",
                "ASIO2WASAPI Monitor",
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

        driverStatusValue.Text = installed ? "● Installed" : "● Not installed";
        driverStatusValue.ForeColor = installed ? Good : Danger;
        headerStatusLabel.Text = installed ? "● Driver installed" : "● Driver not installed";
        headerStatusLabel.ForeColor = installed ? Good : Danger;

        installButton.Enabled = !installed;
        installButton.Visible = !installed;
        uninstallButton.Enabled = installed;
        uninstallButton.Visible = installed;
    }

    private void InstallDriver()
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
    }

    private void UninstallDriver()
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
    }

    private void OpenConfigFolder()
    {
        Directory.CreateDirectory(paths.InstalledDriverDirectory);
        Process.Start("explorer.exe", paths.InstalledDriverDirectory);
    }

    private static void OpenLogFile()
    {
        var path = Path.Combine(Path.GetTempPath(), "asio2wasapi-driver.log");

        if (!File.Exists(path))
        {
            MessageBox.Show(
                "No driver log file exists yet. Enable debug logging, save settings, then restart the driver in your host.",
                "ASIO2WASAPI Monitor",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            return;
        }

        Process.Start(new ProcessStartInfo
        {
            FileName = "notepad.exe",
            Arguments = $"\"{path}\"",
            UseShellExecute = true
        });
    }

    private void ApplyPreset(Preset preset)
    {
        switch (preset)
        {
            case Preset.Safe:
                SetComboValue(wasapiBufferBox, 256);
                SetComboValue(inputRingBox, 2048);
                SetComboValue(outputRingBox, 2048);
                wasapiExclusiveModeBox.Checked = false;
                presetDetailsLabel.Text = "Safe\nMaximum stability. Higher latency, recommended when testing a new machine or output device.";
                break;

            case Preset.Balanced:
                SetComboValue(wasapiBufferBox, 128);
                SetComboValue(inputRingBox, 1024);
                SetComboValue(outputRingBox, 1024);
                wasapiExclusiveModeBox.Checked = true;
                presetDetailsLabel.Text = "Balanced\nGood default for guitar monitoring: stable buffer depth with WASAPI exclusive mode.";
                break;

            case Preset.LowLatency:
                SetComboValue(wasapiBufferBox, 128);
                SetComboValue(inputRingBox, 1024);
                SetComboValue(outputRingBox, 768);
                wasapiExclusiveModeBox.Checked = true;
                presetDetailsLabel.Text = "Low Latency\nLower output safety buffer. Use this only if playback stays clean.";
                break;

            case Preset.Experimental:
                SetComboValue(wasapiBufferBox, 64);
                SetComboValue(inputRingBox, 512);
                SetComboValue(outputRingBox, 512);
                wasapiExclusiveModeBox.Checked = true;
                presetDetailsLabel.Text = "Experimental\nLowest buffer values. Expect crackles on some systems; use for testing only.";
                break;
        }

        UpdateSettingsSummary();
    }

    private void UpdateSettingsSummary()
    {
        if (settingsSummaryLabel == null)
            return;

        var sampleRate = ComboInt(sampleRateBox, 48000);
        var wasapi = ComboInt(wasapiBufferBox, 128);
        var inputRing = ComboInt(inputRingBox, 1024);
        var outputRing = ComboInt(outputRingBox, 1024);
        var channel = Math.Max(0, hardwareInputChannelBox.SelectedIndex) + 1;

        signalInputLabel.Text = string.IsNullOrWhiteSpace(preferredAsioInputBox.Text)
            ? "Any ASIO input"
            : preferredAsioInputBox.Text;
        signalChannelLabel.Text = $"Input {channel}";

        signalOutputLabel.Text = useDefaultWasapiBox.Checked || string.IsNullOrWhiteSpace(preferredWasapiBox.Text)
            ? "Windows default"
            : preferredWasapiBox.Text;
        signalModeLabel.Text = wasapiExclusiveModeBox.Checked ? "Exclusive Mode" : "Shared Mode";

        settingsSummaryLabel.Text =
            $"{sampleRate:N0} Hz  •  WASAPI {wasapi}f  •  Input safety {inputRing}f  •  Output safety {outputRing}f  •  Input {channel}";

        var outputMs = ((double)(wasapi + outputRing) / Math.Max(1, sampleRate)) * 1000.0;
        var stability = outputRing >= 1024 ? "Stable" : outputRing >= 768 ? "Tight" : "Experimental";
        var color = outputRing >= 1024 ? Good : outputRing >= 768 ? Warning : Danger;

        estimatedLatencyLabel.Text =
            $"Estimated output buffer latency: ~{outputMs:0.0} ms  •  Stability: {stability}\n" +
            "Actual round-trip latency also depends on the ASIO host, plugins and output device.";
        estimatedLatencyLabel.ForeColor = color;
    }

    private Button CreatePresetButton(string text, Preset preset)
    {
        var button = CreateButton(text);
        button.Dock = DockStyle.Fill;
        button.Margin = new Padding(4);
        button.Click += (_, _) => ApplyPreset(preset);
        return button;
    }

    private static Panel CreateCardPanel()
    {
        return new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Card,
            BorderStyle = BorderStyle.FixedSingle,
            Margin = new Padding(0)
        };
    }

    private static Control CreateCardHeader(string title, string subtitle)
    {
        var panel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            BackColor = Card
        };
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 24));
        panel.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        panel.Controls.Add(CreateTextLabel(title, 12f, FontStyle.Bold), 0, 0);
        panel.Controls.Add(CreateTextLabel(subtitle, 9f, FontStyle.Regular, TextMuted), 0, 1);
        return panel;
    }

    private static Control CreatePathBox(string title, Label main, Label badge)
    {
        var box = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            BackColor = CardSoft,
            Padding = new Padding(14),
            RowCount = 3,
            ColumnCount = 1,
            Margin = new Padding(0, 4, 0, 0)
        };
        box.RowStyles.Add(new RowStyle(SizeType.Absolute, 22));
        box.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
        box.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        box.Controls.Add(CreateTextLabel(title, 10f, FontStyle.Bold), 0, 0);
        box.Controls.Add(main, 0, 1);
        box.Controls.Add(badge, 0, 2);
        return box;
    }

    private static Label CreateArrow()
    {
        return new Label
        {
            Text = "→",
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleCenter,
            ForeColor = Accent,
            Font = new Font("Segoe UI", 26f, FontStyle.Regular),
            BackColor = Card
        };
    }

    private static Label CreateBadge(string text)
    {
        return new Label
        {
            Text = text,
            AutoSize = true,
            ForeColor = Color.White,
            BackColor = Color.FromArgb(36, 86, 153),
            Padding = new Padding(7, 3, 7, 3),
            Margin = new Padding(0, 4, 0, 0)
        };
    }

    private static Label CreateInfoBox(string title, string body)
    {
        return new Label
        {
            Text = $"{title}\n{body}",
            Dock = DockStyle.Fill,
            ForeColor = TextPrimary,
            BackColor = CardSoft,
            Padding = new Padding(12),
            TextAlign = ContentAlignment.MiddleLeft
        };
    }

    private static Control CreateLabeledControl(string label, Control control)
    {
        var panel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            BackColor = Card
        };
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 22));
        panel.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        panel.Controls.Add(CreateTextLabel(label, 9f, FontStyle.Bold), 0, 0);
        panel.Controls.Add(control, 0, 1);
        return panel;
    }

    private static Control CreateMiniControl(string label, Control control)
    {
        var panel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            BackColor = CardSoft,
            Padding = new Padding(4),
            Margin = new Padding(2)
        };
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 18));
        panel.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        panel.Controls.Add(CreateTextLabel(label, 8.5f, FontStyle.Regular, TextMuted), 0, 0);
        panel.Controls.Add(control, 0, 1);
        return panel;
    }

    private static ComboBox CreateCombo(params string[] values)
    {
        var combo = new ComboBox
        {
            Dock = DockStyle.Fill,
            DropDownWidth = 520,
            DropDownStyle = ComboBoxStyle.DropDown,
            BackColor = Color.FromArgb(12, 15, 21),
            ForeColor = TextPrimary,
            FlatStyle = FlatStyle.Flat
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
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(12, 15, 21),
            ForeColor = TextPrimary,
            BorderStyle = BorderStyle.FixedSingle
        };
    }

    private static CheckBox CreateCheckBox(string text)
    {
        return new CheckBox
        {
            Text = text,
            AutoSize = true,
            ForeColor = TextPrimary,
            BackColor = Color.Transparent,
            Margin = new Padding(0, 6, 12, 0)
        };
    }

    private static Button CreateButton(string text, bool primary = false, bool danger = false)
    {
        var button = new Button
        {
            Text = text,
            AutoSize = false,
            Width = 145,
            Height = 38,
            FlatStyle = FlatStyle.Flat,
            BackColor = primary ? Accent : danger ? Color.FromArgb(86, 28, 36) : Color.FromArgb(20, 25, 34),
            ForeColor = danger ? Danger : TextPrimary,
            Margin = new Padding(8, 0, 0, 0),
            Font = new Font("Segoe UI", 9.5f, FontStyle.Bold)
        };
        button.FlatAppearance.BorderColor = danger ? Danger : primary ? Accent : Border;
        button.FlatAppearance.MouseOverBackColor = primary ? Color.FromArgb(82, 156, 255) : Color.FromArgb(35, 42, 56);
        return button;
    }

    private static Label CreateTextLabel(
        string text,
        float size,
        FontStyle style = FontStyle.Regular,
        Color? color = null)
    {
        return new Label
        {
            Text = text,
            Dock = DockStyle.Fill,
            AutoEllipsis = true,
            Font = new Font("Segoe UI", size, style),
            ForeColor = color ?? TextPrimary,
            BackColor = Color.Transparent,
            TextAlign = ContentAlignment.MiddleLeft
        };
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
            "ASIO2WASAPI Monitor",
            MessageBoxButtons.OK,
            MessageBoxIcon.Error);
    }

    private enum Preset
    {
        Safe,
        Balanced,
        LowLatency,
        Experimental
    }
}
