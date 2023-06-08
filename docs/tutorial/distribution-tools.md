---
title: 'Alternative distribution tools'
description: "There are multiple tools for handling distribution of your Electron apps. This page lists them and gives brief comparisons."
slug: alternative-distribution-tools
hide_title: false
---

The official Electron packaging tool is [Electron Forge](https://electronforge.io/), but there are also other ways to distribute your app.

## Electron Builder

A "complete solution to package and build a ready-for-distribution Electron app"
that focuses on an integrated experience. [`electron-builder`](https://github.com/electron-userland/electron-builder) adds one
single dependency focused on simplicity and manages all further requirements
internally.

`electron-builder` replaces features and modules used by the Electron
maintainers (such as the auto-updater) with custom ones. They are generally
tighter integrated but will have less in common with popular Electron apps
like Atom, Visual Studio Code, or Slack.

You can find more information and documentation in [the repository](https://github.com/electron-userland/electron-builder).

## Hydraulic Conveyor

A [desktop app deployment tool](https://www.hydraulic.dev) that supports
cross-building/signing of all packages from any OS without the need for
multi-platform CI, can do synchronous web-style updates on each start
of the app, requires no code changes, can use plain HTTP servers for updates and
which focuses on ease of use. Conveyor replaces the Electron auto-updaters
with Sparkle on macOS, MSIX on Windows and Linux package repositories.

This is a commercial tool that's free for open source projects. There's
an example of [how to package GitHub Desktop](https://hydraulic.dev/blog/8-packaging-electron-apps.html)
which can be used for learning.

## UpdateRocks

[UpdateRocks](https://www.update.rocks/) supplies update servers for use
with the `autoUpdater` module. It uses GitHub as a backend and can integrate
with CI. UpdateRocks is a commercial SaaS that's free for open source projects.
