// The original launcher is .NET Framework WinForms. This C++/CLI adapter uses
// its existing UI thread, controls and MethodInfo actions; game code is native.
#using <System.dll>
#using <System.Core.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
#using <System.Xml.dll>
#include <vcclr.h>
#include "../native/large_address_image.hpp"
using namespace System;
using namespace System::Collections::Generic;
using namespace System::Reflection;
using namespace System::IO;
using namespace System::Runtime::InteropServices;
using namespace System::Windows::Forms;

[assembly: AssemblyVersion("1.0.0.0")];

#include "launcher_large_address.hpp"

namespace FfxLauncher {
ref class Speech abstract sealed {
public:
    [DllImport("FFXLauncherNative.dll", CallingConvention = CallingConvention::Cdecl)]
    static int LauncherSpeech_Start();
    [DllImport("FFXLauncherNative.dll", CallingConvention = CallingConvention::Cdecl, CharSet = CharSet::Unicode)]
    static int FfxNative_Output(String^ text, int interrupt);
    [DllImport("FFXLauncherNative.dll", CallingConvention = CallingConvention::Cdecl)]
    static void LauncherSpeech_Capture(int paused);
    static void Pause(bool paused) {
        try { if (LauncherSpeech_Start()) LauncherSpeech_Capture(paused ? 1 : 0); }
        catch (Exception^) {}
    }
    static void Say(String^ text) {
        if (String::IsNullOrWhiteSpace(text)) return;
        try { if (LauncherSpeech_Start()) FfxNative_Output(text, 1); }
        catch (Exception^) {} // speech must not prevent the original launcher starting
    }
};

ref class UiReader sealed : IMessageFilter {
    Dictionary<Form^, int>^ mainSelections;
    HashSet<Control^>^ attached;
    Dictionary<String^, String^>^ labels;
    static Object^ Field(Object^ target, String^ name) {
        auto field = target->GetType()->GetField(name, BindingFlags::Instance | BindingFlags::Public | BindingFlags::NonPublic);
        return field == nullptr ? nullptr : field->GetValue(target);
    }
    static Array^ Rects(Form^ form) { return dynamic_cast<Array^>(Field(form, "mActiveRects")); }
    static String^ Action(Object^ rectangle) {
        auto method = dynamic_cast<MethodInfo^>(Field(rectangle, "OnMouseClick"));
        return method == nullptr ? "" : method->Name;
    }
    String^ Choice(Object^ rectangle) {
        String^ result;
        return labels->TryGetValue(Action(rectangle), result) ? result : "";
    }
    static MouseEventArgs^ MouseAt(Object^ rectangle, MouseButtons button, int clicks) {
        auto bounds = safe_cast<System::Drawing::Rectangle>(Field(rectangle, "Rect"));
        return gcnew MouseEventArgs(button, clicks, bounds.Left + bounds.Width / 2,
            bounds.Top + bounds.Height / 2, 0);
    }
    static void MouseEvent(Form^ form, String^ name, MouseEventArgs^ event) {
        auto method = form->GetType()->GetMethod(name,
            BindingFlags::Instance | BindingFlags::NonPublic, nullptr,
            gcnew array<Type^>{MouseEventArgs::typeid}, nullptr);
        if (method == nullptr) throw gcnew MissingMethodException(name);
        method->Invoke(form, gcnew array<Object^>{event});
    }
    void Select(Form^ form, int index, bool announce) {
        auto rectangles = Rects(form);
        if (rectangles == nullptr || rectangles->Length == 0) return;
        index = (index % rectangles->Length + rectangles->Length) % rectangles->Length;
        mainSelections[form] = index;
        if (announce) {
            // Preserve the original hover transition, FMOD move sound and mute/volume.
            // Store selection first so the resulting MouseMove event does not speak twice.
            MouseEvent(form, "OnMouseMove", MouseAt(rectangles->GetValue(index), MouseButtons::None, 0));
        } else {
            for (int i = 0; i < rectangles->Length; ++i) {
                auto rectangle = rectangles->GetValue(i);
                auto hover = rectangle->GetType()->GetField("IsMouseOver");
                if (hover != nullptr) hover->SetValue(rectangle, i == index);
            }
        }
        form->AccessibleName = Choice(rectangles->GetValue(index));
        form->Invalidate();
        if (announce) Speech::Say(form->AccessibleName);
    }
    void ActivateChoice(Form^ form) {
        auto rectangles = Rects(form);
        if (rectangles == nullptr || !mainSelections->ContainsKey(form)) return;
        const int index = mainSelections[form];
        if (index < 0 || index >= rectangles->Length) return;
        if(Action(rectangles->GetValue(index))=="OnFFXClicked") {
            String^ notice;const bool allowed=LargeAddressAware::AllowLaunch(notice);
            Speech::Say(notice);if(!allowed) return;
        }
        // Raising the native click event includes the select sound, original command
        // arguments and RunExeAndExit's close-after-sound behavior for games/movies.
        Select(form, index, false);
        MouseEvent(form, "OnMouseClick", MouseAt(rectangles->GetValue(index), MouseButtons::Left, 1));
        if (!form->IsDisposed) form->Invalidate();
    }
    void MainActivated(Object^ sender, EventArgs^) {
        auto form = safe_cast<Form^>(sender);
        if (!form->Enabled) return;
        Speech::Pause(false);
        Select(form, mainSelections->ContainsKey(form) ? mainSelections[form] : 0, true);
    }
    void MainDeactivated(Object^, EventArgs^) { Speech::Pause(true); }
    void MouseMoved(Object^ sender, MouseEventArgs^) {
        auto form = safe_cast<Form^>(sender);
        if (!form->Enabled) return;
        auto rectangles = Rects(form);
        if (rectangles == nullptr) return;
        // MainForm.OnMouseMove updates IsMouseOver before raising this event.
        for (int i = 0; i < rectangles->Length; ++i) {
            if (safe_cast<bool>(Field(rectangles->GetValue(i), "IsMouseOver")) &&
                (!mainSelections->ContainsKey(form) || mainSelections[form] != i)) {
                mainSelections[form] = i;
                Speech::Say(Choice(rectangles->GetValue(i)));
                break;
            }
        }
    }
    void Removed(Object^ sender, EventArgs^) {
        auto control = safe_cast<Control^>(sender);
        attached->Remove(control);
        auto form = dynamic_cast<Form^>(control);
        if (form != nullptr) mainSelections->Remove(form);
    }
    void Attach(Form^ form) {
        // Native dialogs already expose their controls to the screen reader.
        // Only attach the image-based selector, whose activation restores speech.
        if (form == nullptr || form->GetType()->FullName != "GameLauncher.MainForm" || !attached->Add(form)) return;
        form->Disposed += gcnew EventHandler(this, &UiReader::Removed);
        // English hotspot labels were verified against us/titlemenu.png.
        // Other language image labels have not been verified yet.
        auto program = form->GetType()->Assembly->GetType("GameLauncher.Program");
        auto language = program->GetProperty("LanguagePath", BindingFlags::Static | BindingFlags::Public);
        String^ path = language == nullptr ? "" : safe_cast<String^>(language->GetValue(nullptr, nullptr));
        if (path == "us" || path == "en") {
            mainSelections[form] = 0;
            form->Activated += gcnew EventHandler(this, &UiReader::MainActivated);
            form->Deactivate += gcnew EventHandler(this, &UiReader::MainDeactivated);
            form->MouseMove += gcnew MouseEventHandler(this, &UiReader::MouseMoved);
            Select(form, 0, form->ContainsFocus);
        }
    }
public:
    UiReader() {
        mainSelections = gcnew Dictionary<Form^, int>();
        attached = gcnew HashSet<Control^>();
        labels = gcnew Dictionary<String^, String^>();
        labels["OnFFXClicked"] = "Final Fantasy X";
        labels["OnSideStoryClicked"] = "Final Fantasy X Eternal Calm";
        labels["OnFFX2Clicked"] = "Final Fantasy X-2";
        labels["OnLastMissionClicked"] = "Final Fantasy X-2 Last Mission";
        labels["OnCreditClicked"] = "Credits & Bonus Audio";
        labels["OnCredit2014Clicked"] = "Additional Credits";
        labels["OnConfigClicked"] = "Setting";
    }
    virtual bool PreFilterMessage(Message% message) {
        try {
            auto control = Control::FromHandle(message.HWnd);
            if (control == nullptr) control = Control::FromChildHandle(message.HWnd);
            auto form = control == nullptr ? nullptr : control->FindForm();
            // Sent focus/show messages can bypass IMessageFilter. Attach on
            // the first queued message for a form, then use its actual events.
            Attach(form);
            if(form!=nullptr && form->Enabled && form->GetType()->FullName=="GameLauncher.MainForm" &&
                (message.Msg==0x201 || message.Msg==0x202)) {
                const int raw=message.LParam.ToInt32();
                const auto point=form->PointToClient(control->PointToScreen(System::Drawing::Point(static_cast<short>(raw),static_cast<short>(raw>>16))));
                auto rectangles=Rects(form);
                if(rectangles!=nullptr) for each(Object^ rectangle in rectangles) {
                    auto bounds=safe_cast<System::Drawing::Rectangle>(Field(rectangle,"Rect"));
                    if(bounds.Contains(point) && Action(rectangle)=="OnFFXClicked") {
                        String^ notice;const bool allowed=LargeAddressAware::AllowLaunch(notice);
                        Speech::Say(notice);if(!allowed) return true;
                    }
                }
            }
            if (message.Msg != 0x100 || form == nullptr || !form->Enabled || !mainSelections->ContainsKey(form)) return false;
            // Only the original main selector consumes these keys. A modal
            // settings/credits form retains its normal WinForms input path.
            const int key = message.WParam.ToInt32();
            int index = mainSelections[form];
            if (key == 38 || key == 37) Select(form, index - 1, true);
            else if (key == 40 || key == 39) Select(form, index + 1, true);
            else if (key == 9) Select(form, index + ((Control::ModifierKeys & Keys::Shift) == Keys::Shift ? -1 : 1), true);
            else if (key == 36) Select(form, 0, true);
            else if (key == 35) Select(form, -1, true);
            else if (key == 13 || key == 32) {
                if ((message.LParam.ToInt64() & 0x40000000) == 0) ActivateChoice(form);
            }
            else return false;
            return true;
        } catch (Exception^) { return false; }
    }
};

public ref class ReaderDomainManager sealed : AppDomainManager {
public:
    virtual void InitializeNewDomain(AppDomainSetup^ setup) override {
        AppDomainManager::InitializeNewDomain(setup);
        try {
            // The stock ShellExecute call passes no working directory. An external
            // launcher start must still resolve the original relative game commands.
            Directory::SetCurrentDirectory(Application::StartupPath);
            LargeAddressAware::Begin(Application::StartupPath);
            Application::AddMessageFilter(gcnew UiReader());
        }
        catch (Exception^) {} // CLR/launcher startup remains usable on failure
    }
};
}
