# Introduction

Welcome to the Electron documentation! If this is your first time developing
an Electron app, read through this Getting Started section to get familiar with the
basics. Otherwise, feel free to explore our guides and API documentation!

## What is Electron?

Electron is a framework for building desktop applications using JavaScript,
HTML, and CSS. By embedding [Chromium][chromium] and [Node.js][node] into its
binary, Electron allows you to maintain one JavaScript codebase and create
cross-platform apps that work on Windows, macOS, and Linux — no native development
experience required.

## Prerequisites

These docs operate under the assumption that the reader is familiar with both
Node.js and general web development. If you need to get more comfortable with
either of these areas, we recommend the following resources:

* [Getting started with the Web (MDN)][mdn-guide]
* [Introduction to Node.js][node-guide]

Moreover, you'll have a better time understanding how Electron works if you get
acquainted with Chromium's process model. You can get a brief overview of
Chrome architecture with the [Chrome comic][comic], which was released alongside
Chrome's launch back in 2008. Although it's been over a decade since then, the
core principles introduced in the comic remain helpful to understand Electron.

## Running examples with Electron Fiddle

[Electron Fiddle][fiddle] is a sandbox app written with Electron and supported by
Electron's maintainers. We highly recommend installing it as a learning tool to
experiment with Electron's APIs or to prototype features during development.

Fiddle also integrates nicely with our documentation. When browsing through examples
in our tutorials, you'll frequently see an "Open in Electron Fiddle" button underneath
a code block. If you have Fiddle installed, this button will open a
`fiddle.electronjs.org` link that will automatically load the example into Fiddle,
no copy-pasting required.

## Getting help

Are you getting stuck anywhere? Here are a few links to places to look:

* If you need help with developing your app, our [community Discord server][discord]
is a great place to get advice from other Electron app developers.
* If you suspect you're running into a bug with the `electron` package, please check
the [GitHub issue tracker][issue-tracker] to see if any existing issues match your
problem. If not, feel free to fill out our bug report template and submit a new issue.

[chromium]: https://www.chromium.org/
[node]: https://nodejs.org/
[mdn-guide]: https://developer.mozilla.org/en-US/docs/Learn/Getting_started_with_the_web
[node-guide]: https://nodejs.dev/learn
[comic]: https://www.google.com/googlebooks/chrome/
[fiddle]: https://electronjs.org/fiddle
[issue-tracker]: https://github.com/electron/electron/issues
[discord]: https://discord.gg/electronjs
