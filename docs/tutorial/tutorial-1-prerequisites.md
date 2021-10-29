---
title: 'Prerequisites'
description: 'This guide will step you through the process of creating a barebones Hello World app in Electron, similar to electron/electron-quick-start.'
slug: tutorial-prerequisites
hide_title: false
---

:::info Tutorial parts
This is **part 1** of the Electron tutorial. The other parts are:

1. [Prerequisites][prerequisites]
1. [Scaffolding][scaffolding]
1. [Communicating Between Processes][main-renderer]
1. [Adding Features][features]
1. [Packaging Your Application][packaging]
1. [Publishing and Updating][updates]

:::

This tutorial will guide you through the whole end-to-end process of creating an Electron
application. By the end you will have a full-fledged Electron application ready to
distribute.

## Assumptions

Electron is a framework for building desktop applications using JavaScript,
HTML, and CSS. By embedding [Chromium][chromium] and [Node.js][node] into its
binary, Electron allows you to maintain one JavaScript codebase and create
cross-platform apps that work on Windows, macOS, and Linux. These docs assume
you are familiar with front-end web technologies and Node.js. If you are not,
we recommend the following resources:

- [Getting started with the Web (MDN)][mdn-guide]
- [Introduction to Node.js][node-guide]

Eventually, you should also read about [Electron's process model][process-model]. We will
remind you about this document throughout the tutorial when its contents become more
relevant.

## Required tools

### Git and GitHub

Git is a commonly-used **version control system** for source code, and GitHub is a collaborative
development platform built on top of it. Although neither is strictly necessary to building an Electron
application, we use GitHub's Releases platform in the final step of this tutorial.
Therefore, we'll require you to:

- [Create a GitHub account](https://github.com/join)
- [Install Git](https://github.com/git-guides/install-git)

If you're unfamiliar with how Git works, we recommend reading GitHub's [Git guides]. You can also
use the [GitHub Desktop] app (made with Electron) if you prefer a visual interface
over using the command line.

:::info Installing Git via GitHub Desktop
GitHub Desktop will install the latest version of Git on your system if you don't already have
it installed.
:::

We recommend that you create a local Git repository and publish it to GitHub before starting the tutorial,
and commit your code after every step.

### Node.js and NPM

To begin developing an Electron app, you need to install [Node.js][node-download].
We recommend that you use the latest LTS version available.

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

### Code editor

You will also need a text editor to write your code. While it really doesn't matter which
one you use, this guide has particular examples for [Visual Studio Code] that could
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
[GitHub]: https://github.com/
[Git guides]: https://github.com/git-guides/
[GitHub Desktop]: https://desktop.github.com/
[visual studio code]: https://code.visualstudio.com/

<!-- Tutorial links -->

[prerequisites]: tutorial-1-prerequisites.md
[scaffolding]: tutorial-2-scaffolding.md
[main-renderer]: tutorial-3-main-renderer.md
[features]: tutorial-4-adding-features.md
[packaging]: tutorial-5-packaging.md
[updates]: tutorial-6-publishing-updating.md
