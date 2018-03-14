[![Electron Logo](https://electronjs.org/images/electron-logo.svg)](https://electronjs.org)

[![Travis Build Status](https://travis-ci.org/electron/electron.svg?branch=master)](https://travis-ci.org/electron/electron)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/bc56v83355fi3369/branch/master?svg=true)](https://ci.appveyor.com/project/electron-bot/electron/branch/master)
[![devDependency Status](https://david-dm.org/electron/electron/dev-status.svg)](https://david-dm.org/electron/electron?type=dev)
[![Join the Electron Community on Slack](https://atom-slack.herokuapp.com/badge.svg)](https://atom-slack.herokuapp.com/)

:memo: Available Translations: 🇨🇳 🇹🇼 🇧🇷 🇪🇸 🇰🇷 🇯🇵 🇷🇺 🇫🇷 🇹🇭 🇳🇱 🇹🇷 🇮🇩 🇺🇦 🇨🇿 🇮🇹.
View these docs in other languages at [electron/electron-i18n](https://github.com/electron/electron-i18n/tree/master/content/).

The Electron framework lets you write cross-platform desktop applications
using JavaScript, HTML and CSS. It is based on [Node.js](https://nodejs.org/) and
[Chromium](https://www.chromium.org) and is used by the [Atom
editor](https://github.com/atom/atom) and many other [apps](https://electronjs.org/apps).

Follow [@ElectronJS](https://twitter.com/electronjs) on Twitter for important
announcements.

This project adheres to the Contributor Covenant
[code of conduct](https://github.com/electron/electron/tree/master/CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to [coc@electronjs.org](mailto:coc@electronjs.org).

## Installation

To install prebuilt Electron binaries, use [`npm`](https://docs.npmjs.com/).
The preferred method is to install Electron as a development dependency in your
app:

```sh
npm install electron --save-dev --save-exact
```

The `--save-exact` flag is recommended as Electron does not follow semantic
versioning. For info on how to manage Electron versions in your apps, see
[Electron versioning](https://electronjs.org/docs/tutorial/electron-versioning).

For more installation options and troubleshooting tips, see
[installation](https://electronjs.org/docs/tutorial/installation).

## Quick start

Clone and run the
[electron/electron-quick-start](https://github.com/electron/electron-quick-start)
repository to see a minimal Electron app in action:

```sh
git clone https://github.com/electron/electron-quick-start
cd electron-quick-start
npm install
npm start
```

## Resources for learning Electron

- [electronjs.org/docs](https://electronjs.org/docs) - all of Electron's documentation
- [electron/electron-quick-start](https://github.com/electron/electron-quick-start) - a very basic starter Electron app
- [electronjs.org/community#boilerplates](https://electronjs.org/community#boilerplates) - sample starter apps created by the community
- [electron/simple-samples](https://github.com/electron/simple-samples) - small applications with ideas for taking them further
- [electron/electron-api-demos](https://github.com/electron/electron-api-demos) - an Electron app that teaches you how to use Electron
- [hokein/electron-sample-apps](https://github.com/hokein/electron-sample-apps) - small demo apps for the various Electron APIs

## Programmatic usage

Most people use Electron from the command line, but if you require `electron` inside
your **Node app** (not your Electron app) it will return the file path to the
binary. Use this to spawn Electron from Node scripts:

```javascript
const electron = require('electron')
const proc = require('child_process')

// will print something similar to /Users/maf/.../Electron
console.log(electron)

// spawn Electron
const child = proc.spawn(electron)
```

### Mirrors

- [China](https://npm.taobao.org/mirrors/electron)

## Documentation Translations

Find documentation translations in [electron/electron-i18n](https://github.com/electron/electron-i18n).

## Community

You can ask questions and interact with the community in the following
locations:
- [`electron`](https://discuss.atom.io/c/electron) category on the Atom
forums
- `#atom-shell` channel on Freenode
- [`Atom`](https://atom-slack.herokuapp.com) channel on Slack
- [`electron-ru`](https://telegram.me/electron_ru) *(Russian)*
- [`electron-br`](https://electron-br.slack.com) *(Brazilian Portuguese)*
- [`electron-kr`](https://electron-kr.github.io/electron-kr) *(Korean)*
- [`electron-jp`](https://electron-jp.slack.com) *(Japanese)*
- [`electron-tr`](https://electron-tr.herokuapp.com) *(Turkish)*
- [`electron-id`](https://electron-id.slack.com) *(Indonesia)*

Check out [awesome-electron](https://github.com/sindresorhus/awesome-electron)
for a community maintained list of useful example apps, tools and resources.

## License

[MIT](https://github.com/electron/electron/blob/master/LICENSE)

When using the Electron or other GitHub logos, be sure to follow the [GitHub logo guidelines](https://github.com/logos).
