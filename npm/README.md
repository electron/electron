# electron-prebuilt

[![Travis build status](http://img.shields.io/travis/electron-userland/electron-prebuilt.svg?style=flat)](http://travis-ci.org/electron-userland/electron-prebuilt)
[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/qd978ky9axl8m1m1?svg=true)](https://ci.appveyor.com/project/Atom/electron-prebuilt)

[![badge](https://nodei.co/npm/electron-prebuilt.png?downloads=true)](https://www.npmjs.com/package/electron-prebuilt)

Install [Electron](https://github.com/electron/electron) prebuilt binaries for
command-line use using npm. This module helps you easily install the `electron`
command for use on the command line without having to compile anything.

[Electron](http://electron.atom.io) is a JavaScript runtime that bundles Node.js
and Chromium. You use it similar to the `node` command on the command line for
executing JavaScript programs. For more info you can read [this intro blog post](http://maxogden.com/electron-fundamentals.html)
or dive into the [Electron documentation](http://electron.atom.io/docs).

## Installation

**Note** As of version 1.3.1, this package is published to npm under two names:
`electron` and `electron-prebuilt`. You can currently use either name, but
`electron` is recommended, as the `electron-prebuilt` name is deprecated, and
will only be published until the end of 2016.

Download and install the latest build of Electron for your OS and add it to your
project's `package.json` as a `devDependency`:

```shell
npm install electron --save-dev
```

This is the preferred way to use Electron, as it doesn't require users to
install Electron globally.

You can also use the `-g` flag (global) to symlink it into your PATH:

```shell
npm install -g electron
```

If that command fails with an `EACCESS` error you may have to run it again with `sudo`:

```shell
sudo npm install -g electron
```

Now you can just run `electron` to run electron:

```shell
electron
```

If you need to use an HTTP proxy you can [set these environment variables](https://github.com/request/request/tree/f0c4ec061141051988d1216c24936ad2e7d5c45d#controlling-proxy-behaviour-using-environment-variables).

If you want to change the architecture that is downloaded (e.g., `ia32` on an
`x64` machine), you can use the `--arch` flag with npm install or set the
`npm_config_arch` environment variable:

```shell
npm install --arch=ia32 electron
```

## About

Works on Mac, Windows and Linux OSes that Electron supports (e.g. Electron
[does not support Windows XP](https://github.com/electron/electron/issues/691)).

The version numbers of this module match the version number of the [official
Electron releases](https://github.com/electron/electron/releases), which
[do not follow semantic versioning](http://electron.atom.io/docs/tutorial/electron-versioning/).

This module is automatically released whenever a new version of Electron is
released thanks to [electron-prebuilt-updater](https://github.com/electron/electron-prebuilt-updater),
originally written by [John Muhl](https://github.com/johnmuhl/).

## Usage

First, you have to [write an Electron application](http://electron.atom.io/docs/tutorial/quick-start/).

Then, you can run your app using:

```shell
electron your-app/
```

## Related modules

- [electron-packager](https://github.com/electron-userland/electron-packager) -
  Package and distribute your Electron app with OS-specific bundles
  (.app, .exe etc)
- [electron-builder](https://github.com/electron-userland/electron-builder) -
  create installers
- [menubar](https://github.com/maxogden/menubar) - high level way to create
  menubar desktop applications with electron

Find more at the [awesome-electron](https://github.com/sindresorhus/awesome-electron) list.

## Programmatic usage

Most people use this from the command line, but if you require `electron` inside
your **Node app** (not your Electron app) it will return the file path to the
binary. Use this to spawn Electron from Node scripts:

```javascript
var electron = require('electron')
var proc = require('child_process')

// will something similar to print /Users/maf/.../Electron
console.log(electron)

// spawn Electron
var child = proc.spawn(electron)
```
