---
title: 'Introduction'
description: 'Welcome to the Electron documentation! If this is your first time developing an Electron app, read through this Getting Started section to get familiar with the basics. Otherwise, feel free to explore our guides and API documentation!'
slug: /latest/
hide_title: false
---

# What is Electron?

Electron is a framework for building desktop applications using JavaScript,
HTML, and CSS. By embedding [Chromium][chromium] and [Node.js][node] into its
binary, Electron allows you to maintain one JavaScript codebase and create
cross-platform apps that work on Windows, macOS, and Linux — no native development
experience required.

We recommend you to start with the [Tutorial], even if you have previous
experience as you might find information you didn't know.
The [examples] and [API documentation] are also good places to browse around
and discover new things.

## What is in the docs?

All the official documentation is available from the sidebar navigation. These
are the different categories and what you can expect on each one:

- **[Tutorial]**: An end-to-end guide on how to create and publish your first Electron
  application. You will also find tips and links to more in-depth guides.
- **[Development]**: In depth guides related to developing Electron aplications.
- **[Distribution]**: Learn how to distribute and update your Electron application in all
  supported platforms.
- **[Testing and debugging]**: How to debug JavaScript, write tests, and other tools used
  to create quality Electron applications.
- **[Examples]**: How to integrate more deeply with the Operating System and also how to
  accomplish more complex scenarios.
- **[Developer Checklists]**: The Do's and Don'ts to keep in mind when developing your app.
- **[Resources]**: Useful links to better understand how the Electron project and team works
  and is organized.
- **[Contributing to Electron]**: Compiling Electron and making contributions can be daunting.
  We try to make it easier in this section.

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

- If you need help with developing your app, our [community Discord server][discord]
  is a great place to get advice from other Electron app developers.
- If you suspect you're running into a bug with the `electron` package, please check
  the [GitHub issue tracker][issue-tracker] to see if any existing issues match your
  problem. If not, feel free to fill out our bug report template and submit a new issue.

<!-- Links -->

[api documentation]: ../api/app.md
[chromium]: https://www.chromium.org/
[comic]: https://www.google.com/googlebooks/chrome/
[contributing to electron]: ../development/README.md
[development]: boilerplates-and-clis.md
[developer checklists]: performance.md
[discord]: https://discord.com/invite/APGC3k5yaH
[distribution]: distribution-overview.md
[examples]: examples.md
[fiddle]: https://electronjs.org/fiddle
[issue-tracker]: https://github.com/electron/electron/issues
[mdn-guide]: https://developer.mozilla.org/en-US/docs/Learn/Getting_started_with_the_web
[node]: https://nodejs.org/
[node-guide]: https://nodejs.dev/learn
[process-model]: process-model.md
[resources]: ../breaking-changes.md
[testing and debugging]: application-debugging.md
[tutorial]: tutorial-prerequisites.md
