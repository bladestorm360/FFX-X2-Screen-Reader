// @desc Walk PInput device list — discover ALL input devices and track state changes
//
// PInput::Update (RVA 0x225930) fires every frame and iterates a linked list of devices.
// Each device object has a function pointer at +0x14 which is its update method.
// This probe discovers all devices, hooks their update methods, and tracks state changes.
//
// Known devices (identified via Ghidra vtable analysis):
//   DEV0: PInputDeviceMouse   (vtable 0x621e70, update 0x62d8a0) — GetCursorPos etc.
//   DEV1: PInputDeviceKeyboard (vtable 0x621dc0, update 0x62d770) — DirectInput 256-byte
//   DEV2: PInputDevicePad      (vtable 0x622030, update 0x62dd50) — XInput (reads zeros)
//
// Console: device list on attach, KEYBOARD state changes only.
// File: everything (all devices).
//
// Press keyboard arrows on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/walk_device_list.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function hexDump(addr, len) {
    try {
        var buf = addr.readByteArray(len);
        var bytes = new Uint8Array(buf);
        var hex = '';
        for (var i = 0; i < bytes.length; i++) {
            var h = bytes[i].toString(16);
            if (h.length < 2) h = '0' + h;
            hex += h;
            if ((i + 1) % 4 === 0) hex += ' ';
        }
        return hex.trim();
    } catch(e) { return '(unreadable)'; }
}

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
var modEnd = base.add(Process.enumerateModules()[0].size);
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] walk_device_list');
logBoth('[*] base=0x' + base.toString(16));

// ── Verify PInput::Update hook with raw bytes ──
var pInputAddr = rva(0x225930);
logBoth('[*] PInput::Update at runtime 0x' + pInputAddr.toString(16));
logFile('[*] First 16 bytes at PInput::Update: ' + hexDump(pInputAddr, 16));

// ── Read the global linked list pointer ──
// PTR_LOOP_00c30fa0 in Ghidra → RVA 0x830fa0
var listHeadRVA = 0x830fa0;
var listHead = rva(listHeadRVA);
logBoth('[*] Device list head (PTR_LOOP_00c30fa0) at runtime 0x' + listHead.toString(16));

// ── Hook PInput::Update — walk device list on first call ──
var pInputCount = 0;
var devicesDiscovered = false;
var deviceUpdateFuncs = [];
var deviceObjects = [];
var SNAP_SIZE = 0x600; // bytes to snapshot per device (keyboard state at ~0x218, XInput at ~0x398)
var deviceSnaps = [];
var deviceUpdateCounts = [];
var deviceTypes = []; // 'keyboard', 'mouse', 'xinput', or 'unknown'

// Known vtable addresses (Ghidra VAs) for device type identification
var VTABLE_MOUSE    = 0x621e70;
var VTABLE_KEYBOARD = 0x621dc0;
var VTABLE_XINPUT   = 0x622030;

