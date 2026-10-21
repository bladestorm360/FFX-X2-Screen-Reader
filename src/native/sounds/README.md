The user's Bravely Default `objective.wav` and `arrival.wav` are reused byte for
byte for FFX's beacon and arrival feedback. Bravely's sound notes identify these
as user-supplied workspace assets. Both are embedded in the x86 proxy. Compass
wall tones are synthesized, following the Bravely mixer design.

pickup.wav is an original, user-authorized generated 480 Hz sine loop: mono
16-bit PCM, 48 kHz, 200 ms. Resource 204 embeds it; resource 203 remains the
Sphere Grid license. The race mixer plays it continuously with equal-power
stereo driven by lateral error divided by the native pickup radius.

At bootstrap, Sounds/beacon.wav, Sounds/arrival.wav and Sounds/pickup.wav beside
the proxy can override the embedded assets. Accepted files are mono 16-bit PCM
WAV, at most two seconds and 256 KiB; they are decoded/resampled to 48 kHz once.
Missing/invalid files retain defaults. The callback never opens or decodes files.
