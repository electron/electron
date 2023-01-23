---
title: 'Adding Features'
description: 'In this step of the tutorial, we will share some resources you should read to add features to your application'
slug: tutorial-adding-features
hide_title: false
---

:::info Follow along the tutorial

This is **part 4** of the Electron tutorial.

1. [Prerequisites][prerequisites]
1. [Building your First App][building your first app]
1. [Using Preload Scripts][preload]
1. **[Adding Features][features]**
1. [Packaging Your Application][packaging]
1. [Publishing and Updating][updates]

:::

## Adding application complexity

If you have been following along, you should have a functional Electron application
with a static user interface. From this starting point, you can generally progress
in developing your app in two broad directions:

1. Adding complexity to your renderer process' web app code
1. Deeper integrations with the operating system and Node.js

It is important to understand the distinction between these two broad concepts. For the
first point, Electron-specific resources are not necessary. Building a pretty to-do
list in Electron is just pointing your Electron BrowserWindow to a pretty
to-do list web app. Ultimately, you are building your renderer's UI using the same tools
(HTML, CSS, JavaScript) that you would on the web. Therefore, Electron's docs will
not go in-depth on how to use standard web tools.

On the other hand, Electron also provides a rich set of tools that allow
you to integrate with the desktop environment, from creating tray icons to adding
global shortcuts to displaying native menus. It also gives you all the power of a
Node.js environment in the main process. This set of capabilities separates
Electron applications from running a website in a browser tab, and are the
focus of Electron's documentation.

## How-to examples

Electron's documentation has many tutorials to help you with more advanced topics
and deeper operating system integrations. To get started, check out the
[How-To Examples][how-to] doc.

:::note Let us know if something is missing!

If you can't find what you are looking for, please let us know on [GitHub][] or in
our [Discord server][discord]!

:::

## What's next?

For the rest of the tutorial, we will be shifting away from application code
and giving you a look at how you can get your app from your developer machine
into end users' hands.

<!-- Link labels -->

[discord]: https://discord.gg/electronjs
[github]: https://github.com/electron/website/issues/new
[how-to]: ./examples.md

<!-- Tutorial links -->

[prerequisites]: tutorial-1-prerequisites.md
[building your first app]: tutorial-2-first-app.md
[preload]: tutorial-3-preload.md
[features]: tutorial-4-adding-features.md
[packaging]: tutorial-5-packaging.md
[updates]: tutorial-6-publishing-updating.md
