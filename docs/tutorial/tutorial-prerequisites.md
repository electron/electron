---
title: 'Prerequisites'
description: 'This guide will step you through the process of creating a barebones Hello World app in Electron, similar to electron/electron-quick-start.'
slug: prerequisites
hide_title: false
---

This tutorial will guide you through the whole end-to-end process of creating an Electron application. At the end you will have an Electron
application, with a good foundation, and ready to be distributed.

:::info Tutorial parts
This is part 1 of the Electron tutorial. The other parts are:

1. [Prerequisites]
1. [Scaffolding][scaffolding]
1. [Main and Renderer process communication][main-renderer]
1. [Adding Features][features]
1. [Application Distribution]
1. [Code Signing]
1. [Updating Your Application][updates]

:::

This part covers all the requirements needed to start developing an
Electron application.

## Assumptions and tool dependencies

Electron is a framework for building desktop applications using JavaScript,
HTML, and CSS. By embedding [Chromium][chromium] and [Node.js][node] into its
binary, Electron allows you to maintain one JavaScript codebase and create
cross-platform apps that work on Windows, macOS, and Linux. These docs assume
you are familiar with web technologies and Node.js. If you are not, we recommend
the following resources:

- [Getting started with the Web (MDN)][mdn-guide]
- [Introduction to Node.js][node-guide]

Eventually you should read about [Electron's process model][process-model]. We will
remind you about this document through the tutorial when its contents become more
relevant.

You also need to install [Node.js][node-download]. We recommend that you
use the latest `LTS` version available.

:::tip
Please install Node.js using pre-built installers for your platform.
You may encounter incompatibility issues with different development tools otherwise.
If you are using a mac, it is recommended to use a package manager like [Homebrew] or
[nvm] to avoid any directory permissions.
:::

To check that Node.js was installed correctly, type the following commands in your
terminal client:

```sh
node -v
npm -v
```

The commands should print the versions of Node.js and npm accordingly.

You will also need an editor. While it really doesn't matter which one you use,
this guide has particular examples for [Visual Studio Code] that could
make your development easier.

<!-- Links -->

[chromium]: https://www.chromium.org/
[homebrew]: https://brew.sh/
[mdn-guide]: https://developer.mozilla.org/en-US/docs/Learn/
[node]: https://nodejs.org/
[node-guide]: https://nodejs.dev/learn
[node-download]: https://nodejs.org/en/download/
[nvm]: https://github.com/nvm-sh/nvm
[process-model]: ./process-model.md
[visual studio code]: https://code.visualstudio.com/

<!-- Tutorial links -->

[prerequisites]: tutorial-prerequisites.md
[scaffolding]: tutorial-scaffolding.md
[main-renderer]:./tutorial-main-renderer.md
[features]: ./tutorial-adding-features.md
[application distribution]: distribution-overview.md
[code signing]: code-signing.md
[updates]: updates.md
