# Build Instructions (Linux)

Follow the guidelines below for building **Electron itself** on Linux, for the purposes of creating custom Electron binaries. For bundling and distributing your app code with the prebuilt Electron binaries, see the [application distribution][application-distribution] guide.

[application-distribution]: ../tutorial/application-distribution.md

## Prerequisites

* At least 25GB disk space and 8GB RAM.
* Python >= 3.9.
* [Node.js](https://nodejs.org/download/) >= 22.12.0
* [clang](https://clang.llvm.org/get_started.html) 3.4 or later.
* Development headers of GTK 3 and libnotify.

Note: libXScrnSaver (libxss) is not required for Electron apps, as powerMonitor uses DBus for idle detection. It has been removed from the prerequisites to avoid issues on distributions like RHEL10 where it is unavailable.

On Ubuntu >= 20.04, install the following libraries:

```sh
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk-3-dev \
                       libnotify-dev libasound2-dev libcap-dev \
                       libcups2-dev libxtst-dev \
                       libnss3-dev gcc-multilib g++-multilib curl \
                       gperf bison python3-dbusmock openjdk-8-jre
```

On Ubuntu < 20.04, install the following libraries:

```sh
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk-3-dev \
                       libnotify-dev libgnome-keyring-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 libnss3-dev gcc-multilib g++-multilib curl \
                       gperf bison python-dbusmock openjdk-8-jre
```

On RHEL / CentOS, install the following libraries:

```sh
$ sudo yum install clang dbus-devel gtk3-devel libnotify-devel \
                   libgnome-keyring-devel xorg-x11-server-utils libcap-devel \
                   cups-devel libXtst-devel alsa-lib-devel libXrandr-devel \
                   nss-devel python-dbusmock openjdk-8-jre
```

On Fedora, install the following libraries:

```sh
$ sudo dnf install clang dbus-devel gperf gtk3-devel \
                   libnotify-devel libgnome-keyring-devel libcap-devel \
                   cups-devel libXtst-devel alsa-lib-devel libXrandr-devel \
                   nss-devel python-dbusmock
```

On Arch Linux / Manjaro, install the following libraries:

```sh
$ sudo pacman -Syu base-devel clang libdbus gtk2 libnotify \
                   libgnome-keyring alsa-lib libcap libcups libxtst \
                   libxss nss gcc-multilib curl gperf bison \
                   python2 python-dbusmock jdk8-openjdk
```

Other distributions may offer similar packages for installation via package
managers such as pacman. Or one can compile from source code.

### Cross compilation

If you want to build for an `arm` target you should also install the following
dependencies:

```sh
$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \
                       g++-arm-linux-gnueabihf
```

Similarly for `arm64`, install the following:

```sh
$ sudo apt-get install libc6-dev-arm64-cross linux-libc-dev-arm64-cross \
                       g++-aarch64-linux-gnu
```

And to cross-compile for `arm` or targets, you should pass the
`target_cpu` parameter to `gn gen`:

```sh
$ gn gen out/Testing --args='import(...) target_cpu="arm"'
```

## Building

See [Build Instructions: GN](build-instructions-gn.md)

## Troubleshooting

### Error While Loading Shared Libraries: libtinfo.so.5

Prebuilt `clang` will try to link to `libtinfo.so.5`. Depending on the host
architecture, symlink to appropriate `libncurses`:

```sh
$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5
```

## Advanced topics

The default building configuration is targeted for major desktop Linux
distributions. To build for a specific distribution or device, the following
information may help you.

### Using system `clang` instead of downloaded `clang` binaries

By default Electron is built with prebuilt
[`clang`](https://clang.llvm.org/get_started.html) binaries provided by the
Chromium project. If for some reason you want to build with the `clang`
installed in your system, you can specify the `clang_base_path` argument in the
GN args.

For example if you installed `clang` under `/usr/local/bin/clang`:

```sh
$ gn gen out/Testing --args='import("//electron/build/args/testing.gn") clang_base_path = "/usr/local/bin"'
```

### Using compilers other than `clang`

Building Electron with compilers other than `clang` is not supported.

## Runtime Dependencies for Electron Apps

When packaging Electron apps for Linux distributions, ensure the following runtime libraries are available. These are derived from Chromium's dependencies and are required for Electron apps to function correctly. Package names vary by distribution.

### Core GUI and System Libraries
- **GTK 3** (`libgtk-3-0` on Ubuntu/Debian, `gtk3` on RHEL/Fedora): GUI rendering, window management.
- **DBus** (`libdbus-1-3` on Ubuntu/Debian, `dbus-libs` on RHEL/Fedora): Inter-process communication (used by powerMonitor, notifications).
- **libnotify** (`libnotify4` on Ubuntu/Debian, `libnotify` on RHEL/Fedora): Desktop notifications.

### Audio and Multimedia
- **ALSA** (`libasound2` on Ubuntu/Debian, `alsa-lib` on RHEL/Fedora): Audio support.

### Printing and Capabilities
- **CUPS** (`libcups2` on Ubuntu/Debian, `cups-libs` on RHEL/Fedora): Printing support.
- **libcap** (`libcap2` on Ubuntu/Debian, `libcap` on RHEL/Fedora): POSIX capabilities.

### X11 and Display
- **libXtst** (`libxtst6` on Ubuntu/Debian, `libXtst` on RHEL/Fedora): X11 testing and input simulation.
- **libXrandr** (`libxrandr2` on Ubuntu/Debian, `libXrandr` on RHEL/Fedora): X11 display configuration.
- **X11 core** (`libx11-6`, `libxcb1`, etc. on Ubuntu/Debian; `libX11`, `libxcb` on RHEL/Fedora): Basic X11 integration.

### Security and Networking
- **NSS** (`libnss3` on Ubuntu/Debian, `nss` on RHEL/Fedora): SSL/TLS and cryptography.

### Optional/Legacy
- **libgnome-keyring** (`libgnome-keyring0` on Ubuntu/Debian, `libgnome-keyring` on RHEL/Fedora): Secure credential storage (distribution-dependent).

### Not Required
- **libXScrnSaver** (`libxss1` on Ubuntu/Debian, `libXScrnSaver` on RHEL/Fedora): Not needed; powerMonitor uses DBus for idle detection. Omit to avoid issues on distributions like RHEL10 where it's unavailable.
