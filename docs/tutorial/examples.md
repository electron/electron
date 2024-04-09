---
title: 'Examples Overview'
description: 'A set of examples for common Electron features'
slug: examples
hide_title: false
---

# Examples Overview

In this section, we have collected a set of guides for common features
that you may want to implement in your Electron application. Each guide
contains a practical example in a minimal, self-contained example app.
The easiest way to run these examples is by downloading [Electron Fiddle][fiddle].

Once Fiddle is installed, you can press on the "Open in Fiddle" button that you
will find below code samples like the following one:

```fiddle docs/fiddles/quick-start

```

If there is still something that you do not know how to do, please take a look at the [API][app]
as there is a chance it might be documented just there (and also open an issue requesting the
guide!).

<!-- guide-table-start -->

| Guide                 | Description                                                                                                         |
| :-------------------- | ------------------------------------------------------------------------------------------------------------------- |
| [Message ports][]       | This guide provides some examples of how you might use MessagePorts in your app to communicate different processes. |
| [Device access][]       | Learn how to access the device hardware (Bluetooth, USB, Serial).                                                   |
| [Keyboard shortcuts][]  | Configure local and global keyboard shortcuts for your Electron application.                                        |
| [Multithreading][]      | With Web Workers, it is possible to run JavaScript in OS-level threads                                              |
| [Offscreen rendering][] | Offscreen rendering lets you obtain the content of a BrowserWindow in a bitmap, so it can be rendered anywhere.     |
| [Spellchecker][]        | Learn how to use the built-in spellchecker, set languages, etc.                                                     |
| [Web embeds][]          | Discover the different ways to embed third-party web content in your application.                                   |

<!-- guide-table-end -->

## How to...?

You can find the full list of "How to?" in the sidebar. If there is
something that you would like to do that is not documented, please join
our [Discord server][discord] and let us know!

[app]: ../api/app.md
[discord]: https://discord.gg/electronjs
[fiddle]: https://www.electronjs.org/fiddle
[Message ports]: ./message-ports.md
[Device access]: ./devices.md
[Keyboard shortcuts]: ./keyboard-shortcuts.md
[Multithreading]: ./multithreading.md
[Offscreen rendering]: ./offscreen-rendering.md
[Spellchecker]: ./spellchecker.md
[Web embeds]: ./web-embeds.md
