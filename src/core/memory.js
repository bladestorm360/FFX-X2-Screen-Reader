// Base address + RVA resolution. Loaded before any other core module.

export const base = Process.enumerateModules()[0].base;

export function rva(offset) {
    return base.add(offset);
}
