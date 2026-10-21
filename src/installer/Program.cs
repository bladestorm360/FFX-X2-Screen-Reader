using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace FfxScreenReader.Setup
{
    internal static class Program
    {
        [STAThread]
        private static void Main(string[] args)
        {
            InstallerRuntime.Initialize();
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            string preferred = null, action = null;
            for (int i = 0; i < args.Length; i++) {
                if (args[i] == "--game" && i + 1 < args.Length) preferred = args[++i];
                else if (args[i] == "--preview-repair") action = "repair";
                else if (args[i] == "--preview-restore") action = "restore";
            }
            Application.Run(new InstallerForm(preferred, action));
        }

        internal static byte[] Resource(string name)
        { return ReadResource("Payload." + name); }

        internal static byte[] ReadResource(string name)
        {
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name))
            {
                if (stream == null) throw new InvalidOperationException("Incomplete installer: " + name);
                using (var result = new MemoryStream()) { stream.CopyTo(result); return result.ToArray(); }
            }
        }

        internal static InstallerEngine Engine()
        {
            var hashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string manifest = System.Text.Encoding.UTF8.GetString(Resource("SHA256SUMS.txt"));
            foreach (string line in manifest.Split('\n'))
            {
                if (String.IsNullOrWhiteSpace(line)) continue;
                Match match = Regex.Match(line.Trim(), "^([a-fA-F0-9]{64})\\s+install/(.+)$");
                if (!match.Success || Array.IndexOf(InstallerEngine.PayloadNames, match.Groups[2].Value) < 0 ||
                    hashes.ContainsKey(match.Groups[2].Value))
                    throw new InvalidOperationException("Invalid embedded installer checksum list.");
                hashes.Add(match.Groups[2].Value, match.Groups[1].Value);
            }
            if (hashes.Count != InstallerEngine.PayloadNames.Length)
                throw new InvalidOperationException("Incomplete embedded installer checksum list.");
            BaseGameCatalog.Load();
            var engine = new InstallerEngine(Resource, hashes);
            engine.ValidateDependencies();
            return engine;
        }
    }

    internal sealed partial class InstallerForm : Form
    {
        private readonly ComboBox games = new ComboBox();
        private readonly TextBox selected = new TextBox();
        private readonly TextBox status = new TextBox();
        private readonly Button browse = new Button();
        private readonly Button install = new Button();
        private readonly Button cancel = new Button();
        private readonly Button diagnose = new Button();
        private readonly BackgroundWorker worker = new BackgroundWorker { WorkerReportsProgress = true };
        private readonly List<string> known = new List<string>();

        public InstallerForm() : this(null, null) { }
        internal InstallerForm(string preferred, string action)
        {
            Text = "Final Fantasy X and X-2 Screen Reader Setup";
            AccessibleName = Text;
            AutoScaleMode = AutoScaleMode.Font;
            Font = SystemFonts.MessageBoxFont;
            MinimumSize = new Size(590, 440);
            Size = new Size(760, 480);
            StartPosition = FormStartPosition.CenterScreen;
            var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(16), ColumnCount = 1, RowCount = 8 };
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            var intro = new Label { AutoSize = true, MaximumSize = new Size(700, 0), Text =
                "Install the screen reader and original launcher speech, including PRISM, SDL3 and their licenses. Tolk is backed up and removed. Close the game, launcher, and mod manager first. " +
                "Settings and saves are preserved. Other added DLL/ASI mods and old Frida files are backed up and removed. Original game files are checked first. Select a detected installation or browse to a Steam folder, library, or game folder. Repair launch redirection is optional and changes the registry only after confirmation and backup." };
            layout.Controls.Add(intro);
            layout.Controls.Add(new Label { Text = "Detected &game installation:", AutoSize = true });
            games.DropDownStyle = ComboBoxStyle.DropDownList;
            games.Dock = DockStyle.Top;
            games.AccessibleName = "Detected game installation";
            games.TabIndex = 0;
            games.SelectedIndexChanged += delegate {
                selected.Text = games.SelectedItem == null ? "" : games.SelectedItem.ToString();
                install.Enabled = repair.Enabled = !worker.IsBusy && selected.Text.Length != 0;
            };
            layout.Controls.Add(games);
            layout.Controls.Add(new Label { Text = "Selected game &folder:", AutoSize = true });
            selected.ReadOnly = true;
            selected.Dock = DockStyle.Top;
            selected.AccessibleName = "Selected game folder";
            selected.TabIndex = 1;
            layout.Controls.Add(selected);
            layout.Controls.Add(new Label { Text = "Setup &status:", AutoSize = true });
            status.ReadOnly = true;
            status.Multiline = true;
            status.ScrollBars = ScrollBars.Vertical;
            status.Dock = DockStyle.Fill;
            status.AccessibleName = "Setup status";
            status.TabIndex = 2;
            layout.Controls.Add(status);
            var buttons = new FlowLayoutPanel { AutoSize = true, Dock = DockStyle.Fill, WrapContents = true };
            browse.Text = "&Browse...";
            install.Text = "&Install";
            cancel.Text = "&Cancel";
            diagnose.Text = "Check launch &redirection";
            diagnose.AccessibleName = "Check launch redirection";
            diagnose.TabIndex = 3;
            foreach (Button button in new[] { browse, install, cancel, diagnose }) { button.AutoSize = true; buttons.Controls.Add(button); }
            browse.TabIndex = 0; install.TabIndex = 1; cancel.TabIndex = 2;
            browse.AccessibleName = "Browse"; install.AccessibleName = "Install"; cancel.AccessibleName = "Cancel";
            AddRegistryButtons(buttons);
            layout.Controls.Add(buttons);
            Controls.Add(layout);
            AcceptButton = install;
            CancelButton = cancel;
            install.Enabled = false;
            diagnose.Click += delegate {
                try { status.Text = LaunchRedirects.Describe(selected.Text); status.Focus(); status.SelectAll(); }
                catch (Exception ex) { ShowMessage("Could not read launch redirection entries. " + ex.Message + ". No registry values were changed.", true); }
            };
            browse.Click += Browse;
            install.Click += StartInstall;
            cancel.Click += delegate { Close(); };
            FormClosing += delegate(object sender, FormClosingEventArgs e) {
                if (worker.IsBusy) { e.Cancel = true; ShowMessage("Please wait while installation finishes or restores previous files.", false); }
            };
            worker.DoWork += delegate(object sender, DoWorkEventArgs e) {
                e.Result = Program.Engine().Install((string)e.Argument, delegate(string text) { worker.ReportProgress(0, text); });
            };
            worker.ProgressChanged += delegate(object sender, ProgressChangedEventArgs e) { status.Text = e.UserState.ToString(); };
            worker.RunWorkerCompleted += Finished;
            Shown += delegate {
                try { AddGames(SteamDiscovery.Find(SteamDiscovery.DefaultRoots())); }
                catch (Exception ex) { status.Text = "Steam discovery could not finish: " + ex.Message + ". Use Browse to select your Steam library or game folder."; }
                if (!String.IsNullOrWhiteSpace(preferred)) {
                    try {
                        AddGames(SteamDiscovery.Find(new[] { preferred }));
                        games.SelectedItem = known.Find(delegate(string path) { return String.Equals(path, preferred, StringComparison.OrdinalIgnoreCase); });
                    } catch (Exception ex) { status.Text = ex.Message; }
                }
                if (known.Count == 0 && status.Text.Length == 0) status.Text = "No installation detected. Use Browse to select your Steam folder, Steam library, or game folder.";
                else if (known.Count != 0) status.Text = "Ready. Check the selected game folder, then choose Install.";
                if (games.Items.Count == 0) browse.Focus(); else games.Focus();
                if (action == "repair" && selected.Text.Length != 0) BeginInvoke(new Action(RepairRedirects));
                else if (action == "restore") BeginInvoke(new Action(RestoreRedirects));
            };
        }

        private void AddGames(List<string> paths)
        {
            foreach (string path in paths)
                if (!known.Exists(delegate(string old) { return String.Equals(old, path, StringComparison.OrdinalIgnoreCase); }))
                { known.Add(path); games.Items.Add(path); }
            if (games.Items.Count != 0 && games.SelectedIndex < 0) games.SelectedIndex = 0;
        }

        private void Browse(object sender, EventArgs e)
        {
            using (var dialog = new FolderBrowserDialog())
            {
                dialog.Description = "Select your Steam directory, Steam library, or Final Fantasy X game folder.";
                dialog.ShowNewFolderButton = false;
                if (dialog.ShowDialog(this) != DialogResult.OK) return;
                try
                {
                    List<string> found = SteamDiscovery.Find(new[] { dialog.SelectedPath });
                    if (found.Count == 0) { ShowMessage("No game installation found in that folder. Select a Steam directory, Steam library, or the folder containing FFX.exe and FFX&X-2_LAUNCHER.exe.", true); return; }
                    AddGames(found);
                    games.SelectedItem = known.Find(delegate(string path) { return String.Equals(path, found[0], StringComparison.OrdinalIgnoreCase); });
                    status.Text = found.Count == 1 ? "Game folder found. Choose Install." : "Several game folders found. Select the one to update, then choose Install.";
                    games.Focus();
                }
                catch (Exception ex) { ShowMessage("Could not read the selected folder. " + ex.Message, true); }
            }
        }

        private void StartInstall(object sender, EventArgs e)
        {
            if (worker.IsBusy || selected.Text.Length == 0) return;
            browse.Enabled = games.Enabled = install.Enabled = cancel.Enabled = diagnose.Enabled = repair.Enabled = restore.Enabled = false;
            status.Text = "Checking installer files, original game files, and other DLL/ASI mods.";
            worker.RunWorkerAsync(selected.Text);
        }

        private void Finished(object sender, RunWorkerCompletedEventArgs e)
        {
            browse.Enabled = games.Enabled = install.Enabled = cancel.Enabled = diagnose.Enabled = repair.Enabled = restore.Enabled = true;
            if (e.Error != null)
            {
                string message = e.Error.Message;
                if (e.Error is UnauthorizedAccessException)
                    message += " Close setup, then use Run as administrator if this game folder requires permission.";
                ShowMessage(message, true);
                return;
            }
            cancel.Text = "&Close";
            cancel.AccessibleName = "Close";
            ShowMessage("Installation complete. Original game files verified; other added DLL/ASI mods removed. Start your screen reader, then launch normally through Steam. Previous files are backed up in: " + e.Result, false);
        }

        private void ShowMessage(string text, bool error)
        {
            status.Text = text;
            MessageBox.Show(this, text, error ? "Setup could not finish" : "Screen reader setup", MessageBoxButtons.OK,
                error ? MessageBoxIcon.Error : MessageBoxIcon.Information);
            status.Focus();
            status.SelectionStart = 0;
            status.SelectionLength = 0;
        }
    }
}
