using System.Globalization;

namespace Asio2Wasapi.ControlPanel;

internal sealed class IniFile
{
    private readonly Dictionary<string, Dictionary<string, string>> sections =
        new(StringComparer.OrdinalIgnoreCase);

    public static IniFile Load(string path)
    {
        var ini = new IniFile();

        if (!File.Exists(path))
            return ini;

        var currentSection = "";

        foreach (var rawLine in File.ReadAllLines(path))
        {
            var line = rawLine.Trim();

            if (line.Length == 0 || line.StartsWith(';') || line.StartsWith('#'))
                continue;

            if (line.StartsWith('[') && line.EndsWith(']'))
            {
                currentSection = line[1..^1].Trim();
                ini.EnsureSection(currentSection);
                continue;
            }

            var equals = line.IndexOf('=');

            if (equals < 0)
                continue;

            var key = line[..equals].Trim();
            var value = line[(equals + 1)..].Trim();

            ini.Set(currentSection, key, value);
        }

        return ini;
    }

    public void Save(string path)
    {
        using var writer = new StreamWriter(path, false);

        foreach (var section in sections)
        {
            writer.WriteLine($"[{section.Key}]");

            foreach (var value in section.Value)
                writer.WriteLine($"{value.Key}={value.Value}");

            writer.WriteLine();
        }
    }

    public string GetString(string section, string key, string fallback)
    {
        return sections.TryGetValue(section, out var values) &&
               values.TryGetValue(key, out var value)
            ? value
            : fallback;
    }

    public int GetInt(string section, string key, int fallback)
    {
        var value = GetString(section, key, "");

        return int.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var result)
            ? result
            : fallback;
    }

    public float GetFloat(string section, string key, float fallback)
    {
        var value = GetString(section, key, "");

        return float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var result)
            ? result
            : fallback;
    }

    public bool GetBool(string section, string key, bool fallback)
    {
        var value = GetString(section, key, "").Trim().ToLowerInvariant();

        return value switch
        {
            "1" or "true" or "yes" or "on" => true,
            "0" or "false" or "no" or "off" => false,
            _ => fallback
        };
    }

    public void Set(string section, string key, string value)
    {
        EnsureSection(section);
        sections[section][key] = value;
    }

    private void EnsureSection(string section)
    {
        if (!sections.ContainsKey(section))
            sections[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    }
}