# Snapcraft Guide (Ubuntu Software Center & More)

This guide provides information on how to package your Electron application
for any Snapcraft environment, including the Ubuntu Software Center.

## Background and Requirements

Together with the broader Linux community, Canonical aims to fix many of the
common software installation problems with the [`snapcraft`](snapcraft.io)
project. Snaps are containerized software packages that include required
dependencies, auto-update, and work on all major Linux distributions without
system modification.

There are three ways to create a `.snap` file:

1) Using `electron-installer-snap`, which takes `electron-packager's` output
2) Using an already created `.deb` package
3) Using [`electron-forge`](electron-forge) or
   [`electron-builder`](electron-builder), both tools that come with `snap`
   support out of the box. This is the easiest option.

In all cases, you will need to have the `snapcraft` tool installed. We
recommend building on Ubuntu 16.04 (or the current LTS).

```sh
snap install snapcraft --classic
```

While it _is possible_ to install `snapcraft` on macOS using Homebrew, you are
less likely to encounter issues when running `snapcraft` on an actual Linux
distribution. As of today, it is not able ot build `snap` packages, for instance.

# Using `electron-installer-snap`

The module works like `electron-winstaller` and similar modules in that its
scope is limited to building snap packages. You can install it with:

```sh
npm install --save-dev electron-installer-snap
```

## Step 1: Package Your Electron Application

Package the application using [electron-packager](electron-packager) (or a
similar tool). Make sure to remove `node_modules` that you don't need in your
final application, since any module you don't actually need will just increase
your application's size.

The output should look roughly like this:

```text
.
└── dist
    └── app-linux-x64
        ├── LICENSE
        ├── LICENSES.chromium.html
        ├── content_shell.pak
        ├── app
        ├── icudtl.dat
        ├── libgcrypt.so.11
        ├── libnode.so
        ├── locales
        ├── natives_blob.bin
        ├── resources
        ├── snapshot_blob.bin
        └── version
```

## Step 2: Running `electron-installer-snap`

From a terminal that has `snapcraft` in its `PATH`, run `electron-installer-snap`
with the only required parameter `--src`, which is the location of your packaged
Electron application created in the first step.

```sh
npx electron-installer-snap --src=out/myappname-linux-x64
```

If you have an existing build pipeline, you can use `electron-installer-snap`
programmatically. For more information, see the [API docs](snapcraft-syntax).

```js
const snap = require('electron-installer-snap')

snap(options)
  .then(snapPath => console.log(`Created snap at ${snapPath}!`))
```

# Using an Existing Debian Package

Snapcraft is capable of taking an existing `.deb` file and turning it into
a `.snap` file. The creation of a snap is configured using a `snapcraft.yaml`
file that describes the sources, dependencies, description, and other core
building blocks.

## Step 1: Create a Debian Package

If you do not already have a `.deb` package, using `electron-installer-snap`
might be an easier path to create snap packages. However, multiple solutions
for creating Debian packages exist, including [`electron-forge`](electron-forge),
[`electron-builder`](electron-builder) or
[`electron-installer-debian`](electron-installer-debian).

## Step 2: Create a snapcraft.yaml

For more information on the available configuration options, see the
[documentation on the snapcraft syntax](https://docs.snapcraft.io/build-snaps/syntax).
Let's look at an example:

```yaml
name: myApp
version: 2.0.0
summary: A little description for the app.
description: |
 You know what? This app is amazing! It does all the things
 for you. Some say it keeps you young, maybe even happy.

grade: stable
confinement: classic

parts:
  slack:
    plugin: dump
    source: my-deb.deb
    source-type: deb
    after:
      - desktop-gtk2
    build-packages:
      - patchelf
    stage-packages:
      - libasound2
      - libgconf2-4
      - libnotify4
      - libnspr4
      - libnss3
      - libpcre3
      - libpulse0
      - libxss1
      - libxtst6
  electron-launch:
    plugin: dump
    source: files/
    prepare: |
      chmod +x bin/electron-launch

apps:
  myApp:
    command: bin/electron-launch $SNAP/usr/lib/myApp/myApp
    desktop: usr/share/applications/myApp.desktop
    # Correct the TMPDIR path for Chromium Framework/Electron to ensure
    # libappindicator has readable resources.
    environment:
      TMPDIR: $XDG_RUNTIME_DIR
```

As you can see, the `snapcraft.yaml` instructs the system to launch a file
called `electron-launch`. In this example, it simply passes information on
to the app's binary:

```sh
#!/bin/sh

exec "$@" --executed-from="$(pwd)" --pid=$$ > /dev/null 2>&1 &
```

[snapcraft.io]: https://snapcraft.io/
[snapcraft-store]: https://snapcraft.io/store/
[snapcraft-syntax]: https://docs.snapcraft.io/build-snaps/syntax
[electron-packager]: https://github.com/electron-userland/electron-packager
[electron-forge]: https://github.com/electron-userland/electron-forge
[electron-builder]: https://github.com/electron-userland/electron-builder
[electron-installer-debian]: https://github.com/unindented/electron-installer-debian
