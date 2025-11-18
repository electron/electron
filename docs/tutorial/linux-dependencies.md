# Linux Dependencies

This document lists the runtime dependencies required for Electron applications on Linux.

## Core Dependencies

The following libraries are required for basic Electron functionality:

### Windowing and UI
- **GTK 3** (`libgtk-3-0`) - Windowing toolkit and UI components
- **X11** - Display server protocol
- **X11 extensions**:
  - `libxtst6` (XTest) - Input simulation and testing
  - `libxss1` (XScrnSaver) - Screen saver extension (optional, see note below)

### Security and Networking
- **NSS** (`libnss3`) - Network Security Services for TLS/SSL
- **NSpr** (`libnspr4`) - Netscape Portable Runtime

### Audio
- **ALSA** (`libasound2`) - Advanced Linux Sound Architecture
- **PulseAudio** (`libpulse0`) - Sound server

### System Integration
- **DBus** (`libdbus-1-3`) - Inter-process communication
- **CUPS** (`libcups2`) - Printing system
- **Notify** (`libnotify4`) - Desktop notifications

## Optional Dependencies

### Secure Storage
Depending on your desktop environment:
- **GNOME Keyring** (`libgnome-keyring`) - For secure credential storage on GNOME
- **KWallet** - For secure credential storage on KDE (handled automatically by electron-builder)

## Distribution-Specific Package Names

### Ubuntu/Debian
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
libxtst6

# Optional
libgnome-keyring0  # or libgnome-keyring
```

### RHEL/CentOS/Fedora
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
libXtst

# Optional
libgnome-keyring  # or gnome-keyring
```

### Arch Linux/Manjaro
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
libxtst

# Optional
libgnome-keyring
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
libXtst6

# Optional
libgnome-keyring0
```

## Important Notes

### libXScrnSaver (libXss)
The `libXScrnSaver` library was previously included in dependency lists because it was thought to be required for the `powerMonitor.getSystemIdleTime()` API. However:

- Electron's powerMonitor functionality uses Chromium's built-in UI idle monitoring from `//ui/base/idle`
- This does not require libXScrnSaver
- The library is no longer available in some distributions (e.g., RHEL 10)
- Applications work correctly without it

If your packaging tool (like electron-builder) includes libXScrnSaver by default, you can safely remove it from your dependency list.

### Dynamic vs Static Linking
Electron links to most system libraries dynamically at runtime. The dependencies listed above are runtime requirements, not build-time requirements.

### Testing Dependencies
To verify your application has the correct dependencies, you can use tools like:
- `ldd` - List dynamic dependencies of executables
- `readelf -d` - Examine ELF file dependencies
- `objdump -p` - Display object file information

## Troubleshooting

### Missing Library Errors
If you encounter errors like "libXXX.so.X: cannot open shared object file", install the corresponding package for your distribution.

### Permission Issues
Some libraries may require specific permissions or setuid binaries. Ensure your packaging grants appropriate permissions.

### Wayland Support
Electron supports both X11 and Wayland. Additional dependencies may be needed for full Wayland support in some configurations.
