[![Electron Logo](https://electron.atom.io/images/electron-logo.svg)](https://electron.atom.io/)

[![Travis Build Status](https://travis-ci.org/electron/electron.svg?branch=master)](https://travis-ci.org/electron/electron)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/bc56v83355fi3369/branch/master?svg=true)](https://ci.appveyor.com/project/electron-bot/electron/branch/master)
[![devDependency Status](https://david-dm.org/electron/electron/dev-status.svg)](https://david-dm.org/electron/electron?type=dev)
[![Join the Electron Community on Slack](http://atom-slack.herokuapp.com/badge.svg)](http://atom-slack.herokuapp.com/)

:memo: Available Translations: [Korean](https://github.com/electron/electron/tree/master/docs-translations/ko-KR/project/README.md) | [Simplified Chinese](https://github.com/electron/electron/tree/master/docs-translations/zh-CN/project/README.md) | [Brazilian Portuguese](https://github.com/electron/electron/tree/master/docs-translations/pt-BR/project/README.md) | [Traditional Chinese](https://github.com/electron/electron/tree/master/docs-translations/zh-TW/project/README.md) | [Spanish](https://github.com/electron/electron/tree/master/docs-translations/es/project/README.md) | [Turkish](https://github.com/electron/electron/tree/master/docs-translations/tr-TR/project/README.md) | [German](https://github.com/electron/electron/tree/master/docs-translations/de-DE/project/README.md)

The Electron framework lets you write cross-platform desktop applications
using JavaScript, HTML and CSS. It is based on [Node.js](https://nodejs.org/) and
[Chromium](http://www.chromium.org) and is used by the [Atom
editor](https://github.com/atom/atom) and many other [apps](https://electron.atom.io/apps).

Follow [@ElectronJS](https://twitter.com/electronjs) on Twitter for important
announcements.

This project adheres to the Contributor Covenant 
[code of conduct](https://github.com/electron/electron/tree/master/CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to [electron@github.com](mailto:electron@github.com).

## Installation

To install prebuilt Electron binaries, use [`npm`](https://docs.npmjs.com/).
The preferred method is to install Electron as a development dependency in your
app:

```sh
npm install electron --save-dev --save-exact
```

The `--save-exact` flag is recommended as Electron does not follow semantic
versioning. For info on how to manage Electron versions in your apps, see
[Electron versioning](https://electron.atom.io/docs/tutorial/electron-versioning/).

For more installation options and troubleshooting tips, see
[installation](https://electron.atom.io/docs/tutorial/installation/).

## Quick Start

Clone and run the 
[electron/electron-quick-start](https://github.com/electron/electron-quick-start)
repository to see a minimal Electron app in action:

```
git clone https://github.com/electron/electron-quick-start
cd electron-quick-start
npm install
npm start
```

## Resources for Learning Electron

- [electron.atom.io/docs](http://electron.atom.io/docs) - all of Electron's documentation
- [electron/electron-quick-start](https://github.com/electron/electron-quick-start) - a very basic starter Electron app
- [electron.atom.io/community/#boilerplates](http://electron.atom.io/community/#boilerplates) - sample starter apps created by the community
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

- [Brazilian Portuguese](https://github.com/electron/electron/tree/master/docs-translations/pt-BR)
- [Korean](https://github.com/electron/electron/tree/master/docs-translations/ko-KR)
- [Japanese](https://github.com/electron/electron/tree/master/docs-translations/jp)
- [Spanish](https://github.com/electron/electron/tree/master/docs-translations/es)
- [Simplified Chinese](https://github.com/electron/electron/tree/master/docs-translations/zh-CN)
- [Traditional Chinese](https://github.com/electron/electron/tree/master/docs-translations/zh-TW)
- [Turkish](https://github.com/electron/electron/tree/master/docs-translations/tr-TR)
- [Thai](https://github.com/electron/electron/tree/master/docs-translations/th-TH)
- [Ukrainian](https://github.com/electron/electron/tree/master/docs-translations/uk-UA)
- [Russian](https://github.com/electron/electron/tree/master/docs-translations/ru-RU)
- [French](https://github.com/electron/electron/tree/master/docs-translations/fr-FR)
- [Indonesian](https://github.com/electron/electron/tree/master/docs-translations/id)

## Community

You can ask questions and interact with the community in the following
locations:
- [`electron`](http://discuss.atom.io/c/electron) category on the Atom
forums
- `#atom-shell` channel on Freenode
- [`Atom`](http://atom-slack.herokuapp.com/) channel on Slack
- [`electron-ru`](https://telegram.me/electron_ru) *(Russian)*
- [`electron-br`](https://electron-br.slack.com) *(Brazilian Portuguese)*
- [`electron-kr`](http://www.meetup.com/electron-kr/) *(Korean)*
- [`electron-jp`](https://electron-jp.slack.com) *(Japanese)*
- [`electron-tr`](http://electron-tr.herokuapp.com) *(Turkish)*
- [`electron-id`](https://electron-id.slack.com) *(Indonesia)*

Check out [awesome-electron](https://github.com/sindresorhus/awesome-electron)
for a community maintained list of useful example apps, tools and resources.

## License

[MIT](https://github.com/electron/electron/blob/master/LICENSE)

When using the Electron or other GitHub logos, be sure to follow the [GitHub logo guidelines](https://github.com/logos).
