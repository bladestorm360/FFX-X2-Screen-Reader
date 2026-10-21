// @desc Hook DirectInput8 — trace actual input from keyboard and gamepad
//
// The game imports DirectInput8Create from dinput8.dll. XInput reads all zeros.
// DirectInput is the real input path. We hook:
//   1. DirectInput8Create → capture IDirectInput8 pointer
//   2. IDirectInput8::CreateDevice (vtable[3]) → capture IDirectInputDevice8 pointer
//   3. IDirectInputDevice8::GetDeviceState (vtable[9]) → read input buffer after each call
//
// Console: ONLY when new key/button pressed (first occurrence per key)
// File: full buffer dumps
//
// Press UP/DOWN on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/hook_dinput.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] hook_dinput');
logBoth('[*] base=0x' + base.toString(16));

// ── Find DirectInput8Create in dinput8.dll ──
var dinputMod = null;
var modules = Process.enumerateModules();
for (var i = 0; i < modules.length; i++) {
    if (modules[i].name.toLowerCase() === 'dinput8.dll') {
        dinputMod = modules[i];
        break;
    }
}

if (!dinputMod) {
    logBoth('[!] dinput8.dll not found');
} else {
    var exports = dinputMod.enumerateExports();
    var diCreateAddr = null;
    for (var j = 0; j < exports.length; j++) {
        if (exports[j].name === 'DirectInput8Create') {
            diCreateAddr = exports[j].address;
            break;
        }
    }

    if (!diCreateAddr) {
        logBoth('[!] DirectInput8Create not found in exports');
    } else {
        logBoth('[*] DirectInput8Create at 0x' + diCreateAddr.toString(16));

        // Hook DirectInput8Create to get the IDirectInput8 interface
        // HRESULT DirectInput8Create(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN)
        // ppvOut is arg[3] — pointer to receive the IDirectInput8 pointer
        Interceptor.attach(diCreateAddr, {
            onEnter: function(args) {
                this.ppvOut = args[3]; // LPVOID* — output pointer
            },
            onLeave: function(retval) {
                if (retval.toInt32() === 0 && this.ppvOut) { // S_OK
                    var diObj = this.ppvOut.readPointer();
                    logBoth('[DI8] IDirectInput8 created: 0x' + diObj.toString(16));
                    hookCreateDevice(diObj);
                }
            }
        });
        logBoth('[OK] DirectInput8Create hooked');
    }
}

// ── Hook IDirectInput8::CreateDevice (vtable index 3) ──
function hookCreateDevice(diObj) {
    // IDirectInput8 vtable: QueryInterface(0), AddRef(1), Release(2), CreateDevice(3)
    var vtable = diObj.readPointer();
    var createDeviceAddr = vtable.add(3 * 4).readPointer();
    logBoth('[DI8] CreateDevice at 0x' + createDeviceAddr.toString(16));

    Interceptor.attach(createDeviceAddr, {
        onEnter: function(args) {
            // CreateDevice(this, rguid, lplpDirectInputDevice, pUnkOuter)
            this.ppDevice = args[2]; // IDirectInputDevice8** output
        },
        onLeave: function(retval) {
            if (retval.toInt32() === 0 && this.ppDevice) {
                var deviceObj = this.ppDevice.readPointer();
                logBoth('[DI8] Device created: 0x' + deviceObj.toString(16));
                hookGetDeviceState(deviceObj);
            }
        }
    });
}

// ── Hook IDirectInputDevice8::GetDeviceState (vtable index 9) ──
var deviceCount = 0;
var prevStates = {};
var seenKeys = {};
var getStateCount = 0;

