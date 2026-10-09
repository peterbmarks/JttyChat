# JttyChat

JttyChat is a small Qt6 desktop chat application, styled after iMessage, for
the **JTTY** amateur radio digital mode. Type a message and send it, and it's
transmitted over the air as a JTTY signal through your transceiver; messages
received from other stations appear as incoming chat bubbles.

## Features

- A scrolling chat view with speech-bubble messages — blue/right-aligned for
  sent, grey/left-aligned for received — above a text field and Send button.
- **Transmit**: typed messages are encoded to a JTTY audio waveform and
  played out your chosen sound output device, keying your transceiver's PTT
  (via Hamlib) for exactly the duration of the transmission.
- **Receive**: audio from your chosen sound input device is continuously
  decoded in the background; completed JTTY messages appear automatically as
  received bubbles.
- A Settings window (File → Settings…) for your callsign, audio input/output
  device selection, and a Transceiver section for Hamlib rig control (rig
  model, serial port, baud rate, and a Connect button to test the link and
  show the rig's current frequency and mode).
- Captitalise text option to try to do sentence capitalisation
- Append callsign adds your callsign to the end of messages
- Macro buttons. Right click to set up. Click to insert text.
- A build script that packages the app as a Linux AppImage for distribution.

## Building on Linux

### Dependencies

JttyChat needs a C++17 compiler, a Fortran compiler (the JTTY codec is
Fortran — see [Credits](#credits)), CMake, Qt6 (widgets + multimedia), Hamlib,
and FFTW3's single-precision library.

On Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake gfortran pkg-config \
    qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev \
    libhamlib-dev libfftw3-dev
```

Other distributions will need the equivalent packages: a Qt6 development
environment with the Multimedia module, Hamlib's development package
(provides `hamlib.pc`), FFTW3's single-precision development package
(provides `fftw3f.pc`), and `gfortran`.

### Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/JttyChat
```

### Packaging as an AppImage

```sh
./scripts/build_appimage.sh
```

This builds the app, then downloads `linuxdeploy` and its Qt plugin on first
run (cached under `tools/`) to bundle everything into
`dist/JttyChat-x86_64.AppImage`.

## Project layout

- `src/` — the Qt6 C++ application.
- `thirdparty/jtty_codec/` — the JTTY encode/decode engine (Fortran), copied
  from an existing implementation; see [Credits](#credits).
- `packaging/` — the desktop entry and icon used by the AppImage.
- `scripts/build_appimage.sh` — builds and packages the AppImage.

## Credits

The JTTY encode/decode engine in `thirdparty/jtty_codec/` is copied,
unmodified, from the JTTY mode implementation in a fork of
[WSJT-X](https://wsjtx.github.io/wsjtx/) (`wsjtx-3.2.0-rc1`). JTTY is a
GFSK-based weak-signal mode designed for fast, RTTY-style contest exchanges
and keyboard-to-keyboard contacts; see `thirdparty/jtty_codec/NOTICE.txt` for
the specific entry points this project calls.

WSJT-X is developed by the WSJT Development Team:

Joe Taylor, K1JT; Bill Somerville, G4WJS; Steve Franke, K9AN; Nico Palermo,
IV3NWV; Uwe Risse, DG2YCB; Brian Moran, N9ADG; Roger Rehr, W3SZ; John Nelson,
G4KLA; Charlie Suckling, DL3WDG; Terrell Deppe, KJ5HST; and David Christle,
KD0BTO —

with acknowledged contributions from AC6SL, AE4JY, DF2ET, DJ0OT, DL3WDG,
EA4AC, G4KLA, IW3RAB, JA7UDE, K3WYC, KA1GT, KA6MAL, KA9Q, KB1ZMX, KD6EKQ,
KG4IYS, KI7MT, KK1D, ND0B, PY1ZRJ, PY2SDR, VE1SKY, VK3ACF, VK4BDJ, VK7MO,
VR2UPU, W3DJS, W4TI, W4TV, and W9MDB.

The copied codec is licensed under the GNU General Public License v3 (see
`thirdparty/jtty_codec/COPYING`); JttyChat, which links it directly into the
application, is distributed under the same license.

JttyChat also depends on:

- [Qt6](https://www.qt.io/) — application framework and UI.
- [Hamlib](https://hamlib.github.io/) — transceiver (CAT/PTT) control.
- [FFTW](https://www.fftw.org/) — used internally by the JTTY codec.

# Testing

I use a HackRF recording which I play back like this

```sh
hackrf_transfer -t jtty_capture.iq8 -f 7090000 -x 47 -R
```