try {
    Interceptor.attach(pInputAddr, {
        onEnter: function(args) {
            pInputCount++;
            var ecx = ptr(this.context.ecx);

            if (!devicesDiscovered) {
                devicesDiscovered = true;
                logBoth('[PINPUT] ECX=0x' + ecx.toString(16));

                // Also check the DAT_00cc9cd0 flag and callback
                var flagAddr = rva(0x8C9CD0);
                var cbAddr = rva(0x8C9CD4);
                try {
                    var flag = flagAddr.readU8();
                    var cb = cbAddr.readPointer();
                    logFile('[PINPUT] DAT_00cc9cd0 flag=' + flag + ' cb=0x' + cb.toString(16));
                } catch(e) {}

                // Walk the linked list
                try {
                    var headVal = listHead.readPointer();
                    logFile('[LIST] Head value: 0x' + headVal.toString(16));
                    logFile('[LIST] Head addr:  0x' + listHead.toString(16));

                    // The list is circular: iterate until we see listHead again or null
                    var node = headVal;
                    var count = 0;
                    var maxNodes = 20; // safety limit

                    while (!node.isNull() && count < maxNodes) {
                        // Check if we've looped back to head
                        if (count > 0 && node.equals(listHead)) {
                            logFile('[LIST] Looped back to head after ' + count + ' nodes');
                            break;
                        }

                        // Device object is at node[-1] (offset -4 from node)
                        // From decompile: ppuVar1[-1] + 0x14 for vtable access
                        // ppuVar1[-1] is *(node - 4)
                        var deviceObjPtr;
                        try {
                            deviceObjPtr = node.sub(4).readPointer();
                        } catch(e) {
                            logFile('[LIST] Cannot read node[-1] at ' + node.sub(4));
                            break;
                        }

                        if (deviceObjPtr.isNull()) {
                            logFile('[LIST] Device object is NULL at node ' + count);
                            break;
                        }

                        // Read function table pointer at deviceObj+0x14
                        // Decompile: (**(code **)(ppuVar1[-1] + 0x14))()
                        // = read ptr at deviceObj+0x14, then read first entry from that table
                        // Read update function pointer at deviceObj+0x14
                        // Decompile: (**(code **)(ppuVar1[-1] + 0x14))()
                        // The ** is Ghidra's call-through-pointer syntax, NOT two memory reads.
                        // Actual operation: read function ptr at deviceObj+0x14, then CALL it.
                        // ONE dereference only.
                        var updateFunc;
                        try {
                            updateFunc = deviceObjPtr.add(0x14).readPointer();
                        } catch(e) {
                            logFile('[LIST] Cannot read update func at device+0x14');
                            break;
                        }

                        var ghidraUpdate = toGhidra(updateFunc);
                        var inModule = updateFunc.compare(base) >= 0 && updateFunc.compare(modEnd) < 0;

                        // Also read standard vtable at offset 0 for identification
                        var stdVtable;
                        try { stdVtable = deviceObjPtr.readPointer(); } catch(e) { stdVtable = ptr(0); }
                        var ghidraVtable = stdVtable.isNull() ? 'null' : '0x' + toGhidra(stdVtable).toString(16);

                        logBoth('[DEV' + count + '] obj=0x' + deviceObjPtr.toString(16) +
                            ' vtable=' + ghidraVtable +
                            ' update=0x' + ghidraUpdate.toString(16) +
                            (inModule ? ' (FFX.exe)' : ' (external)'));

                        // Verify: read first 4 bytes at update func to confirm it's code
                        logFile('[DEV' + count + '] update bytes: ' +
                            hexDump(updateFunc, 8));

                        // Dump first 16 bytes of the device object
                        logFile('[DEV' + count + '] hex: ' + hexDump(deviceObjPtr, 64));

                        // Read nearby offsets in device object for context
                        logFile('[DEV' + count + '] device offsets:');
                        for (var v = 0; v < 0x20; v += 4) {
                            try {
                                var val = deviceObjPtr.add(v).readPointer();
                                var valInMod = val.compare(base) >= 0 && val.compare(modEnd) < 0;
                                logFile('  +0x' + v.toString(16) + ' = 0x' + val.toString(16) +
                                    (valInMod ? ' (Ghidra 0x' + toGhidra(val).toString(16) + ')' : ''));
                            } catch(e) { break; }
                        }

                        // Identify device type by vtable address
                        var ghidraVtableNum = stdVtable.isNull() ? 0 : toGhidra(stdVtable).toInt32();
                        var devType = 'unknown';
                        if (ghidraVtableNum === VTABLE_KEYBOARD) devType = 'keyboard';
                        else if (ghidraVtableNum === VTABLE_MOUSE) devType = 'mouse';
                        else if (ghidraVtableNum === VTABLE_XINPUT) devType = 'xinput';
                        logBoth('[DEV' + count + '] type=' + devType);

                        deviceObjects.push(deviceObjPtr);
                        deviceUpdateFuncs.push({addr: updateFunc, ghidra: ghidraUpdate, idx: count});
                        deviceSnaps.push(null);
                        deviceUpdateCounts.push(0);
                        deviceTypes.push(devType);

                        // Next node
                        try {
                            node = node.readPointer();
                        } catch(e) {
                            logFile('[LIST] Cannot read next pointer at node ' + count);
                            break;
                        }
                        count++;
                    }

                    logBoth('[*] Found ' + count + ' devices. Hooking update methods...');

                    // Hook each device's update method
                    for (var d = 0; d < deviceUpdateFuncs.length; d++) {
                        hookDeviceUpdate(d);
                    }

                    logBoth('[*] Press d-pad or keyboard. State changes will appear.');

                } catch(e) {
                    logBoth('[!] List walk error: ' + e.message);
                }
            }
        }
    });
    logBoth('[OK] PInput::Update hooked (RVA 0x225930)');
} catch(e) {
    logBoth('[!] PInput::Update: ' + e.message);
}

function hookDeviceUpdate(devIdx) {
    var info = deviceUpdateFuncs[devIdx];
    var devObj = deviceObjects[devIdx];

    try {
        Interceptor.attach(info.addr, {
            onEnter: function(args) {
                this.devIdx = devIdx;
                this.ecx = ptr(this.context.ecx);
            },
            onLeave: function(retval) {
                var idx = this.devIdx;
                deviceUpdateCounts[idx]++;
                var ecx = this.ecx;

                // Take snapshot of device state
                var newSnap;
                try {
                    newSnap = new Uint8Array(ecx.readByteArray(SNAP_SIZE));
                } catch(e) { return; }

                var oldSnap = deviceSnaps[idx];
                if (oldSnap) {
                    // Diff
                    var changes = [];
                    for (var b = 0; b < SNAP_SIZE; b++) {
                        if (oldSnap[b] !== newSnap[b]) {
                            changes.push({offset: b, from: oldSnap[b], to: newSnap[b]});
                        }
                    }
                    if (changes.length > 0) {
                        var devType = deviceTypes[idx] || 'unknown';
                        var parts = [];
                        for (var c = 0; c < Math.min(changes.length, 8); c++) {
                            parts.push('+0x' + changes[c].offset.toString(16) + ':' +
                                changes[c].from + '->' + changes[c].to);
                        }
                        var msg = '[INPUT ' + devType.toUpperCase() + ' DEV' + idx + '] ' +
                            changes.length + ' changes: ' + parts.join(', ');
                        // Console: keyboard only (mouse is noisy, xinput reads zeros)
                        if (devType === 'keyboard') {
                            logBoth(msg);
                        } else {
                            logFile(msg);
                        }
                        // Full detail to file always
                        for (var d = 0; d < changes.length; d++) {
                            logFile('  +0x' + changes[d].offset.toString(16) + ': ' +
                                changes[d].from + ' -> ' + changes[d].to);
                        }
                    }
                }

                deviceSnaps[idx] = newSnap;

                // Periodic log
                if (deviceUpdateCounts[idx] % 500 === 0) {
                    logFile('[DEV' + idx + '] update count: ' + deviceUpdateCounts[idx]);
                }
            }
        });
        logBoth('[OK] DEV' + devIdx + ' update hooked (Ghidra 0x' +
            info.ghidra.toString(16) + ')');
    } catch(e) {
        logBoth('[!] DEV' + devIdx + ' hook failed: ' + e.message);
    }
}

logBoth('[*] Waiting for PInput::Update to discover devices...');
