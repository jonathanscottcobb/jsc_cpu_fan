# JSC CPU Temp & Fan Tray Monitor

[![CI](https://github.com/jonathanscottcobb/jsc_cpu_fan/actions/workflows/ci.yml/badge.svg)](https://github.com/jonathanscottcobb/jsc_cpu_fan/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/jonathanscottcobb/jsc_cpu_fan)](https://github.com/jonathanscottcobb/jsc_cpu_fan/releases)

Linux system-tray app that shows **CPU temperature** and **motherboard fan speeds** by reading the kernel `hwmon` sysfs interface (`/sys/class/hwmon`). No custom kernel driver and no privileged daemon.

One tray icon is drawn per reading (each CPU core and each fan), showing a short label over a live value, e.g. `C0 / 72°`, `Ex / 2.4k`.

## Install (users)

The easiest options for Linux Mint / Ubuntu (MATE, Cinnamon, and other desktops):

### Debian package (.deb)

Download the latest `jsc-cpu_*.deb` from the [Releases page](https://github.com/jonathanscottcobb/jsc_cpu_fan/releases), then:

```bash
sudo apt install ./jsc-cpu_*.deb
```

This installs `/usr/bin/jsc_cpu`, a menu entry (JSC CPU Monitor), and the app icon. Launch it from your applications menu or run `jsc_cpu`.

### AppImage (any distro, no install)

Download `jsc-cpu-x86_64.AppImage` from Releases, then:

```bash
chmod +x jsc-cpu-x86_64.AppImage
./jsc-cpu-x86_64.AppImage
```

### Start automatically on login

After installing the `.deb`, enable autostart with:

```bash
mkdir -p ~/.config/autostart
cp /usr/share/applications/jsc-cpu.desktop ~/.config/autostart/
```

(For the AppImage, copy it somewhere stable and create a `~/.config/autostart/jsc-cpu.desktop` whose `Exec=` points at it.)

## Requirements (building from source)

- Linux with `hwmon` sensors (most desktops/laptops after `sensors-detect` / stock modules)
- CMake ≥ 3.16
- C++17 compiler (`g++` or `clang++`)
- Qt 6 Widgets (`qt6-base-dev` on Debian/Ubuntu)

### Install deps (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake qt6-base-dev
```

Optional: `lm-sensors` is useful for debugging (`sensors`) but this app does **not** link against `libsensors`.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binary: `build/jsc_cpu`

If Qt6 is installed in a custom prefix:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build
```

## Run

```bash
./build/jsc_cpu
```

- Hover the tray icon for a summary tooltip, e.g. `CPU 72°C | Exhaust 1803 RPM`
- Left-click (or **Show details**) for a live table of all temperatures and fans
- Context menu: Show details, Refresh now, Quit

### Dump sensors (no tray)

```bash
./build/jsc_cpu --dump
```

Prints all discovered `hwmon` temps/fans and the primary CPU pick to stdout — useful for headless checks or verifying your board’s labels.

## How sensors are read

1. Scan `/sys/class/hwmon/hwmon*`; never assume a stable `hwmonN` number
2. Inspect both the hwmon class directory and its physical `device` target
3. Combine all `tempN_input` and `fanN_input` channels and deduplicate sysfs aliases by canonical path
4. If hwmon provides no temperatures, fall back to `/sys/class/thermal/thermal_zone*/temp`
5. Temperatures are millidegrees Celsius → °C; fans are RPM
6. Primary CPU temp preference: package/`Tctl`-like labels, else max core temp, else max of all temps
7. Poll interval: 1 second

This follows the Linux hwmon ABI rather than hard-coding vendor enumeration paths. On Apple hardware, `coretemp` commonly supplies CPU package/cores while `applesmc` supplies fans such as `Exhaust`. Some `applesmc` kernels expose attributes in the class directory and others on the parent platform device; both layouts are handled. Typical PCs expose `coretemp` / `k10temp` plus a Super I/O chip (`nct*`, `it87`, etc.) for multiple chassis fans.

Ensure sensors are loaded:

```bash
sensors
# If empty, try:
sudo sensors-detect   # follow prompts carefully
```

## Packaging & releases (maintainers)

Packaging is driven entirely by CMake + CPack, and automated by GitHub Actions.

### Build a `.deb` locally

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && cpack -G DEB
```

Dependencies (Qt6, libc, libstdc++) are resolved automatically via `dpkg-shlibdeps`, so the package installs cleanly with `apt`.

### Automated releases

- [`.github/workflows/ci.yml`](.github/workflows/ci.yml) builds, runs a headless `--dump` smoke test, and produces a `.deb` on every push/PR.
- [`.github/workflows/release.yml`](.github/workflows/release.yml) runs on a version tag and attaches a `.deb` and an AppImage to the GitHub Release.

Cut a release by pushing a tag:

```bash
git tag v1.0.0
git push origin v1.0.0
```

### Wider distribution (optional, not yet configured)

- **APT PPA (Launchpad):** the most convenient path for Mint/Ubuntu users (`add-apt-repository` + `apt install jsc-cpu` with automatic updates). Requires a Launchpad account and a GPG signing key.
- **Flathub (Flatpak):** cross-distro reach. Needs a manifest plus sandbox permissions for `/sys/class/hwmon` access and StatusNotifierItem tray support.
- **Official Debian/Ubuntu repos:** possible but slow (ITP bug, packaging review, a sponsor).

Note: desktop environments such as MATE and Cinnamon do not bundle third-party apps into their own source trees; "easy adoption" for their users is achieved through the channels above (the `.deb`/PPA being the most familiar).

## License

Released under the [MIT License](LICENSE). Copyright (c) 2026 Jonathan Scott-Cobb.

## Author

Jonathan Scott-Cobb — author and maintainer.
