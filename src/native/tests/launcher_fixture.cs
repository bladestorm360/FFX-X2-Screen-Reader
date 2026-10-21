// Offline surrogate for the recovered launcher API. Never starts a game.
using System;
using System.Reflection;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace GameLauncher {
    public static class Program { public static string LanguagePath { get { return "us"; } } }
    public class ActiveRect { public bool IsMouseOver; public Rectangle Rect; public MethodInfo OnMouseClick; }
    public class MainForm : Form {
        private ActiveRect[] mActiveRects;
        public int Activations, MoveSounds, SelectSounds;
        protected override void OnMouseMove(MouseEventArgs e) {
            bool entered = false;
            foreach (ActiveRect rectangle in mActiveRects) {
                bool over = rectangle.Rect.Contains(e.Location);
                if (over && !rectangle.IsMouseOver) entered = true;
                rectangle.IsMouseOver = over;
            }
            if (entered) ++MoveSounds;
            base.OnMouseMove(e);
        }
        private void NativeMouseClick(object sender, MouseEventArgs e) {
            foreach (ActiveRect rectangle in mActiveRects) if (rectangle.IsMouseOver) {
                ++SelectSounds;
                rectangle.OnMouseClick.Invoke(this, new object[0]);
            }
        }
        public MainForm() {
            string[] methods = {"OnFFXClicked", "OnSideStoryClicked", "OnFFX2Clicked", "OnLastMissionClicked", "OnCreditClicked", "OnCredit2014Clicked", "OnConfigClicked"};
            mActiveRects = new ActiveRect[methods.Length];
            for (int i = 0; i < methods.Length; ++i) mActiveRects[i] = new ActiveRect {
                Rect = new Rectangle(10, 10 + i * 40, 100, 30), OnMouseClick = GetType().GetMethod(methods[i]) };
            MouseClick += NativeMouseClick;
        }
        public void OnFFXClicked() { ++Activations; }
        public void OnSideStoryClicked() { ++Activations; }
        public void OnFFX2Clicked() { ++Activations; }
        public void OnLastMissionClicked() { ++Activations; }
        public void OnCreditClicked() { ++Activations; }
        public void OnCredit2014Clicked() { ++Activations; }
        public void OnConfigClicked() { ++Activations; }
    }
}
class Fixture {
    [DllImport("PrismRuntime/prism.dll", CallingConvention = CallingConvention.Cdecl)] static extern int Test_Count();
    [DllImport("PrismRuntime/prism.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)] static extern uint Test_Read(int index, StringBuilder text, uint length);
    static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    static void WaitCount(int count) {
        DateTime deadline = DateTime.UtcNow.AddSeconds(10);
        while (Test_Count() < count && DateTime.UtcNow < deadline) Thread.Sleep(1);
        Check(Test_Count() == count, "launcher native speech queue");
    }
    static void Raise(Control control, string name) {
        typeof(Control).GetMethod(name, BindingFlags.Instance | BindingFlags.NonPublic).Invoke(control, new object[] { EventArgs.Empty });
    }
    static void FormEvent(Form form, string name) {
        typeof(Form).GetMethod(name, BindingFlags.Instance | BindingFlags.NonPublic).Invoke(form, new object[] { EventArgs.Empty });
    }
    static bool Key(IMessageFilter reader, Form form, int key, bool repeat) {
        Message message = Message.Create(form.Handle, 0x100, new IntPtr(key), new IntPtr(repeat ? 0x40000000 : 0));
        return reader.PreFilterMessage(ref message);
    }
    [STAThread] static int Main() {
        try {
            Check(AppDomain.CurrentDomain.DomainManager != null && AppDomain.CurrentDomain.DomainManager.GetType().FullName == "FfxLauncher.ReaderDomainManager", "CLR AppDomainManager bootstrap failed");
            Check(Path.GetFullPath(Environment.CurrentDirectory) == Path.GetFullPath(Application.StartupPath), "relative launcher commands have the wrong working directory");
            Assembly adapter = Assembly.LoadFrom("FFXLauncherReader.dll");
            var laa=adapter.GetType("FfxLauncher.LargeAddressAware");
            var status=laa.GetField("state",BindingFlags.Static|BindingFlags.NonPublic);
            var deadline=DateTime.UtcNow.AddSeconds(10);
            while((int)status.GetValue(null)<2 && DateTime.UtcNow<deadline) Thread.Sleep(1);
            Check((int)status.GetValue(null)==2,"valid already-LAA launcher check failed");
            IMessageFilter reader = (IMessageFilter)Activator.CreateInstance(adapter.GetType("FfxLauncher.UiReader"), true);
            using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                Check(Key(reader, form, 40, false), "first queued message did not attach");
                Check(form.AccessibleName == "Final Fantasy X Eternal Calm", "native hotspot order");
                Check(form.MoveSounds == 1, "keyboard hover bypassed original move handler");
                Key(reader, form, 36, false);
                Check(form.AccessibleName == "Final Fantasy X", "home selection");
                Key(reader, form, 13, false); Key(reader, form, 13, true);
                Check(form.Activations == 1, "activation / autorepeat changed original action count");
                Check(form.SelectSounds == 1, "keyboard activation bypassed original select sound");
                Key(reader, form, 32, false); Check(form.Activations == 2, "space activation");
                Check(form.SelectSounds == 2, "space activation sound");
                Key(reader, form, 35, false); Check(form.AccessibleName == "Setting", "end selection");
                Key(reader, form, 37, false); Key(reader, form, 37, false);
                Check(form.AccessibleName == "Credits & Bonus Audio", "ampersand lost from image label");
                Check(form.MoveSounds == 5, "each changed native hover must play its move sound");
                var samePoint = new MouseEventArgs(MouseButtons.None, 0, 60, 185, 0);
                typeof(GameLauncher.MainForm).GetMethod("OnMouseMove", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(form, new object[] { samePoint });
                Check(form.MoveSounds == 5, "unchanged hover repeated the native sound");
                Check(!Key(reader, form, 65, false), "unrelated key consumed");
                WaitCount(5);
                FormEvent(form, "OnDeactivate");
                form.Enabled = false;
                Check(!Key(reader, form, 40, false), "disabled main selector consumed dialog input");
                Check(form.AccessibleName == "Credits & Bonus Audio", "dialog changed preserved main selection");
                using (Form settings = new Form()) {
                    Check(!Key(reader, settings, 40, false), "settings navigation consumed by main selector");
                    Label label = new Label { Name = "label2", Text = "Text language" };
                    ComboBox combo = new ComboBox { Name = "comboBox1", AccessibleName = "Native combo" };
                    combo.Items.AddRange(new object[] { "English", "French" }); combo.SelectedIndex = 0;
                    TrackBar slider = new TrackBar { Name = "trackBar1", AccessibleName = "Native slider" };
                    Button button = new Button { Text = "&OK", AccessibleName = "Native button" };
                    settings.Controls.Add(label); settings.Controls.Add(combo); settings.Controls.Add(slider); settings.Controls.Add(button);
                    Key(reader, settings, 65, false);
                    Raise(combo, "OnEnter"); combo.SelectedIndex = 1;
                    Raise(slider, "OnEnter"); slider.Value = 4;
                    Raise(button, "OnEnter");
                    Check(combo.AccessibleName == "Native combo" && slider.AccessibleName == "Native slider" && button.AccessibleName == "Native button", "native control accessibility was overwritten");
                    Check(Test_Count() == 5, "settings controls produced addon speech");
                    Check(!Key(reader, settings, 13, false) && !Key(reader, settings, 9, false), "native dialog keys consumed");
                }
                form.Enabled = true;
                FormEvent(form, "OnActivated");
                WaitCount(6);
                Check(form.AccessibleName == "Credits & Bonus Audio", "closing dialog lost main selection");
                Check(Key(reader, form, 39, false), "main selector did not resume after dialog closed");
                WaitCount(7);
                Check(form.AccessibleName == "Additional Credits", "restored custom navigation changed hotspot order");
            }
            WaitCount(7);
            StringBuilder text = new StringBuilder(256);
            Test_Read(Test_Count() - 1, text, 256);
            Check(text.ToString() == "Additional Credits", "custom menu speech did not resume");
            Console.WriteLine("Launcher fixture passed: CLR bootstrap, original hotspot order/actions/sounds, native dialog isolation and custom speech/navigation restoration.");
            return 0;
        } catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}
