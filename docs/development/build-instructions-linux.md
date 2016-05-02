# Build Instructions (Linux)

Follow the guidelines below for building Electron on Linux.

## Prerequisites

* At least 25GB disk space and 8GB RAM.
* Python 2.7.x. Some distributions like CentOS still use Python 2.6.x
  so you may need to check your Python version with `python -V`.
* Node.js v0.12.x. There are various ways to install Node. You can download
  source code from [Node.js](http://nodejs.org) and compile from source.
  Doing so permits installing Node on your own home directory as a standard user.
  Or try repositories such as [NodeSource](https://nodesource.com/blog/nodejs-v012-iojs-and-the-nodesource-linux-repositories).
* Clang 3.4 or later.
* Development headers of GTK+ and libnotify.

On Ubuntu, install the following libraries:

```bash
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 libnss3-dev gcc-multilib g++-multilib curl
```

On Fedora, install the following libraries:

```bash
$ sudo yum install clang dbus-devel gtk2-devel libnotify-devel libgnome-keyring-devel \
                   xorg-x11-server-utils libcap-devel cups-devel libXtst-devel \
                   alsa-lib-devel libXrandr-devel GConf2-devel nss-devel
```

Other distributions may offer similar packages for installation via package
managers such as pacman. Or one can compile from source code.

## Getting the Code

```bash
$ git clone https://github.com/electron/electron.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
the build project files. You must have Python 2.7.x for the script to succeed.
Downloading certain files can take a long time. Notice that we are using
`ninja` to build Electron so there is no `Makefile` generated.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

### Cross compilation

If you want to build for an `arm` target you should also install the following
dependencies:

```bash
$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \
                       g++-arm-linux-gnueabihf
```

And to cross compile for `arm` or `ia32` targets, you should pass the
`--target_arch` parameter to the `bootstrap.py` script:

```bash
$ ./script/bootstrap.py -v --target_arch=arm
```

## Building

If you would like to build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

This script will cause a very large Electron executable to be placed in
the directory `out/R`. The file size is in excess of 1.3 gigabytes. This
happens because the Release target binary contains debugging symbols.
To reduce the file size, run the `create-dist.py` script:

```bash
$ ./script/create-dist.py
```

This will put a working distribution with much smaller file sizes in
the `dist` directory. After running the create-dist.py script, you
may want to remove the 1.3+ gigabyte binary which is still in `out/R`.

You can also build the `Debug` target only:

```bash
$ ./script/build.py -c D
```

After building is done, you can find the `electron` debug binary under `out/D`.

## Cleaning

To clean the build files:

```bash
$ ./script/clean.py
```

## Troubleshooting

### Error While Loading Shared Libraries: libtinfo.so.5

Prebulit `clang` will try to link to `libtinfo.so.5`. Depending on the host
architecture, symlink to appropriate `libncurses`:

```bash
$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5
```

## Tests

Test your changes conform to the project coding style using:

```bash
$ npm run lint
```

Test functionality using:

```bash
$ ./script/test.py
```

## Advanced topics

The default building configuration is targeted for major desktop Linux
distributions, to build for a specific distribution or device, following
information may help you.

### Build libchromiumcontent locally

To avoid using the prebuilt binaries of libchromiumcontent, you can pass the
`--build_libchromiumcontent` switch to `bootstrap.py` script:

```bash
$ ./script/bootstrap.py -v --build_libchromiumcontent
```

Note that by default the `shared_library` configuration is not built, so you can
only build `Release` version of Electron if you use this mode:

```bash
$ ./script/build.py -c D
```
