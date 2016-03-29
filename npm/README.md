# electron-prebuilt

[![build status](http://img.shields.io/travis/electron-userland/electron-prebuilt.svg?style=flat)](http://travis-ci.org/electron-userland/electron-prebuilt)

[![badge](https://nodei.co/npm/electron-prebuilt.png?downloads=true)](https://www.npmjs.com/package/electron-prebuilt)

Install [electron](https://github.com/atom/electron) prebuilt binaries for command-line use using npm. This module helps you easily install the `electron` command for use on the command line without having to compile anything.

Electron is a JavaScript runtime that bundles Node.js and Chromium. You use it similar to the `node` command on the command line for executing JavaScript programs. For more info you can read [this intro blog post](http://maxogden.com/electron-fundamentals.html) or dive into the [Electron documentation](https://github.com/atom/electron/tree/master/docs)

## Installation

Download and install the latest build of electron for your OS and add it to your projects `package.json` as a `devDependency`:

```
npm install electron-prebuilt --save-dev
```

This is the preferred way to use electron, as it doesn't require users to install electron globally.

You can also use the `-g` flag (global) to symlink it into your PATH:

```
npm install -g electron-prebuilt
```

If that command fails with an `EACCESS` error you may have to run it again with `sudo`:

```
sudo npm install -g electron-prebuilt
```

Now you can just run `electron` to run electron:

```
electron
```

If you need to use an HTTP proxy you can [set these environment variables](https://github.com/request/request/tree/f0c4ec061141051988d1216c24936ad2e7d5c45d#controlling-proxy-behaviour-using-environment-variables)

If you want to change the architecture that is downloaded (e.g., `ia32` on an `x64` machine), you can use the `--arch` flag with npm install or set the `npm_config_arch` environment variable:
```
npm install --arch=ia32 electron-prebuilt
```

## About

Works on Mac, Windows and Linux OSes that Electron supports (e.g. Electron [does not support Windows XP](https://github.com/atom/electron/issues/691)).

The version numbers of this module match the version number of the [offical Electron releases](https://github.com/atom/electron/releases), which do not follow [semantic versioning](http://semver.org/).

This module is automatically released whenever a new version of Electron is released thanks to [electron-prebuilt-updater](https://github.com/johnmuhl/electron-prebuilt-updater) written by [John Muhl](https://github.com/johnmuhl/).

## Usage

First you have to [write an electron application](https://github.com/atom/electron/blob/master/docs/tutorial/quick-start.md)

Then you can run your app using:

```
electron your-app/
```

## Related modules

- [electron-packager](https://github.com/electron-userland/electron-packager) - package and distribute your electron app in OS executables (.app, .exe etc)
- [electron-builder](https://github.com/loopline-systems/electron-builder) - create installers for Windows and OS X. It's built to work together with electron-packager
- [menubar](https://github.com/maxogden/menubar) - high level way to create menubar desktop applications with electron

Find more at the [awesome-electron](https://github.com/sindresorhus/awesome-electron) list

## Programmatic usage

Most people use this from the command line, but if you require `electron-prebuilt` inside your node app it will return the file path to the binary.
Use this to spawn electron from node scripts.

``` js
var electron = require('electron-prebuilt')
var proc = require('child_process')

// will something similar to print /Users/maf/.../Electron
console.log(electron)

// spawn electron
var child = proc.spawn(electron)
```
