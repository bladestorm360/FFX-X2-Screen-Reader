# lib/ — runtime dependencies

Files here are loaded by `dinput8.dll` at runtime.

## Committed to the repo

- `frida-gadget.config` — our config; tells the gadget to auto-load `ffx_sr.js` from
  the game directory on injection.

## NOT committed (upstream dependency — fetch locally)

- `frida-gadget.dll` — Frida's injected runtime. Not our code; redistributing the
  binary in the repo would bloat git history (~17 MB each update) and muddy
  licensing, so it is `.gitignore`d and must be obtained from the Frida project.

### How to fetch `frida-gadget.dll`

1. Go to https://github.com/frida/frida/releases
2. Pick a release (the project is tested against recent gadget builds; any
   reasonably modern release works).
3. Download the **32-bit Windows** gadget asset:
   `frida-gadget-<version>-windows-x86.dll.xz`
4. Extract the `.xz` (7-Zip handles this).
5. Rename the extracted `.dll` to `frida-gadget.dll`.
6. Place it at `lib/frida-gadget.dll`.

The file must be 32-bit x86 — FFX.exe is a 32-bit process. A 64-bit gadget will
fail to load.

## Licensing

Frida is dual-licensed (wxWindows / commercial). The mod's
`THIRD_PARTY.txt` in each release attributes Frida and links the license.
Releases that redistribute `frida-gadget.dll` (e.g. the staged zip under
`releases/vX.X/`) must continue to include that attribution.
