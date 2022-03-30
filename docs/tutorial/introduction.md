# 介绍

欢迎阅读本文档! 如果你刚刚接触 Electron app的开发, 建议您从开始的章节熟悉Electron基础知识，否者可以从手册或API文档的任意章节开始。

## Electron是什么?

Electron 是通过内置[Chromium][chromium] 和 [Node.js][node] ，并使用JavaScript,
HTML和CSS来构建跨平台桌面程序的框架.无须原生开发经验，Electron帮助您基于JavaScript创建适用于Windows、macOS和Linux的桌面程序.

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
