// @auto
// @desc Search memory for title screen text, log addresses + module owners

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/backtrace_text.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// Known title screen strings to search for (both UTF-8 and UTF-16)
var searchStrings = [
    'New Game',
    'Continue',
    'Load Game',
    'Load',
    'New',
    'Tidus',
    'Yuna',
    'Auron',
    'Items',
    'Equipment',
    'Sphere Grid',
    'Attack',
    'Magic',
    'Overdrive'
];

logBoth('[*] backtrace_text: searching for game text in memory...');

// Get all modules for ownership checks
var modules = Process.enumerateModules();

function findModuleForAddr(addr) {
    var a = addr instanceof NativePointer ? addr : ptr(addr);
    for (var i = 0; i < modules.length; i++) {
        var base = modules[i].base;
        var end = base.add(modules[i].size);
        if (a.compare(base) >= 0 && a.compare(end) < 0) {
            return modules[i].name + '+0x' + a.sub(base).toString(16);
        }
    }
    return 'unknown(0x' + a.toString(16) + ')';
}

// Convert a JS string to a byte pattern for scanning
function stringToHexPattern(s) {
    var hex = '';
    for (var i = 0; i < s.length; i++) {
        var c = s.charCodeAt(i);
        var h = c.toString(16);
        if (h.length < 2) h = '0' + h;
        hex += h + ' ';
    }
    return hex.trim();
}

// Convert to UTF-16LE byte pattern
function stringToUtf16Pattern(s) {
    var hex = '';
    for (var i = 0; i < s.length; i++) {
        var c = s.charCodeAt(i);
        var lo = (c & 0xFF).toString(16);
        var hi = ((c >> 8) & 0xFF).toString(16);
        if (lo.length < 2) lo = '0' + lo;
        if (hi.length < 2) hi = '0' + hi;
        hex += lo + ' ' + hi + ' ';
    }
    return hex.trim();
}

// Dump hex context around an address
function hexDump(addr, before, after) {
    try {
        var start = addr.sub(before);
        var buf = start.readByteArray(before + after);
        if (!buf) return '(unreadable)';
        var bytes = new Uint8Array(buf);
        var hex = '';
        for (var i = 0; i < bytes.length; i++) {
            var h = bytes[i].toString(16);
            if (h.length < 2) h = '0' + h;
            hex += h + ' ';
            if (i === before - 1) hex += '| ';
        }
        return hex.trim();
    } catch (e) {
        return '(read error: ' + e.message + ')';
    }
}

var totalHits = 0;

// Scan all readable memory ranges
var ranges = Process.enumerateRanges('r--');
logFile('[*] Scanning ' + ranges.length + ' memory ranges...');
logFile('');

for (var si = 0; si < searchStrings.length; si++) {
    var searchStr = searchStrings[si];
    var utf8Pattern = stringToHexPattern(searchStr);
    var utf16Pattern = stringToUtf16Pattern(searchStr);
    var utf8Hits = 0;
    var utf16Hits = 0;

    logFile('=== Searching for: "' + searchStr + '" ===');
    logFile('  UTF-8 pattern:  ' + utf8Pattern);
    logFile('  UTF-16 pattern: ' + utf16Pattern);

    for (var ri = 0; ri < ranges.length; ri++) {
        var range = ranges[ri];

        // UTF-8 scan
        try {
            var matches = Memory.scanSync(range.base, range.size, utf8Pattern);
            for (var m = 0; m < matches.length; m++) {
                var addr = matches[m].address;
                var owner = findModuleForAddr(addr);
                var context = hexDump(addr, 16, searchStr.length + 16);
                logFile('  [UTF-8] 0x' + addr.toString(16) + ' in ' + owner);
                logFile('    hex: ' + context);
                utf8Hits++;
                totalHits++;
            }
        } catch (e) {}

        // UTF-16 scan
        try {
            var matches16 = Memory.scanSync(range.base, range.size, utf16Pattern);
            for (var m = 0; m < matches16.length; m++) {
                var addr16 = matches16[m].address;
                var owner16 = findModuleForAddr(addr16);
                var context16 = hexDump(addr16, 16, searchStr.length * 2 + 16);
                logFile('  [UTF16] 0x' + addr16.toString(16) + ' in ' + owner16);
                logFile('    hex: ' + context16);
                utf16Hits++;
                totalHits++;
            }
        } catch (e) {}
    }

    var hitSummary = searchStr + ': UTF-8=' + utf8Hits + ', UTF-16=' + utf16Hits;
    if (utf8Hits > 0 || utf16Hits > 0) {
        logBoth('[HIT] ' + hitSummary);
    } else {
        logFile('  (no matches)');
    }
    logFile('');
}

logBoth('[*] Scan complete. Total hits: ' + totalHits);
logBoth('[*] Log: ' + logPath);
logHandle.close();
