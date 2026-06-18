<div align="center">

<img src="https://electronjs.org/images/electron-logo.svg" alt="Electron Logo" width="150" />

# Electron

### Build cross-platform desktop apps with JavaScript, HTML, and CSS

[![GitHub Actions Build Status](https://github.com/electron/electron/actions/workflows/build.yml/badge.svg)](https://github.com/electron/electron/actions/workflows/build.yml)
[![Electron Discord Invite](https://img.shields.io/discord/745037351163527189?color=%237289DA&label=chat&logo=discord&logoColor=white)](https://discord.gg/electronjs)
[![npm version](https://img.shields.io/npm/v/electron?color=%2347A3F3&logo=npm&logoColor=white)](https://www.npmjs.com/package/electron)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/electron/electron/blob/main/LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/electron/electron?style=social)](https://github.com/electron/electron)

<br />

**Electron** is a battle-tested framework for building powerful cross-platform desktop applications using web technologies — the same stack powering [Visual Studio Code](https://github.com/Microsoft/vscode/), [Slack](https://slack.com/), and [many more](https://electronjs.org/apps).

[📖 Documentation](https://electronjs.org/docs) · [🚀 Quick Start](https://electronjs.org/docs/tutorial/quick-start) · [💬 Community](https://www.electronjs.org/community) · [🐛 Report a Bug](https://github.com/electron/electron/issues/new/choose)

</div>

---

## ✨ Why Electron?

- **One codebase, three platforms** — Ship to macOS, Windows, and Linux from a single JavaScript codebase.
- **Powered by Chromium + Node.js** — Access the full Web platform and native OS APIs side-by-side.
- **Proven at scale** — Trusted by millions of users through apps like VS Code, Figma, Twitch, and Discord.
- **Rich ecosystem** — Leverage the entire npm ecosystem and Electron's own suite of tooling.
- **Active community** — Backed by the [OpenJS Foundation](https://openjsf.org/) with thousands of contributors worldwide.

---

## 🚀 Getting Started

### Installation

Install Electron as a development dependency in your project:

```sh
npm install electron --save-dev
```

Or with Yarn:

```sh
yarn add electron --dev
```

> For advanced installation options, mirrors, and troubleshooting tips, see the [Installation Guide](docs/tutorial/installation.md).
> To manage Electron versions across your apps, see [Electron Versioning](docs/tutorial/electron-versioning.md).

### Hello World

```javascript
// main.js
const { app, BrowserWindow } = require('electron')

function createWindow () {
  const win = new BrowserWindow({ width: 800, height: 600 })
  win.loadFile('index.html')
}

app.whenReady().then(createWindow)
```

Pair it with an `index.html` and you have a desktop app. It's that simple.

---

## 💻 Platform Support

Electron releases pre-built binaries for all major operating systems:

| Platform | Architectures | Minimum Version |
|----------|--------------|-----------------|
| **macOS** | Intel (x64), Apple Silicon (arm64) | macOS Monterey (12) |
| **Windows** | x86, x64, arm64 | Windows 10 |
| **Linux** | x64, arm64, armv7l | Ubuntu 18.04 / Fedora 32 / Debian 10 |

> Windows 7, 8, and 8.1 support was removed in Electron 23, in line with [Chromium's deprecation policy](https://www.electronjs.org/blog/windows-7-to-8-1-deprecation-notice).

---

## 🛠️ Programmatic Usage

You can also spawn Electron processes programmatically from a Node.js script:

```javascript
const electron = require('electron')
const { spawn } = require('node:child_process')

// Returns the path to the Electron binary
console.log(electron) // e.g. /Users/you/.../Electron

// Spawn an Electron process
const child = spawn(electron, ['path/to/your/app'])
```

---

## 🔬 Electron Fiddle

Try Electron instantly — no setup required. [**Electron Fiddle**](https://github.com/electron/fiddle) lets you:

- Build, run, and package small Electron experiments
- Browse live, runnable API examples
- Switch between Electron versions with one click

[→ Download Electron Fiddle](https://www.electronjs.org/fiddle)

---

## 📚 Resources

| Resource | Description |
|----------|-------------|
| [Official Docs](https://electronjs.org/docs) | Complete API reference and tutorials |
| [Electron Fiddle](https://github.com/electron/fiddle) | Interactive sandbox for experiments |
| [Community Boilerplates](https://electronjs.org/community#boilerplates) | Starter templates from the community |
| [Blog](https://electronjs.org/blog) | Release notes and in-depth articles |
| [Community Page](https://www.electronjs.org/community) | Forums, tools, and third-party resources |

---

## 🌍 Internationalization

Electron's documentation is available in multiple languages via [Crowdin](https://crowdin.com/project/electron):

🇨🇳 Chinese (Simplified) · 🇧🇷 Portuguese · 🇪🇸 Spanish · 🇯🇵 Japanese · 🇷🇺 Russian · 🇫🇷 French · 🇩🇪 German

We welcome translation contributions! Visit our [Crowdin project](https://crowdin.com/project/electron) to get started.

---

## 🤝 Contributing

We love contributions from the community! Here's how you can get involved:

1. **Report bugs** — Open an issue on [GitHub](https://github.com/electron/electron/issues/new/choose).
2. **Fix bugs / add features** — Read [CONTRIBUTING.md](CONTRIBUTING.md) for our development workflow.
3. **Improve docs** — Help us make the documentation clearer and more complete.
4. **Translate** — Contribute translations on [Crowdin](https://crowdin.com/project/electron).

> This project follows the Contributor Covenant [Code of Conduct](CODE_OF_CONDUCT.md).
> Please report unacceptable behavior to [coc@electronjs.org](mailto:coc@electronjs.org).

---

## 🔒 Security

Found a vulnerability? Please **do not** open a public issue. Instead, follow our [Security Policy](SECURITY.md) for responsible disclosure. We take security seriously and will respond promptly.

---

## 📜 License

[MIT](https://github.com/electron/electron/blob/main/LICENSE) © [Electron Community](https://electronjs.org)

When using Electron logos, please follow the [OpenJS Foundation Trademark Policy](https://trademark-policy.openjsf.org/).

---

<div align="center">

Follow [@electronjs](https://twitter.com/electronjs) on Twitter/X for announcements and updates.

<br />

⭐ **Star this repo if Electron powers your app!** ⭐

</div>
