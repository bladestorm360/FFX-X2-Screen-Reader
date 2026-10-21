using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Security.Principal;
using System.Windows.Forms;

namespace FfxScreenReader.Setup
{
    internal sealed partial class InstallerForm
    {
        private readonly Button repair = new Button();
        private readonly Button restore = new Button();
        private void AddRegistryButtons(FlowLayoutPanel buttons) {
            repair.Text = "&Repair launch redirection";
            restore.Text = "Restore launch bac&kup";
            repair.AccessibleName = "Repair launch redirection";
            restore.AccessibleName = "Restore launch backup";
            repair.TabIndex = 4; restore.TabIndex = 5;
            repair.AutoSize = restore.AutoSize = true;
            repair.Enabled = false;
            buttons.Controls.Add(repair); buttons.Controls.Add(restore);
            repair.Click += delegate { RepairRedirects(); };
            restore.Click += delegate { RestoreRedirects(); };
        }
        private static Form RegistryConfirmation(string title, string explanation, IList<LaunchRedirectRepair.Entry> entries) {
            var dialog = new Form { Text = title, AccessibleName = title, Size = new Size(760, 520),
                MinimumSize = new Size(620, 380), Font = SystemFonts.MessageBoxFont, StartPosition = FormStartPosition.CenterParent };
                var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(12), ColumnCount = 1, RowCount = 2 };
                layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
                var text = new TextBox { AccessibleName = "Registry changes to review", Multiline = true, ReadOnly = true,
                    ScrollBars = ScrollBars.Both, Dock = DockStyle.Fill, TabIndex = 0, Text = explanation + "\r\n\r\n" + LaunchRedirects.EntriesText(entries) };
                var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true };
                var yes = new Button { Text = "&Confirm registry change", AccessibleName = "Confirm registry change", AutoSize = true, DialogResult = DialogResult.Yes, TabIndex = 0 };
                var no = new Button { Text = "&Cancel", AccessibleName = "Cancel registry change", AutoSize = true, DialogResult = DialogResult.Cancel, TabIndex = 1 };
                buttons.Controls.Add(yes); buttons.Controls.Add(no); layout.Controls.Add(text); layout.Controls.Add(buttons); dialog.Controls.Add(layout);
                dialog.CancelButton = no; // Enter in the review text cannot authorize a registry write.
                dialog.Shown += delegate { text.Focus(); text.SelectionStart = 0; text.SelectionLength = 0; };
                return dialog;
        }
        private bool ConfirmRegistry(string title, string explanation, IList<LaunchRedirectRepair.Entry> entries) {
            using (var dialog = RegistryConfirmation(title, explanation, entries)) return dialog.ShowDialog(this) == DialogResult.Yes;
        }
        private bool Administrator(string action) {
            using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
                if (new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator)) return true;
            status.Text = "Launch registry repair requires administrator permission. Windows will request permission and reopen setup. You must still review and confirm the registry changes.";
            var start = new ProcessStartInfo(Assembly.GetExecutingAssembly().Location,
                "--game \"" + selected.Text.TrimEnd('\\') + "\" --preview-" + action) { UseShellExecute = true, Verb = "runas" };
            using (Process elevated = Process.Start(start)) { }
            Close();
            return false;
        }
        private void RepairRedirects() {
            if (worker.IsBusy || selected.Text.Length == 0) return;
            try {
                InstallerEngine.RequireClosedProcesses();
                var entries = LaunchRedirectRepair.Read(selected.Text);
                if (entries.Count == 0) { ShowMessage("No FFX Debugger launch redirection was found. No registry values were changed. Choose Install to install or update the mod.", false); return; }
                if (!Administrator("repair")) return;
                if (!ConfirmRegistry("Repair FFX launch redirection", "This changes the Windows registry. It removes only the listed Debugger values for FFX executables so they can launch normally. A complete XML backup is saved first. Other registry values and keys are preserved. Close the game, launcher, and previous mod manager before confirming.", entries)) return;
                using (var save = new SaveFileDialog { Title = "Save launch redirection backup", Filter = "Launch redirection backup (*.xml)|*.xml", AddExtension = true,
                    DefaultExt = "xml", FileName = "FFX_Launch_Backup-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".xml",
                    InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) }) {
                    if (save.ShowDialog(this) != DialogResult.OK) return;
                    InstallerEngine.RequireClosedProcesses();
                    int count = LaunchRedirectRepair.Repair(selected.Text, entries, save.FileName);
                    ShowMessage("Launch redirection repaired: " + count + " Debugger values removed. Backup: " + save.FileName + ". Choose Install, then launch through Steam. Restore launch backup can undo this repair.", false);
                }
            } catch (System.ComponentModel.Win32Exception ex) {
                ShowMessage(ex.NativeErrorCode == 1223 ? "Administrator request canceled. No registry values were changed." : ex.Message, true);
            } catch (Exception ex) { ShowMessage(ex.Message, true); }
        }
        private void RestoreRedirects() {
            if (worker.IsBusy) return;
            try {
                InstallerEngine.RequireClosedProcesses();
                if (!Administrator("restore")) return;
                using (var open = new OpenFileDialog { Title = "Select launch redirection backup", Filter = "Launch redirection backup (*.xml)|*.xml", CheckFileExists = true }) {
                    if (open.ShowDialog(this) != DialogResult.OK) return;
                    var entries = LaunchRedirectRepair.ReadBackup(open.FileName);
                    if (!ConfirmRegistry("Restore FFX launch backup", "This changes the Windows registry by restoring the listed Debugger values from your backup. It can re-enable the previous mod's custom launcher. Existing Debugger values will not be overwritten.", entries)) return;
                    InstallerEngine.RequireClosedProcesses();
                    int count = LaunchRedirectRepair.Restore(open.FileName, entries);
                    ShowMessage("Launch backup restored: " + count + " Debugger values. Registry backup: " + open.FileName, false);
                }
            } catch (System.ComponentModel.Win32Exception ex) {
                ShowMessage(ex.NativeErrorCode == 1223 ? "Administrator request canceled. No registry values were changed." : ex.Message, true);
            } catch (Exception ex) { ShowMessage(ex.Message, true); }
        }
    }
}
