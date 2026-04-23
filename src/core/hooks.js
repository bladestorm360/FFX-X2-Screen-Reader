// Thin wrapper around Interceptor.attach. Wraps each user-provided handler
// in a try/catch so that a JS exception inside onEnter/onLeave becomes a
// logged warning rather than an unhandled throw across the Frida callback
// boundary — the latter can destabilize the host process.

import { rva } from './memory.js';
import { logFile } from './speech.js';

function guard(fn, label) {
    if (!fn) return undefined;
    return function() {
        try { return fn.apply(this, arguments); }
        catch (e) {
            logFile('hook[' + label + '] threw: ' + e.message);
        }
    };
}

export function attachRva(rvaOffset, handlers, name) {
    const label = name || ('0x' + rvaOffset.toString(16));
    try {
        const wrapped = {};
        if (handlers.onEnter)     wrapped.onEnter     = guard(handlers.onEnter, label + '.onEnter');
        if (handlers.onLeave)     wrapped.onLeave     = guard(handlers.onLeave, label + '.onLeave');
        if (handlers.onException) wrapped.onException = guard(handlers.onException, label + '.onException');
        Interceptor.attach(rva(rvaOffset), wrapped);
        logFile(label + ' hooked');
        return true;
    } catch (e) {
        logFile('FAIL ' + label + ': ' + e.message);
        return false;
    }
}
