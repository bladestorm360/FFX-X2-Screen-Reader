// Offline surrogate for the recovered launcher API. Never starts a game.
using System;
using System.Diagnostics;
using System.Reflection;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace GameLauncher {
    public static class Program {
        public static string CurrentLanguagePath = "us";
        public static bool MetadataUnavailable;
        public static string LanguagePath { get { if (MetadataUnavailable) throw new InvalidOperationException("not ready"); return CurrentLanguagePath; } }
    }
    public class ActiveRect { public bool IsMouseOver; public Rectangle Rect; public MethodInfo OnMouseClick; }
    public class MainForm : Form {
        private ActiveRect[] mActiveRects;
        public int Activations, MoveSounds, SelectSounds;
        public string LastAction;
        public Point LastClick;
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
                LastClick = e.Location;
                LastAction = rectangle.OnMouseClick.Name;
                ++SelectSounds;
                rectangle.OnMouseClick.Invoke(this, new object[0]);
            }
        }
        public void RebuildHotspots(bool small) {
            string[] methods = {"OnFFXClicked", "OnSideStoryClicked", "OnFFX2Clicked", "OnLastMissionClicked", "OnCreditClicked", "OnCredit2014Clicked", "OnConfigClicked"};
            mActiveRects = new ActiveRect[methods.Length];
            for (int i = 0; i < methods.Length; ++i) mActiveRects[i] = new ActiveRect {
                Rect = new Rectangle(small ? 210 : 10, 10 + i * (small ? 25 : 40), small ? 50 : 100, small ? 20 : 30), OnMouseClick = GetType().GetMethod(methods[i]) };
        }
        public MainForm() {
            RebuildHotspots(false);
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
    [DllImport("FFXLauncherNative.dll", CallingConvention = CallingConvention.Cdecl)] static extern int LauncherSpeech_Start();
    [DllImport("FFXLauncherNative.dll", CallingConvention = CallingConvention.Cdecl)] static extern int FfxNative_SpeechReady();
    static readonly string[] Locales = { "us", "jp", "ch", "kr", "de", "fr", "it", "sp" };
    static readonly string[] Actions = { "OnFFXClicked", "OnSideStoryClicked", "OnFFX2Clicked", "OnLastMissionClicked", "OnCreditClicked", "OnCredit2014Clicked", "OnConfigClicked" };
    // Independent expectations transcribed from the approved title artwork,
    // including its intentionally English Japanese Settings and Chinese logos.
    static readonly string[] Settings = { "Setting", "Setting", "\u8a2d\u7f6e", "\uc124\uc815", "Einstellung", "Param\u00e8tres", "Impostazioni", "Configuraci\u00f3n" };
    static readonly string[] Credits = { "Credits & Bonus Audio", "Credits & Bonus Audio", "Credits & Bonus Audio", "Credits & Bonus Audio", "Abspann & Extras", "Cr\u00e9dits & contenu bonus", "Riconoscimenti & Extra", "Cr\u00e9ditos y contenido extra" };
    static readonly string[] Additional = { "Additional Credits", "Additional Credits", "Additional Credits", "Additional Credits", "Weitere Mitwirkende", "Cr\u00e9dits suppl\u00e9mentaires", "Riconoscimenti aggiuntivi", "Cr\u00e9ditos adicionales" };
    static double maxDispatchMs, maxBackendMs, totalDispatchMs, totalBackendMs;
    static int presentations;
    static string Expected(int locale, int row) {
        if (row == 0) return "Final Fantasy X";
        if (row == 1) return locale == 1 ? "Final Fantasy X \u6c38\u9060\u306e\u30ca\u30ae\u7bc0" : "Final Fantasy X Eternal Calm";
        if (row == 2) return "Final Fantasy X-2";
        if (row == 3) return "Final Fantasy X-2 Last Mission";
        return row == 4 ? Credits[locale] : row == 5 ? Additional[locale] : Settings[locale];
    }
    static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    static void WaitSpeech(int before, string expected, string context) {
        DateTime deadline = DateTime.UtcNow.AddSeconds(10);
        while (Test_Count() <= before && DateTime.UtcNow < deadline) Thread.Sleep(1);
        Check(Test_Count() == before + 1, context + ": expected exactly one new speech event");
        StringBuilder text = new StringBuilder(256);
        Test_Read(before, text, 256);
        Check(text.ToString() == expected, context + ": wrong native speech: " + text);
    }
    static void Present(IMessageFilter reader, GameLauncher.MainForm form, int key, string expected, string context) {
        int before = Test_Count();
        Stopwatch watch = Stopwatch.StartNew();
        Check(Key(reader, form, key, false), context + ": key not consumed");
        double dispatch = watch.Elapsed.TotalMilliseconds;
        Check(form.AccessibleName == expected, context + ": wrong accessible label");
        WaitSpeech(before, expected, context);
        double backend = watch.Elapsed.TotalMilliseconds;
        ++presentations; totalDispatchMs += dispatch; totalBackendMs += backend;
        maxDispatchMs = Math.Max(maxDispatchMs, dispatch); maxBackendMs = Math.Max(maxBackendMs, backend);
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
    static void Move(GameLauncher.MainForm form, bool small, int row) {
        var point = new MouseEventArgs(MouseButtons.None, 0, small ? 235 : 60, 10 + row * (small ? 25 : 40) + (small ? 10 : 15), 0);
        typeof(GameLauncher.MainForm).GetMethod("OnMouseMove", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(form, new object[] { point });
    }
    static void EmptyReader(IMessageFilter reader) {
        foreach (string name in new string[] { "mainSelections", "attached" }) {
            object collection = reader.GetType().GetField(name, BindingFlags.Instance | BindingFlags.NonPublic).GetValue(reader);
            Check((int)collection.GetType().GetProperty("Count").GetValue(collection, null) == 0, "disposed form retained by " + name);
        }
    }
    static void NativeDialog(IMessageFilter reader, GameLauncher.MainForm form) {
        int before = Test_Count();
        FormEvent(form, "OnDeactivate"); form.Enabled = false;
        Check(!Key(reader, form, 40, false), "disabled main selector consumed dialog input");
        using (Form settings = new Form()) {
            ComboBox combo = new ComboBox { AccessibleName = "Native combo" };
            combo.Items.AddRange(new object[] { "English", "French" }); combo.SelectedIndex = 0;
            TrackBar slider = new TrackBar { AccessibleName = "Native slider" };
            Button button = new Button { Text = "&OK", AccessibleName = "Native button" };
            settings.Controls.Add(combo); settings.Controls.Add(slider); settings.Controls.Add(button);
            Check(!Key(reader, settings, 40, false) && !Key(reader, settings, 13, false) && !Key(reader, settings, 9, false), "native dialog keys consumed");
            Raise(combo, "OnEnter"); combo.SelectedIndex = 1;
            Raise(slider, "OnEnter"); slider.Value = 4; Raise(button, "OnEnter");
            Check(combo.AccessibleName == "Native combo" && slider.AccessibleName == "Native slider" && button.AccessibleName == "Native button", "native control accessibility overwritten");
        }
        // No producer is scheduled for native controls. Allow the actual worker
        // to run before asserting this negative rather than using absolute counts.
        Thread.Sleep(10);
        Check(Test_Count() == before, "native settings controls produced addon speech");
        form.Enabled = true;
    }
    [STAThread] static int Main(string[] arguments) {
        try {
            Check(AppDomain.CurrentDomain.DomainManager != null && AppDomain.CurrentDomain.DomainManager.GetType().FullName == "FfxLauncher.ReaderDomainManager", "CLR AppDomainManager bootstrap failed");
            Check(Path.GetFullPath(Environment.CurrentDirectory) == Path.GetFullPath(Application.StartupPath), "relative launcher commands have the wrong working directory");
            Assembly adapter = Assembly.LoadFrom("FFXLauncherReader.dll");
            var laa=adapter.GetType("FfxLauncher.LargeAddressAware");
            var status=laa.GetField("state",BindingFlags.Static|BindingFlags.NonPublic);
            var deadline=DateTime.UtcNow.AddSeconds(10);
            while((int)status.GetValue(null)<2 && DateTime.UtcNow<deadline) Thread.Sleep(1);
            Check((int)status.GetValue(null)==2,"valid already-LAA launcher check failed");
            Stopwatch cold = Stopwatch.StartNew();
            Check(LauncherSpeech_Start() != 0, "native speech startup failed");
            deadline=DateTime.UtcNow.AddSeconds(10);
            while(FfxNative_SpeechReady()==0 && DateTime.UtcNow<deadline) Thread.Sleep(1);
            Check(FfxNative_SpeechReady()!=0,"native speech never became ready");
            double readyMs = cold.Elapsed.TotalMilliseconds;
            IMessageFilter reader = (IMessageFilter)Activator.CreateInstance(adapter.GetType("FfxLauncher.UiReader"), true);
            // Run this same bounded path against old/new adapters for a fair
            // English comparison; the old adapter cannot complete Japanese cases.
            if (arguments.Length == 1 && arguments[0] == "--english-latency") {
                using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                    Present(reader, form, 36, Expected(0, 0), "English cold selection");
                    Console.WriteLine("English cold: ready {0:F3} ms, dispatch {1:F3} ms, sampled fake-backend receipt {2:F3} ms", readyMs, maxDispatchMs, maxBackendMs);
                    presentations = 0; maxDispatchMs = maxBackendMs = totalDispatchMs = totalBackendMs = 0;
                    for (int step = 1; step <= 128; ++step) Present(reader, form, 40, Expected(0, step % 7), "English warm selection");
                }
                EmptyReader(reader);
                Console.WriteLine("English warm {0} keys: dispatch mean/max {1:F3}/{2:F3} ms; sampled fake-backend receipt mean/max {3:F3}/{4:F3} ms", presentations, totalDispatchMs / presentations, maxDispatchMs, totalBackendMs / presentations, maxBackendMs);
                return 0;
            }
            GameLauncher.Program.CurrentLanguagePath = "jp";
            using (GameLauncher.MainForm japanese = new GameLauncher.MainForm()) {
                Present(reader, japanese, 40, Expected(1, 1), "Japanese startup");
            }
            EmptyReader(reader);
            GameLauncher.Program.CurrentLanguagePath = "us";
            using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                Present(reader, form, 40, Expected(0, 1), "first queued message");
                Check(form.MoveSounds == 1, "keyboard hover bypassed original move handler");
                Present(reader, form, 36, Expected(0, 0), "home selection");
                Key(reader, form, 13, false); Key(reader, form, 13, true);
                Check(form.Activations == 1, "activation / autorepeat changed original action count");
                Check(form.SelectSounds == 1, "keyboard activation bypassed original select sound");
                Key(reader, form, 32, false); Check(form.Activations == 2, "space activation");
                Check(form.SelectSounds == 2, "space activation sound");
                Present(reader, form, 35, Expected(0, 6), "end selection");
                Present(reader, form, 37, Expected(0, 5), "left selection");
                Present(reader, form, 37, Expected(0, 4), "ampersand preservation");
                Check(form.MoveSounds == 5, "each changed native hover must play its move sound");
                Move(form, false, 4);
                Check(form.MoveSounds == 5, "unchanged hover repeated the native sound");
                Check(!Key(reader, form, 65, false), "unrelated key consumed");
                NativeDialog(reader, form);
                int before = Test_Count();
                FormEvent(form, "OnActivated");
                WaitSpeech(before, Expected(0, 4), "unchanged settings return");
                Check(form.AccessibleName == "Credits & Bonus Audio", "closing dialog lost main selection");
                Present(reader, form, 39, Expected(0, 5), "settings navigation restoration");
            }
            EmptyReader(reader);
            // Every startup locale must attach, speak each actual row and use
            // the original method/sound exactly once for each Enter or Space.
            for (int locale = 0; locale < Locales.Length; ++locale) {
                GameLauncher.Program.CurrentLanguagePath = Locales[locale];
                using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                    for (int row = 0; row < 7; ++row) {
                        Present(reader, form, row == 0 ? 36 : 40, Expected(locale, row), "startup " + Locales[locale] + " row " + row);
                        Key(reader, form, row % 2 == 0 ? 13 : 32, false);
                        Key(reader, form, row % 2 == 0 ? 13 : 32, true);
                        Check(form.LastAction == Actions[row] && form.Activations == row + 1 && form.SelectSounds == row + 1, "original action/sound order or repeat guard");
                    }
                    Present(reader, form, 9, Expected(locale, 0), "Tab wraps");
                    Present(reader, form, 38, Expected(locale, 6), "Up wraps");
                }
                EmptyReader(reader);
            }
            // Native settings reuse MainForm and rebuild its rectangles. Model
            // both event orderings: rebuild before activation, and after modal
            // return/activation as in OnConfigClicked's normal continuation.
            for (int from = 0; from < Locales.Length; ++from) for (int to = 0; to < Locales.Length; ++to) {
                GameLauncher.Program.CurrentLanguagePath = Locales[from];
                using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                    Present(reader, form, 35, Expected(from, 6), "settings origin");
                    NativeDialog(reader, form);
                    bool changedBeforeActivation = (from + to) % 2 == 0;
                    if (changedBeforeActivation) { GameLauncher.Program.CurrentLanguagePath = Locales[to]; form.RebuildHotspots(true); }
                    int before = Test_Count();
                    FormEvent(form, "OnActivated");
                    WaitSpeech(before, Expected(changedBeforeActivation ? to : from, 6), "modal reactivation");
                    if (!changedBeforeActivation) { GameLauncher.Program.CurrentLanguagePath = Locales[to]; form.RebuildHotspots(true); }
                    // Activation without a navigation step must use the NEW
                    // rectangle and retained row, never a cached hover object.
                    Key(reader, form, 13, false);
                    Check(form.LastAction == Actions[6] && form.LastClick == new Point(235, 170), "settings rebuild left stale activation geometry");
                    Check(form.AccessibleName == Expected(to, 6), "activation retained stale language");
                    Present(reader, form, 37, Expected(to, 5), "settings target language");
                    before = Test_Count(); Move(form, true, 1);
                    WaitSpeech(before, Expected(to, 1), "mouse after rebuild");
                    Check(form.AccessibleName == Expected(to, 1), "mouse accessible name stale");
                    int sounds = form.MoveSounds; before = Test_Count();
                    Move(form, true, 1); Check(form.MoveSounds == sounds && Test_Count() == before, "same hover repeated a transition");
                    Present(reader, form, 36, Expected(to, 0), "post-rebuild Home");
                    // Identical valid key events must still speak: no text dedup.
                    Present(reader, form, 36, Expected(to, 0), "repeated Home");
                }
                EmptyReader(reader);
            }
            foreach (string path in new string[] { null, "unknown", "en", "es", "JP" }) {
                GameLauncher.Program.CurrentLanguagePath = path;
                using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                    Present(reader, form, 35, Expected(path == "es" ? 7 : 0, 6), "fallback/alias " + path);
                    GameLauncher.Program.CurrentLanguagePath = "jp";
                    Present(reader, form, 36, Expected(1, 0), "late language readiness");
                    Present(reader, form, 40, Expected(1, 1), "late Japanese label");
                }
                EmptyReader(reader);
            }
            GameLauncher.Program.MetadataUnavailable = true;
            using (GameLauncher.MainForm form = new GameLauncher.MainForm()) {
                Present(reader, form, 40, Expected(0, 1), "throwing language getter fallback");
                GameLauncher.Program.MetadataUnavailable = false;
                Present(reader, form, 36, Expected(1, 0), "language getter recovered");
                Present(reader, form, 40, Expected(1, 1), "recovered Japanese label");
            }
            EmptyReader(reader);
            Console.WriteLine("Launcher fixture passed: Japanese regression, 8 startup locales/all 7 actions, 64 settings transitions, rebuilt geometry, mouse/keyboard/native sounds, modal isolation, aliases/fallback/recovery, repeated speech and disposed-form cleanup.");
            Console.WriteLine("Controlled host only: startup ready {0:F3} ms; {1} key presentations, dispatch mean/max {2:F3}/{3:F3} ms, fake-backend receipt mean/max {4:F3}/{5:F3} ms. Actual launcher/audio/focus latency remains unmeasured.", readyMs, presentations, totalDispatchMs / presentations, maxDispatchMs, totalBackendMs / presentations, maxBackendMs);
            return 0;
        } catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}
