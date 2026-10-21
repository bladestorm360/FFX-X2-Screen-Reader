The native reader uses the installed x86 MinHook 1.3.4 binary and PRISM 0.18.3
through their public APIs. Game and launcher speech share the native PRISM
adapter and serialize backend work on their speech owner. The single mod
installer includes the x86 PrismRuntime pinned by prism.lock.json and installs
it when missing, including its license, notice, component licenses and build
provenance. A matching receipted runtime is verified without replacement.
Developer deployment checks the installed receipt and locked files read-only.
It contains no copied UnX/Fahrenheit game implementation.
The original WinForms launcher is extended by a C++/CLI adapter; the speech
worker and game reader are native C++.

SDL3 3.4.4 is built from the shared SDL3-source tree with the x86 toolchain,
loaded through its public API and deployed as SDL3.dll. SDL is under the zlib
license; its original LICENSE.txt is staged as SDL3_LICENSE.txt and installed as
FFX_SR_SDL3_LICENSE.txt. Source: https://github.com/libsdl-org/SDL.

The sample decoder and objective/arrival WAV assets are adapted/copied from the
user's Bravely Default mod. Controller command arming/rebinding and
navigation audio follow that mod; the spoken flat settings menu follows the
user's FF12 mod. These are custom source references, not copies from Fahrenheit
or UnX. See sounds/README.md for asset provenance.

Standard/Expert Sphere Grid layout records are from
https://github.com/Grayfox96/FFX-Sphere-Grid-viewer, copyright (c) 2024 Grayfox96,
MIT, extracted with Karifean's FFXDataParser. Source URLs, hashes and the original
license are preserved with the offline datasets; sphere_layouts_LICENSE.txt
retains the full notice and is embedded in the native DLL as resource 203.

MinHook copyright (C) 2009-2017 Tsuda Kageyu. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

PRISM source: https://github.com/ethindp/prism, pinned commit
b68e9c94240cefb1f08b2306c26376e3ab300a7a. PRISM is licensed under MPL-2.0.
PrismRuntime includes the original PRISM license, notice, bundled component
licenses and build provenance; see prism.lock.json for exact file hashes.
The native adapter replaces the former Tolk dependency. SDK initialization and
all backend calls stay on the serialized speech owner; no runtime speech-engine
or gameplay confirmation is implied by offline ABI fixtures.
