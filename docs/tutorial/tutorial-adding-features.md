---
title: 'Adding features to your application'
description: 'In this step of the tutorial we will share some resources you should read to add features to your application'
slug: adding-features
hide_title: false
---

:::info Tutorial parts
This is part 4 of the Electron tutorial. The other parts are:

1. [Prerequisites]
1. [Scaffolding]
1. [Main and Renderer process communication][main-renderer]
1. [Adding Features][features]
1. [Application Distribution]
1. [Code Signing]
1. [Updating Your Application][updates]

:::

If you have been following the tutorial, you should have a basic Electron application
with some basic user interface. Now is the moment to work on your application and
make it more integrated with the Operating System as well as adding some more advanced
features.

## Managing your application's window lifecycle

Application windows behave differently on each OS, and Electron puts the responsibility on developers to implement these conventions in their app.

In general, you can use the `process` global's [`platform`][node-platform] attribute
to run code specifically for certain operating systems.

### Quit the app when all windows are closed (Windows & Linux)

On Windows and Linux, exiting all windows generally quits an application entirely.

To implement this, listen for the `app` module's [`'window-all-closed'`][window-all-closed]
event, and call [`app.quit()`][app-quit] if the user is not on macOS (`darwin`).

```js
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
```

### Open a window if none are open (macOS)

Whereas Linux and Windows apps quit when they have no windows open, macOS apps generally
continue running even without any windows open, and activating the app when no windows
are available should open a new one.

To implement this feature, listen for the `app` module's [`activate`][activate]
event, and call your existing `createWindow()` method if no browser windows are open.

Because windows cannot be created before the `ready` event, you should only listen for
`activate` events after your app is initialized. Do this by attaching your event listener
from within your existing `whenReady()` callback.

```js
app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})
```

All your code should be similar to this:

```fiddle docs/latest/fiddles/windows-lifecycle

```

## Other integrations and How to's

To help you with more advance topics and deeper OS integration, we have created some tutorials.
You can access them by clicking the links below:

- [OS Integration]: How to make your application feel more integrated with the Operating
  System where it is running.
- [How to]: A list of more advanced topics that are general to Electron development and
  not for a particular Operating System.

:::tip Let us know if something is missing!
If you can't find what you are looking for, please let us know on our [GitHub repo] or in
our [Discord server][discord]!
:::

<!-- Link labels -->

[discord]: https://discord.com/invite/electron
[github repo]: https://github.com/electron/electronjs.org-new/issues/new
[os integration]: ./os-integration.md
[how to]: ./examples.md

<!-- Tutorial links -->

[prerequisites]: tutorial-prerequisites.md
[scaffolding]: tutorial-scaffolding.md
[main-renderer]: ./tutorial-main-renderer.md
[features]: ./tutorial-adding-features.md
[application distribution]: distribution-overview.md
[code signing]: code-signing.md
[updates]: updates.md
