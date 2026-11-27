using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
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

        internal static byte[] ReadResource(string name)
        {
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name))
            {
                if (stream == null) throw new InvalidOperationException("Incomplete installer: " + name);
                using (var result = new MemoryStream()) { stream.CopyTo(result); return result.ToArray(); }
            }
        }

        // The release build and scanner use the production reader on locally
        // staged bytes before any archive is published.
        internal static Func<string, byte[]> VerifyReleaseArchive(byte[] bytes, string tag)
        {
            Func<string, byte[]> resources = GitHubUpdates.OpenPackage(bytes, tag);
            CreateEngine(resources).ValidateNativePayload();
            return resources;
        }

        internal static InstallerEngine CreateEngine(Func<string, byte[]> resources)
        {
            var hashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string manifest = System.Text.Encoding.UTF8.GetString(resources("SHA256SUMS.txt"));
            foreach (string line in manifest.Split('\n'))
            {
                if (String.IsNullOrWhiteSpace(line)) continue;
                Match match = Regex.Match(line.Trim(), "^([a-fA-F0-9]{64})\\s+install/(.+)$");
                if (!match.Success || Array.IndexOf(InstallerEngine.PayloadNames, match.Groups[2].Value) < 0 ||
                    hashes.ContainsKey(match.Groups[2].Value))
                    throw new InvalidOperationException("Invalid release payload checksum list.");
                hashes.Add(match.Groups[2].Value, match.Groups[1].Value);
            }
            if (hashes.Count != InstallerEngine.PayloadNames.Length)
                throw new InvalidOperationException("Incomplete release payload checksum list.");
            BaseGameCatalog.Load();
            var engine = new InstallerEngine(resources, hashes);
            engine.ValidateDependencies();
            return engine;
        }

        internal static SetupResult InstallSelected(string game, Action<string> status)
        {
            GitHubRelease release;
            InstallerEngine engine;
            try
            {
                status("Checking GitHub for the latest stable release.");
                release = GitHubUpdates.Check();
                status("Downloading and verifying GitHub release " + release.Tag + ".");
                engine = CreateEngine(GitHubUpdates.FetchPayload(release));
            }
            catch (System.Net.WebException ex)
            {
                using (ex.Response)
                    throw new IOException("Setup could not reach the release on GitHub (" + ex.Status +
                        "). No game files were changed. Check your connection and try again.", ex);
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException("The GitHub release could not be verified. No game files were changed. " + ex.Message, ex);
            }
            status("Installing screen reader " + release.Tag + ".");
            string source = "GitHub release " + release.Tag + ", ZIP SHA-256 " + release.Digest;
            return new SetupResult(engine.InstallVersion(game, status, release.Version, source), release.Version);
        }
    }

    internal sealed class SetupResult
    {
        public readonly string Backup;
        public readonly Version Version;
        public SetupResult(string backup, Version version)
        {
            Backup = backup;
            Version = version;
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
        private readonly Button safeList = new Button();
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
                "Install downloads the latest stable screen reader release from GitHub. An internet connection is required. Close the game, launcher, and mod manager first. " +
                "Settings and saves are preserved. Use Mod safe list to keep selected DLL and ASI files. Unlisted added modules and legacy screen reader files are backed up and removed. Original game files are checked first. Repair launch redirection is optional and changes the registry only after confirmation and backup." };
            layout.Controls.Add(intro);
            layout.Controls.Add(new Label { Text = "Detected &game installation:", AutoSize = true });
            games.DropDownStyle = ComboBoxStyle.DropDownList;
            games.Dock = DockStyle.Top;
            games.AccessibleName = "Detected game installation";
            games.TabIndex = 0;
            games.SelectedIndexChanged += delegate {
                selected.Text = games.SelectedItem == null ? "" : games.SelectedItem.ToString();
                install.Enabled = repair.Enabled = safeList.Enabled = !worker.IsBusy && selected.Text.Length != 0;
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
            safeList.Text = "Mod safe &list";
            safeList.AccessibleName = "Mod safe list";
            safeList.AutoSize = true;
            safeList.TabIndex = 6;
            safeList.Enabled = false;
            safeList.Click += EditModSafeList;
            buttons.Controls.Add(safeList);
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
                e.Result = Program.InstallSelected((string)e.Argument, text => worker.ReportProgress(0, text));
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
            browse.Enabled = games.Enabled = install.Enabled = cancel.Enabled = diagnose.Enabled = repair.Enabled =
                restore.Enabled = safeList.Enabled = false;
            status.Text = "Preparing setup.";
            worker.RunWorkerAsync(selected.Text);
        }

        private void Finished(object sender, RunWorkerCompletedEventArgs e)
        {
            browse.Enabled = games.Enabled = install.Enabled = cancel.Enabled = diagnose.Enabled = repair.Enabled = restore.Enabled = true;
            safeList.Enabled = selected.Text.Length != 0;
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
            var result = (SetupResult)e.Result;
            ShowMessage("Installation complete for screen reader version " + result.Version + ". Original game files verified. Safe-listed files were left in place and other added DLL/ASI mods were cleaned up. Start your screen reader, then launch normally through Steam. Backups and the install report are in: " + result.Backup, false);
        }

        // Match the packed TASKDIALOGCONFIG in commctrl.h.
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 1)]
        private struct TaskDialogConfig
        {
            public int Size;
            public IntPtr Owner;
            public IntPtr Instance;
            public int Flags;
            public int CommonButtons;
            public string Title;
            public IntPtr MainIcon;
            public string MainInstruction;
            public string Content;
            public int ButtonCount;
            public IntPtr Buttons;
            public int DefaultButton;
            public int RadioButtonCount;
            public IntPtr RadioButtons;
            public int DefaultRadioButton;
            public string VerificationText;
            public string ExpandedInformation;
            public string ExpandedControlText;
            public string CollapsedControlText;
            public IntPtr FooterIcon;
            public string Footer;
            public IntPtr Callback;
            public IntPtr CallbackData;
            public int Width;
        }

        [DllImport("comctl32.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
        private static extern int TaskDialogIndirect(ref TaskDialogConfig config, IntPtr button, IntPtr radioButton, IntPtr verification);

        private static bool TryShowTaskDialog(IWin32Window owner, string text, string title, bool error)
        {
            const int allowDialogCancellation = 0x0008;
            const int okButton = 0x0001;
            const int errorIcon = 0xfffe;
            const int informationIcon = 0xfffd;
            var config = new TaskDialogConfig {
                Size = Marshal.SizeOf(typeof(TaskDialogConfig)),
                Owner = owner.Handle,
                Flags = allowDialogCancellation,
                CommonButtons = okButton,
                Title = title,
                Content = text,
                MainIcon = new IntPtr(error ? errorIcon : informationIcon)
            };
            // Task dialog content keeps literal ampersands in paths and executable names.
            // The setup manifest enables Common Controls 6.
            try { return TaskDialogIndirect(ref config, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero) >= 0; }
            catch (EntryPointNotFoundException) { return false; }
        }

        internal static void ShowSetupDialog(IWin32Window owner, string title, string text, bool error)
        {
            if (!TryShowTaskDialog(owner, text, title, error))
                MessageBox.Show(owner, text, title, MessageBoxButtons.OK,
                    error ? MessageBoxIcon.Error : MessageBoxIcon.Information);
        }

        private void ShowMessage(string text, bool error)
        {
            status.Text = text;
            string title = error ? "Setup could not finish" : "Screen reader setup";
            ShowSetupDialog(this, title, text, error);
            status.Focus();
            status.SelectionStart = 0;
            status.SelectionLength = 0;
        }
    }
}
