# JSC CPU Temp & Fan Tray Monitor

[![CI](https://github.com/jonathanscottcobb/jsc_cpu_fan/actions/workflows/ci.yml/badge.svg)](https://github.com/jonathanscottcobb/jsc_cpu_fan/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/jonathanscottcobb/jsc_cpu_fan)](https://github.com/jonathanscottcobb/jsc_cpu_fan/releases)

Linux system-tray app that shows **CPU temperature** and **motherboard fan speeds** by reading the kernel `hwmon` sysfs interface (`/sys/class/hwmon`). No custom kernel driver and no privileged daemon.

One tray icon is drawn per reading (each CPU core and each fan), showing a short label over a live value, e.g. `C0 / 72°`, `Ex / 2.4k`.

## Tested hardware

Tested on **x86-64 Apple Mac hardware running Linux**, using Linux's
`coretemp` driver for per-core CPU temperatures and `applesmc` for fan
speeds. The sensor discovery code also supports other Linux `hwmon` drivers
and does not depend on Apple-specific paths.

![JSC CPU Monitor showing CPU core temperatures and Exhaust fan speed in the Linux system tray](resources/screenshots/tray-monitor.png)

## Download and install

Prebuilt packages are available on the
[latest release page](https://github.com/jonathanscottcobb/jsc_cpu_fan/releases/latest).
You do not need to compile the source code.

### Linux Mint, Ubuntu, Debian and derivatives (recommended)

1. Open the [latest release](https://github.com/jonathanscottcobb/jsc_cpu_fan/releases/latest).
2. Under **Assets**, download the file ending in `_amd64.deb`.
3. Open a terminal in your Downloads folder and install it:

   ```bash
   cd ~/Downloads
   sudo apt install ./jsc-cpu_*_amd64.deb
   ```

   Using `apt` installs the required Qt libraries automatically. You can also
   double-click the `.deb` and install it with your distribution's software
   installer.

4. Open your application menu and launch **JSC CPU Monitor**. It can also be
   started from a terminal:

   ```bash
   jsc_cpu
   ```

The monitor runs in the system tray. One icon appears for each detected CPU
core and fan. Right-click any icon for **Show details**, **Refresh now**, or
**Quit**.

To uninstall:

```bash
sudo apt remove jsc-cpu
```

### Other Linux distributions (portable AppImage)

The AppImage bundles Qt and does not need to be installed:

1. Open the [latest release](https://github.com/jonathanscottcobb/jsc_cpu_fan/releases/latest).
2. Under **Assets**, download `jsc-cpu-x86_64.AppImage`.
3. Make it executable and run it:

   ```bash
   cd ~/Downloads
   chmod +x jsc-cpu-x86_64.AppImage
   ./jsc-cpu-x86_64.AppImage
   ```

You can move the AppImage to a permanent location such as
`~/Applications/` before running it.

### Start automatically when you log in

For the `.deb` installation:

```bash
mkdir -p ~/.config/autostart
cp /usr/share/applications/jsc-cpu.desktop ~/.config/autostart/
```

For the AppImage, use your desktop's **Startup Applications** settings and
select the AppImage from its permanent location.

### If no sensors appear

Check whether Linux can see your hardware sensors:

```bash
sensors
```

On Debian-based distributions, install the diagnostic utility with
`sudo apt install lm-sensors`. The application itself reads Linux's `hwmon`
interface directly and does not require administrator privileges.

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
