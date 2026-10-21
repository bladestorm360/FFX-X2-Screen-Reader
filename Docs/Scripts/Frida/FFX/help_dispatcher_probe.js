// @desc Help-menu event-dispatcher probe (FUN_00776220 input dispatch)
// @auto
//
// Confirms whether FUN_00776220 (RVA 0x376220) is event-driven (fires only
// on input) or per-frame (fires every frame and returns early on idle).
//
// FUN_00776220 writes *param_1:
//   0 = no input this call (idle)
//   1 = cursor advanced to a new item (DAT_0112A088 / DAT_0112A084 changed)
//   2 = confirm/cancel with downstream state change (DAT_0112A038 set)
//
// Events of interest are retcode != 0. Per-frame zero retcode is the
// silent path. The probe counts both and surfaces a periodic summary so
// we can tell whether a production hook here would fire on idle frames.
//
// User instructions: open Main Menu -> Help, idle 10s, press D-pad
// up/down a few times, exit. Expected if event-driven: ~0 calls/sec on
// idle, ~1 call per arrow press with retcode=1. Expected if per-frame:
// ~60 calls/sec on idle with retcode=0, retcode=1 only on input.

var base = Process.enumerateModules()[0].base;
function rva(off) { return base.add(off); }

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/help_dispatcher_probe.log';
var logH = new File(logPath, 'w');
function logFile(m) { logH.write(m + '\n'); logH.flush(); }

var DAT_CURSOR_IDX  = rva(0xD2A088); // DAT_0112A088 (32-bit page index)
var DAT_FOCUS_PTR   = rva(0xD2A084); // DAT_0112A084 (focused-item ptr)
var DAT_SCENE       = rva(0x8416B0);  // DAT_00c416b0 (scene id; Help = 0x0B)
var FUN_DISPATCHER  = 0x376220;       // FUN_00776220

var counters = { total: 0, ret0: 0, ret1: 0, ret2: 0, retOther: 0, focusChanges: 0 };
var lastFocus = ptr(0);
var SUMMARY_EVERY = 600; // ~10s at 60fps
var sinceSummary = 0;

console.log('[HELP_DISPATCHER_PROBE] hook installed. Open Help menu, idle, then arrow.');
logFile('--- help_dispatcher_probe START base=' + base + ' ---');

Interceptor.attach(rva(FUN_DISPATCHER), {
    onEnter: function(args) {
        this.outArg = args[0];
        try {
            this.cursorBefore = DAT_CURSOR_IDX.readS32();
            this.focusBefore  = DAT_FOCUS_PTR.readPointer();
        } catch (e) {
            this.cursorBefore = -1;
            this.focusBefore  = ptr(0);
        }
    },
    onLeave: function() {
        try {
            counters.total++;
            sinceSummary++;
            var ret = -1;
            try { ret = this.outArg.readS32(); } catch (e) {}
            if (ret === 0) counters.ret0++;
            else if (ret === 1) counters.ret1++;
            else if (ret === 2) counters.ret2++;
            else counters.retOther++;

            var cursorAfter = -1;
            var focusAfter  = ptr(0);
            try {
                cursorAfter = DAT_CURSOR_IDX.readS32();
                focusAfter  = DAT_FOCUS_PTR.readPointer();
            } catch (e) {}
            var focusChanged = !focusAfter.equals(this.focusBefore);
            if (focusChanged) counters.focusChanges++;

            var sceneId = -1;
            try { sceneId = DAT_SCENE.readS32(); } catch (e) {}

            if (ret !== 0 || focusChanged) {
                var msg = '[EVENT] ret=' + ret +
                    ' scene=0x' + sceneId.toString(16) +
                    ' cursor=' + this.cursorBefore + '->' + cursorAfter +
                    ' focus=' + this.focusBefore + '->' + focusAfter;
                console.log(msg);
                logFile(msg);
            }

            if (sinceSummary >= SUMMARY_EVERY) {
                sinceSummary = 0;
                var s = '[SUMMARY] total=' + counters.total +
                    ' ret0=' + counters.ret0 +
                    ' ret1=' + counters.ret1 +
                    ' ret2=' + counters.ret2 +
                    ' retOther=' + counters.retOther +
                    ' focusChanges=' + counters.focusChanges;
                console.log(s);
                logFile(s);
            }
        } catch (e) { logFile('[ERR] ' + e.message); }
    }
});

logFile('hook installed; awaiting activity');
