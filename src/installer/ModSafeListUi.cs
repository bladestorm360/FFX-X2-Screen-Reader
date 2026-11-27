using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace FfxScreenReader.Setup
{
    internal sealed partial class InstallerForm
    {
        private void EditModSafeList(object sender, EventArgs e)
        {
            if (worker.IsBusy || selected.Text.Length == 0) return;
            try
            {
                var stock = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (BaseGameCatalog catalog in BaseGameCatalog.Load()) stock.UnionWith(catalog.Files.Keys);
                using (var dialog = new ModSafeListDialog(selected.Text, stock))
                    if (dialog.ShowDialog(this) == DialogResult.OK)
                        ShowMessage("Mod safe list saved for the selected game folder.", false);
            }
            catch (Exception ex) { ShowMessage(ex.Message, true); }
        }
    }

    internal sealed class ModSafeListDialog : Form
    {
        private readonly string game;
        private readonly ICollection<string> stock;
        private readonly HashSet<string> original;
        private readonly ListBox files = new ListBox();
        private readonly Button remove = new Button();

        public ModSafeListDialog(string game, ICollection<string> stock)
        {
            this.game = game;
            this.stock = stock;
            original = ModSafeList.Read(game);
            Text = AccessibleName = "Mod safe list";
            Font = SystemFonts.MessageBoxFont;
            AutoScaleMode = AutoScaleMode.Font;
            StartPosition = FormStartPosition.CenterParent;
            MinimumSize = new Size(540, 330);
            Size = new Size(690, 420);
            ShowInTaskbar = false;

            var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, Padding = new Padding(12) };
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.Controls.Add(new Label { AutoSize = true, MaximumSize = new Size(495, 0), UseMnemonic = false,
                Text = "Add each loader and DLL or ASI plugin you want setup to keep. Paths are relative to the selected game folder. Save applies your changes." });
            files.AccessibleName = "Protected DLL and ASI files";
            files.Dock = DockStyle.Fill;
            files.HorizontalScrollbar = true;
            files.SelectionMode = SelectionMode.MultiExtended;
            files.Sorted = true;
            files.TabIndex = 0;
            foreach (string name in original) files.Items.Add(name);
            layout.Controls.Add(files);

            var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, TabIndex = 1 };
            var add = new Button { Text = "&Add files...", AccessibleName = "Add files", AutoSize = true, TabIndex = 0 };
            remove.Text = "&Remove selected";
            remove.AccessibleName = "Remove selected";
            remove.AutoSize = true;
            remove.TabIndex = 1;
            remove.Enabled = false;
            var save = new Button { Text = "&Save", AccessibleName = "Save", AutoSize = true, TabIndex = 2 };
            var cancel = new Button { Text = "&Cancel", AccessibleName = "Cancel", AutoSize = true, TabIndex = 3, DialogResult = DialogResult.Cancel };
            buttons.Controls.AddRange(new Control[] { add, remove, save, cancel });
            layout.Controls.Add(buttons);
            Controls.Add(layout);
            CancelButton = cancel;
            files.SelectedIndexChanged += delegate { remove.Enabled = files.SelectedIndices.Count != 0; };
            add.Click += delegate { AddFiles(); };
            remove.Click += delegate { RemoveFiles(); };
            save.Click += delegate { Save(); };
            Shown += delegate { if (files.Items.Count == 0) add.Focus(); else files.Focus(); };
        }

        private HashSet<string> CurrentEntries()
        {
            var entries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string name in files.Items) entries.Add(name);
            return entries;
        }

        private void AddFiles()
        {
            try
            {
                using (var picker = new OpenFileDialog { Title = "Select DLL and ASI files to keep",
                    Filter = "DLL and ASI files (*.dll;*.asi)|*.dll;*.asi", InitialDirectory = game,
                    CheckFileExists = true, DereferenceLinks = false, Multiselect = true, RestoreDirectory = true })
                {
                    if (picker.ShowDialog(this) != DialogResult.OK) return;
                    var selectedEntries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    foreach (string path in picker.FileNames) selectedEntries.Add(ModSafeList.SelectFile(game, path, stock));
                    var existing = CurrentEntries();
                    foreach (string entry in selectedEntries)
                        if (existing.Add(entry)) files.Items.Add(entry);
                    files.ClearSelected();
                    for (int index = 0; index < files.Items.Count; index++)
                        if (selectedEntries.Contains((string)files.Items[index])) files.SetSelected(index, true);
                    files.Focus();
                }
            }
            catch (Exception ex) { InstallerForm.ShowSetupDialog(this, "Could not add files", ex.Message, true); }
        }

        private void RemoveFiles()
        {
            if (files.SelectedIndices.Count == 0) return;
            var selectedIndices = new List<int>();
            foreach (int index in files.SelectedIndices) selectedIndices.Add(index);
            for (int index = selectedIndices.Count - 1; index >= 0; index--)
                files.Items.RemoveAt(selectedIndices[index]);
            files.ClearSelected();
            if (files.Items.Count > 0) files.SelectedIndex = Math.Min(selectedIndices[0], files.Items.Count - 1);
            remove.Enabled = files.SelectedIndices.Count != 0;
            files.Focus();
        }

        private void Save()
        {
            try
            {
                ModSafeList.Save(game, original, CurrentEntries(), stock);
                DialogResult = DialogResult.OK;
            }
            catch (Exception ex) { InstallerForm.ShowSetupDialog(this, "Could not save mod safe list", ex.Message, true); }
        }
    }
}
