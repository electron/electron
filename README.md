[![Electron Logo](https://electronjs.org/images/electron-logo.svg)](https://electronjs.org)

[![GitHub Actions Build Status](https://github.com/electron/electron/actions/workflows/build.yml/badge.svg)](https://github.com/electron/electron/actions/workflows/build.yml)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/4lggi9dpjc1qob7k/branch/main?svg=true)](https://ci.appveyor.com/project/electron-bot/electron-ljo26/branch/main)
[![Electron Discord Invite](https://img.shields.io/discord/745037351163527189?color=%237289DA&label=chat&logo=discord&logoColor=white)](https://discord.gg/electronjs)

# Electron Framework

## Available Translations
📝 Available in multiple languages: 🇨🇳 🇧🇷 🇪🇸 🇯🇵 🇷🇺 🇫🇷 🇺🇸 🇩🇪  
View these docs in other languages on our [Crowdin project](https://crowdin.com/project/electron).

## About Electron
The Electron framework lets you write cross-platform desktop applications using **JavaScript, HTML, and CSS**. It is based on **[Node.js](https://nodejs.org/)** and **[Chromium](https://www.chromium.org/)** and is used by **[Visual Studio Code](https://github.com/Microsoft/vscode/)** and many other **[apps](https://electronjs.org/apps)**.

Follow [@electronjs](https://twitter.com/electronjs) on Twitter for important announcements.

This project adheres to the **Contributor Covenant** [code of conduct](https://github.com/electron/electron/tree/main/CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to [coc@electronjs.org](mailto:coc@electronjs.org).

---

## Installation
To install prebuilt Electron binaries, use **[npm](https://docs.npmjs.com/)**. The preferred method is to install Electron as a development dependency in your app:

```sh
npm install electron --save-dev
```

For more installation options and troubleshooting tips, see the [installation guide](https://www.electronjs.org/docs/latest/tutorial/installation). For managing Electron versions in your apps, see [Electron versioning](https://www.electronjs.org/docs/latest/tutorial/electron-versioning).

---

## Platform Support
Each Electron release provides binaries for **macOS, Windows, and Linux**.

### macOS (Big Sur and newer)
- Electron provides **64-bit Intel** and **Apple Silicon (ARM)** binaries.

### Windows (Windows 10 and newer)
- Electron provides **ia32 (x86), x64 (amd64), and arm64** binaries.
- **Windows on ARM** support was added in Electron **5.0.8**.
- Support for **Windows 7, 8, and 8.1** was [removed in Electron 23, in line with Chromium's Windows deprecation policy](https://www.electronjs.org/blog/windows-7-to-8-1-deprecation-notice).

### Linux
- The prebuilt binaries of Electron are built on **Ubuntu 20.04**.
- Verified to work on:
  - **Ubuntu 18.04 and newer**
  - **Fedora 32 and newer**
  - **Debian 10 and newer**

---

## Quick Start & Electron Fiddle
### Electron Fiddle
Use **[Electron Fiddle](https://github.com/electron/fiddle)** to:
- Build, run, and package small Electron experiments.
- See code examples for all of Electron's APIs.
- Try out different versions of Electron.

### Clone a Starter Project
Alternatively, clone and run the **[electron/electron-quick-start](https://github.com/electron/electron-quick-start)** repository to see a minimal Electron app in action:

```sh
git clone https://github.com/electron/electron-quick-start
cd electron-quick-start
npm install
npm start
```

---

## Resources for Learning Electron
- 📚 [electronjs.org/docs](https://electronjs.org/docs) - All of Electron's documentation.
- 🔧 [electron/fiddle](https://github.com/electron/fiddle) - Tool to build, run, and package small Electron experiments.
- 🛠 [electron/electron-quick-start](https://github.com/electron/electron-quick-start) - A very basic starter Electron app.
- 🚀 [electronjs.org/community#boilerplates](https://www.electronjs.org/community#boilerplates) - Sample starter apps created by the community.

---

## Programmatic Usage
Most people use Electron from the command line, but if you require Electron inside your **Node.js** app (not your Electron app), it will return the file path to the binary. Use this to spawn Electron from Node scripts:

```js
const electron = require('electron');
const proc = require('node:child_process');

// will print something similar to /Users/maf/.../Electron
console.log(electron);

// spawn Electron
const child = proc.spawn(electron);
```

---

## Mirrors
### China
- [China Mirror](https://npmmirror.com/mirrors/electron/)

See the **[Advanced Installation Instructions](https://www.electronjs.org/docs/latest/tutorial/installation#mirror)** to learn how to use a custom mirror.

---

## Documentation Translations
We crowdsource translations for our documentation via **[Crowdin](https://crowdin.com/project/electron)**. We currently accept translations for:
- 🇨🇳 Chinese (Simplified)
- 🇫🇷 French
- 🇩🇪 German
- 🇯🇵 Japanese
- 🇧🇷 Portuguese
- 🇷🇺 Russian
- 🇪🇸 Spanish

---

## Contributing
Interested in **reporting/fixing issues** and **contributing to the codebase**? See **[CONTRIBUTING.md](https://github.com/electron/electron/blob/main/CONTRIBUTING.md)** for guidelines on how to get started.

---

## Community
Find information on:
- Reporting bugs 🐞
- Getting help 💡
- Finding third-party tools and sample apps 🔍

Check out the **[Community Page](https://www.electronjs.org/community)** for more details.

---

## License
**[MIT](https://github.com/electron/electron/blob/main/LICENSE)**

When using **Electron logos**, ensure you follow the **[OpenJS Foundation Trademark Policy](https://trademark-policy.openjsf.org/)**.

