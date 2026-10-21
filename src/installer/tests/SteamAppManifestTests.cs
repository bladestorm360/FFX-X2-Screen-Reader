using System;
using System.IO;
using System.Text;
using FfxScreenReader.Setup;

internal static class SteamAppManifestTests
{
    private const string Id = "1235377778411650212";
    private static string Metadata(string folder)
    {
        return "\"AppState\" { \"appid\" \"359870\" \"installdir\" \"" + folder +
            "\" \"InstalledDepots\" { \"359871\" { \"manifest\" \"" + Id + "\" } } }";
    }
    private static void Reject(Action action, Action<bool, string> assert, string message)
    {
        bool rejected = false;
        try { action(); }
        catch (InvalidOperationException ex)
        {
            rejected = ex.Message.Contains("No game files were changed");
        }
        assert(rejected, message);
    }
    public static void Run(string fixture, Action<bool, string> assert)
    {
        string apps = Path.Combine(fixture, "Steam metadata", "steamapps");
        string game = Path.Combine(apps, "common", "Game & Test");
        Directory.CreateDirectory(game);
        string path = Path.Combine(apps, "appmanifest_359870.acf");
        assert(SteamAppManifest.GetInstalledManifest(game) == null, "Missing manifest returns no installed catalog");
        assert(!SteamAppManifest.MatchesGameLocation(game), "Missing manifest does not identify game location");
        File.WriteAllText(path, Metadata("Game & Test"));
        assert(SteamAppManifest.GetInstalledManifest(game) == Id, "Installed depot catalog identity");
        assert(SteamAppManifest.MatchesGameLocation(game), "Manifest identifies damaged game with no executable files");
        assert(SteamAppManifest.GetInstalledManifest(Path.Combine(fixture, "arbitrary")) == null, "Arbitrary folder has no applicable metadata");
        string other = Path.Combine(apps, "common", "Other Game");
        assert(!SteamAppManifest.MatchesGameLocation(other), "Same-library unrelated game is not identified");
        Reject(delegate { SteamAppManifest.GetInstalledManifest(other); }, assert, "Location mismatch refuses catalog");
        File.WriteAllText(path, "// comment before root\n" + Metadata("Game & Test").Replace("AppState", "appSTATE") + " // final comment");
        assert(SteamAppManifest.GetInstalledManifest(game) == Id, "Comments and case-insensitive keys");
        File.WriteAllText(path, Metadata("Game & Test").Replace("\"appid\" \"359870\"", ""));
        assert(SteamAppManifest.GetInstalledManifest(game) == Id, "Optional absent appid accepted");
        File.WriteAllText(path, Metadata("Game & Test").Replace("\"InstalledDepots\"", "\"notes\" \"escaped \\\\ slash \\\"quote\\\" and \\/\" \"InstalledDepots\""));
        assert(SteamAppManifest.GetInstalledManifest(game) == Id, "Backslash quote and slash escapes");
        string valid = Metadata("Game & Test");
        string[] invalid = {
            valid.Replace("359870", "123"),
            valid.Replace("Game & Test", ".."),
            valid.Replace("Game & Test", "../Game"),
            valid.Replace("Game & Test", "Other Game"),
            valid.Replace("\"appid\" \"359870\"", "\"appid\" \"359870\" \"APPID\" \"359870\""),
            valid.Replace("\"manifest\"", "\"manifest\" \"1\" \"Manifest\""),
            valid.Substring(0, valid.Length - 1),
            valid + "}",
            valid + "\"truncated",
            valid.Replace(Id, "-1"),
            valid.Replace(Id, "0"),
            valid.Replace(Id, "18446744073709551616"),
            valid.Replace("359871", "359872"),
            valid.Replace("\"359871\" {", "\"999999\" {} \"359871\" {"),
            valid.Replace("\"manifest\" \"" + Id + "\"", ""),
            valid.Replace("\"InstalledDepots\" { \"359871\" { \"manifest\" \"" + Id + "\" } }", ""),
            valid.Replace("\"appid\" \"359870\"", "\"appid\" {}"),
            valid.Replace("Game & Test", "bad\\q"),
            valid.Replace("\"manifest\" \"" + Id + "\"", "\"manifest\" {}")
        };
        for (int i = 0; i < invalid.Length; i++)
        {
            File.WriteAllText(path, invalid[i]);
            Reject(delegate { SteamAppManifest.GetInstalledManifest(game); }, assert, "Invalid metadata case " + i);
        }
        File.WriteAllText(path, new string(' ', 1048577));
        Reject(delegate { SteamAppManifest.GetInstalledManifest(game); }, assert, "Metadata size limit");
        var deep = new StringBuilder(valid);
        for (int i = 0; i < 34; i++) deep.Append("\"nested\" {");
        for (int i = 0; i < 34; i++) deep.Append("}");
        File.WriteAllText(path, deep.ToString());
        Reject(delegate { SteamAppManifest.GetInstalledManifest(game); }, assert, "Metadata depth limit");
        var many = new StringBuilder(valid);
        for (int i = 0; i < 16385; i++) many.Append("\"key" + i + "\" \"value\" ");
        File.WriteAllText(path, many.ToString());
        Reject(delegate { SteamAppManifest.GetInstalledManifest(game); }, assert, "Metadata node limit");
        File.WriteAllBytes(path, new byte[] { 0xff, 0xfe, 0xff });
        Reject(delegate { SteamAppManifest.GetInstalledManifest(game); }, assert, "Invalid UTF-8 metadata");
        File.WriteAllText(path, valid.Replace(Id, "5168047039601173807"));
        assert(SteamAppManifest.GetInstalledManifest(game) == "5168047039601173807", "Second installed catalog identity");
    }
}
