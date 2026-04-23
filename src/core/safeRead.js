// Guarded memory readers. Every feature reads game memory through these —
// they validate the pointer and catch read failures rather than letting
// them propagate into `Interceptor` callbacks where an unhandled exception
// can destabilize the host process.
//
// Rules:
// - `isValidPtr(p)` rejects null, missing, and low-address garbage (the
//   0x10000 threshold catches uninit sentinels like 0x1, 0xCC, 0xFEEE, etc.
//   that slip past `.isNull()`).
// - None of these functions throw. They return `null` / `0` / `''` on any
//   failure. Callers should treat the result as "no data this frame" and
//   continue gracefully.

export function isValidPtr(p) {
    if (!p) return false;
    try {
        if (p.isNull()) return false;
        return p.compare(ptr(0x10000)) > 0;
    } catch (e) { return false; }
}

// Returns an ArrayBuffer, or null on invalid address / partial-page read.
// `readByteArray` itself returns null when the read crosses an unmapped
// page, which `new Uint8Array(null)` would then throw on. Callers that
// wrap the result in a Uint8Array MUST check for null first.
export function safeReadByteArray(addr, maxLen) {
    if (!isValidPtr(addr)) return null;
    try {
        const raw = addr.readByteArray(maxLen);
        return raw || null;
    } catch (e) { return null; }
}

// Guarded pointer read. Returns the pointer, or null on any failure.
export function safeReadPointer(addr) {
    if (!isValidPtr(addr)) return null;
    try {
        const p = addr.readPointer();
        return isValidPtr(p) ? p : null;
    } catch (e) { return null; }
}

// Bounded C-string read. Requires explicit maxLen — no unbounded walks.
// Replaces the helper that lived in utils/text.js.
export function safeReadCString(addr, maxLen) {
    if (!isValidPtr(addr)) return null;
    try {
        const s = addr.readCString(maxLen);
        return (s && s.length > 0) ? s : null;
    } catch (e) { return null; }
}