function hookGetDeviceState(deviceObj) {
    deviceCount++;
    var devId = deviceCount;

    // IDirectInputDevice8 vtable:
    // 0=QueryInterface, 1=AddRef, 2=Release, 3=GetCapabilities,
    // 4=EnumObjects, 5=GetProperty, 6=SetProperty, 7=Acquire,
    // 8=Unacquire, 9=GetDeviceState
    var vtable = deviceObj.readPointer();
    var getStateAddr = vtable.add(9 * 4).readPointer();
    logBoth('[DEV' + devId + '] GetDeviceState at 0x' + getStateAddr.toString(16));

    Interceptor.attach(getStateAddr, {
        onEnter: function(args) {
            // GetDeviceState(this, cbData, lpvData)
            this.cbData = args[1].toInt32();
            this.lpvData = args[2];
            this.devId = devId;
        },
        onLeave: function(retval) {
            if (retval.toInt32() !== 0) return; // DI_OK = 0
            getStateCount++;

            var size = this.cbData;
            var buf = this.lpvData;
            var id = this.devId;
            var stateKey = 'dev' + id;

            // Read the state buffer
            var data;
            try {
                data = new Uint8Array(buf.readByteArray(size));
            } catch(e) { return; }

            // Compare against previous state
            var prev = prevStates[stateKey];
            if (prev) {
                for (var k = 0; k < data.length && k < prev.length; k++) {
                    if (data[k] !== prev[k]) {
                        var key = stateKey + ':' + k;
                        var msg = '[INPUT] dev' + id + ' offset ' + k +
                            ': ' + prev[k] + ' -> ' + data[k];

                        // Console: first time each key/offset changes
                        if (!seenKeys[key]) {
                            seenKeys[key] = true;

                            // Backtrace on first input detection per device
                            var bt = Thread.backtrace(this.context, Backtracer.ACCURATE);
                            var btGhidra = [];
                            for (var b = 0; b < Math.min(bt.length, 5); b++) {
                                btGhidra.push('0x' + toGhidra(bt[b]).toString(16));
                            }
                            logBoth(msg + ' <- ' + btGhidra.join(' <- '));
                        } else {
                            logFile(msg);
                        }
                    }
                }
            } else {
                // First read — log device info
                logBoth('[DEV' + id + '] Buffer size: ' + size + ' bytes');
                if (size === 256) {
                    logBoth('[DEV' + id + '] Keyboard device (256-byte state)');
                } else {
                    logBoth('[DEV' + id + '] Gamepad/other device (' + size + '-byte state)');
                }
            }

            prevStates[stateKey] = data;

            // Periodic log
            if (getStateCount % 500 === 0) {
                logFile('[POLL] ' + getStateCount + ' total GetDeviceState calls');
            }
        }
    });
    logBoth('[OK] DEV' + devId + ' GetDeviceState hooked');
}

// ── Also try to hook already-created devices ──
// DirectInput8Create may have already been called before we attached.
// Try to find the device by hooking the game's call site instead.
// The FFX.exe import for DirectInput8Create is at a known location.
// But if the device already exists, we can try to hook GetDeviceState
// by finding the device object in memory.
//
// Alternative: hook the game's input polling function that CALLS GetDeviceState.
// From the decompile, the PhyreEngine input update (FUN_00625930, RVA 0x225930)
// iterates registered devices and calls their update methods.
// Let's also hook that to catch the call chain.

var pInputDumped = false;
try {
    Interceptor.attach(rva(0x225930), {
        onEnter: function(args) {
            var ecx = ptr(this.context.ecx);
            if (!pInputDumped) {
                pInputDumped = true;
                logBoth('[PINPUT] ECX=0x' + ecx.toString(16));
                // Dump key offsets from the PInput object
                try {
                    for (var off = 0; off < 0x40; off += 4) {
                        var val = ecx.add(off).readU32();
                        if (val !== 0) {
                            logFile('[PINPUT] +0x' + off.toString(16) + ' = 0x' + val.toString(16));
                        }
                    }
                } catch(e) { logFile('[PINPUT] dump error: ' + e.message); }
            }
            if (getStateCount === 0) {
                logFile('[INPUT_UPDATE] PInput::Update called, getStateCount=' + getStateCount);
            }
        }
    });
    logBoth('[OK] PInput::Update (RVA 0x225930)');
} catch(e) {
    logBoth('[!] PInput::Update: ' + e.message);
}

logBoth('[*] Press UP/DOWN on title screen. Input changes will appear on console.');
