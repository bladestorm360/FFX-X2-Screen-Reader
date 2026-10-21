# Native DirectInput ownership conversion

Status: source implemented and affected offline checks pass; canonical x86 candidate build passes; product undeployed. Overall
strict threading remains NONCONFORMING.

Existing archived PInputDeviceKeyboard::Update logical22D770 and DirectInput
gamepad Update22DB80 read their live COM device at this+14. XInput22DD50 is a
different backend and is not a COM discovery event. The verified native PInput
owner225930 precedes these calls. Windows SDK IDirectInputDevice8 has32 vtable
entries: Release2, GetDeviceState9 and GetDeviceData10, all stdcall. Native
discovery previously called GetCapabilities and MH_ApplyQueued inline; the
MinHook1.3.4 enable path suspends other threads. Moving that enable call to a
worker would still suspend the game, so it is not the chosen solution.

Use32 preallocated process-lifetime table adapters, keyed by copied original
vtable address and verified keyboard/pad kind. On the actual native update,
copy exactly132 ready bytes (interface adjustment plus32 methods), replace three entries with statically compiled
stdcall adapters, then change only that live device's vtable pointer. This is
bounded approved input interception; no allocation, COM query, worker, native
wait, late code patch or hook enable occurs. Other objects sharing the original
vtable, including SDL devices, retain their original methods. Adapter functions
carry their static slot identity; they do not dereference an object after its
Release or depend on a later vtable to recover the original function. Forward
all32 methods and native return/LastError; postprocessing is limited to the
verified game owner. Original exceptions propagate. Copied function addresses
remain valid for the live native COM method call; no COM pointer enters a worker.

The first isolated Windows COM fixture exposed an access violation in original
Release. Offline x86 SysWOW64/dinput8.dll SHA256
2a746ccf3922aecced0387f3c6e21e55c6df786df8164c482f2a2e6165edd395
RVA5B60 reads vtable[-1] at5B83 to adjust the interface pointer before5B87
reads the aggregate owner. Preserve that native prefix in the adapted table;
the32-entry SDK method layout alone is insufficient. Add this requirement to
the fake ABI fixture as well as retesting the actual unacquired COM Release.

Slots are immutable after first binding and never recycled while code may call
them. Exhaustion preserves original input. Keyboard Release0 clears only its
token; native owner resets held state on the next update. Reconnection/table
changes rebind from the live owner event. Keyboard fallback and pad neutral
suppression retain their existing semantics. Mod key dispatch, listener bridge,
speech and other reader locks remain separate unresolved threading work.

Acceptance: synthetic exact-ABI forwarding/LastError/input/focus/release and
foreign-device isolation, actual unacquired Windows keyboard instance-GUID
discovery through copied PInput owner fixture, no capabilities/hook-enabling
call from native discovery, affected input/controller fixtures and canonical
build. Game/runtime confirmation remains user-side.

## Independently verified checks

Canonical --input passes keyboard/modifier/context/Peek/menu/NVDA guards,64
distinct equal queued commands, pressure native handback and availability
retirement. The actual unacquired DirectInput keyboard fixture passes owner
binding and final Release with the preserved prefix (one keyboard, zero
distinct-from-SysKeyboard instance GUIDs in this run). The x86 W4/WX fake COM
fixture independently verifies all untouched32 methods, calling conventions,
original return/LastError, failed data, foreign caller bypass, original exception,
saved adapter after a chained vtable change, final object free, prefix, finite
capacity and no GetCapabilities or late hook registration. No game launch,
keyboard acquisition, physical injection, real speech or performance probe.

input.cpp now has fixed bootstrap-only binding arrays and a64-slot copied key
queue. One native try-publication returns failure/handback on pressure; native
input consumes a bounded FIFO and availability uses an atomic revision. Message
held state belongs only to the verified native owner. No input/message queue
mutex remains. Dispatch callbacks still perform substantial reader/navigation
work and speech still contains locks; these are not certified conforming.

Luna gpt-6-luna medium returned the two specified fixture transformations in
workspace work without compiling. Primary corrected synthetic COM object
indirection and restored the existing recovery call, then independently ran
the full input/real COM/ABI checks. Underlying Windows prefix discovery and
production adapter integration remained Primary work. All checks listed above
pass; product build/deployment and gameplay remain separate.
