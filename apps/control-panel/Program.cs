namespace Asio2Wasapi.ControlPanel;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        ApplicationConfiguration.Initialize();

        Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);

        Application.ThreadException += (_, e) =>
        {
            WriteCrashLog(e.Exception);
            MessageBox.Show(
                e.Exception.ToString(),
                "ASIO2WASAPI Control Panel Crash",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
        };

        AppDomain.CurrentDomain.UnhandledException += (_, e) =>
        {
            if (e.ExceptionObject is Exception ex)
                WriteCrashLog(ex);
        };

        try
        {
            if (args.Length > 0)
            {
                if (args[0] == "--install")
                {
                    var dllPath = args.Length > 1
                        ? args[1]
                        : DriverPaths.Resolve().DriverDllPath;

                    DriverInstaller.Install(dllPath);

                    MessageBox.Show(
                        "Driver installed successfully.",
                        "ASIO2WASAPI Control Panel",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Information);

                    return;
                }

                if (args[0] == "--uninstall")
                {
                    DriverInstaller.Uninstall();

                    MessageBox.Show(
                        "Driver uninstalled successfully.",
                        "ASIO2WASAPI Control Panel",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Information);

                    return;
                }
            }

            Application.Run(new MainForm());
        }
        catch (Exception ex)
        {
            WriteCrashLog(ex);

            MessageBox.Show(
                ex.ToString(),
                "ASIO2WASAPI Control Panel Startup Crash",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
        }
    }

    private static void WriteCrashLog(Exception ex)
    {
        try
        {
            var path = Path.Combine(
                AppContext.BaseDirectory,
                "asio2wasapi-control-crash.log");

            File.WriteAllText(path, ex.ToString());
        }
        catch
        {
        }
    }
}