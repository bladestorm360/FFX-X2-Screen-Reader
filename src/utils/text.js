// Generic string helpers. The low-level guarded reader is in core/safeRead.js;
// this file keeps the existing `readCString(addr)` shape (64-byte default)
// so pre-refactor import sites don't need to change.

import { safeReadCString } from '../core/safeRead.js';

export function readCString(addr, maxLen) {
    return safeReadCString(addr, maxLen || 64);
}

export function cleanText(s) {
    while (s.length > 1 && /^[^A-Za-z0-9 ]/.test(s[0])) s = s.substring(1);
    if (s.length > 1 && /^[0-9][A-Za-z]/.test(s)) s = s.substring(1);
    return s.trim();
}
