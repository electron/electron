# Linux Dependencies

This document lists the runtime dependencies required for Electron applications on Linux.

Related: issue #48989

## Core Dependencies

The following libraries are required for basic Electron functionality:

### Windowing and UI

* GTK 3 (`libgtk-3-0`) - Windowing toolkit and UI components
* X11 - Display server protocol (when running under X11 or XWayland)
* X11 client / helper libraries:
  * `libx11` (libX11) - core X11 client library
  * `libxcb1` (libxcb) - X protocol C-language binding used by toolkits and Chromium
  * `libxrandr` / `libxfixes` - common X11 helper libraries
  * `libxtst6` (XTest) - Input simulation and testing
  * `libxss1` (XScrnSaver) - Screen saver extension (optional — see note below)

### Security and Networking

* NSS (`libnss3`) - Network Security Services for TLS/SSL
* NSpr (`libnspr4`) - Netscape Portable Runtime

### Audio

* ALSA (`libasound2`) - Advanced Linux Sound Architecture
* PulseAudio (`libpulse0` / `libpulse`) - Sound server

### System Integration

* DBus (`libdbus-1-3`) - Inter-process communication
* CUPS (`libcups2` / `cups-libs`) - Printing system
* Notify (`libnotify4` / `libnotify`) - Desktop notifications

## Optional Dependencies

### Secure Storage

Depending on desktop environment and distribution:

* libsecret / GNOME Secret Service (`libsecret`, `libsecret-1-0`) — common on modern GNOME
* GNOME Keyring (`libgnome-keyring`) — older GNOME setups
* KDE KWallet (`kwalletd`, `libkwalletbackend5`) — package names vary by distro; electron-builder can integrate KWallet

### Wayland

* For Wayland-native sessions you may need `libwayland-client` and compositor-specific helper libraries. If targeting Wayland, test on Wayland-only environments; some distributions provide combined runtimes (e.g., Flatpak runtimes) that include common Wayland libs.

## Distribution-Specific Package Names

### Ubuntu / Debian
```bash
# Core dependencies
libgtk-3-0
libnss3
libnspr4
libasound2
libpulse0
libdbus-1-3
libcups2
libnotify4
libx11-6
libxcb1
libxrandr2
libxfixes3
libxtst6

# Optional
libsecret-1-0
libgnome-keyring0
```

### RHEL / CentOS / Fedora

```bash
# Core dependencies
gtk3
nss
nspr
alsa-lib
pulseaudio-libs
dbus-libs
cups-libs
libnotify
libX11
libxcb
libXrandr
libXfixes
libXtst

# Optional
libsecret
libgnome-keyring
kwalletd
```

### Arch Linux / Manjaro
```bash
# Core dependencies
gtk3
nss
nspr
alsa-lib
libpulse
dbus
libcups
libnotify
libx11
libxcb
libxrandr
libxfixes
libxtst

# Optional
libsecret
libgnome-keyring
kwallet
```

### openSUSE
```bash
# Core dependencies
libgtk-3-0
mozilla-nss
mozilla-nspr
alsa-lib
libpulse0
libdbus-1-3
cups-libs
libnotify4
libX11_6
libxcb1
libXrandr2
libXfixes3
libXtst6

# Optional
libsecret-1-0
libgnome-keyring0
```

Notes: package names can vary across distro versions — test on the specific target distro/release.

## Important Notes

### libXScrnSaver (libXss)

The `libXScrnSaver` library was previously included in dependency lists because it was thought to be required for the `powerMonitor.getSystemIdleTime()` API. However:

* Electron's powerMonitor functionality uses Chromium's built-in UI idle monitoring from `//ui/base/idle`
* This does not require libXScrnSaver
* The library is no longer available in some distributions (e.g., RHEL 10)
* Applications work correctly without it

If your packaging tool (like electron-builder) includes libXScrnSaver by default, you can safely remove it from your dependency list.

### Dynamic vs Static Linking

Electron links to most system libraries dynamically at runtime. The dependencies listed above are runtime requirements, not build-time requirements.

## Testing Dependencies

To verify your application has the correct dependencies, use:

* `ldd` - List dynamic dependencies of executables
  * Example: `ldd /path/to/electron | grep "not found"`
  * Example: `ldd path/to/your/app/resources/app.asar.unpacked/native.node`
* `readelf -d` - Examine ELF file dependencies
* `objdump -p` - Display object file information

Recommended workflow:

1. Run the packaged Electron binary (or your app) on a clean container or VM of the target distro.
2. Use ldd/readelf to find missing shared libraries and install the corresponding packages.
3. Repeat in CI using docker images or distro VMs.

## Packaging Hints

* `electron-builder`: verify which system dependencies it auto-includes; remove libXss if present and unnecessary.
* `snap`: declare plugs (audio, network, etc.) and consider base snap compatibility.
* `flatpak`: use appropriate runtimes (org.freedesktop.Platform) and add portal/permission declarations.
* `AppImage`: ensure target systems provide the runtime libs or bundle appropriate deps where allowed by licenses.

## Troubleshooting

### Missing Library Errors

If you see errors like "libXXX.so.X: cannot open shared object file", identify the missing SONAME (ldd/readelf) and install the corresponding package for your distribution.

### Permission Issues

Some libraries may require specific permissions or setuid binaries. Ensure your packaging grants appropriate permissions.

### Wayland Support

Electron supports X11 and Wayland. Wayland-native sessions may require additional libraries and compositor support. If you rely on Wayland, test in a Wayland-only environment.
