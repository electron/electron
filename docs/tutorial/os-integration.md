---
title: 'Integrating with the OS'
description: 'This guide will step you through the process of creating a barebones Hello World app in Electron, similar to electron/electron-quick-start.'
slug: os-integration
hide_title: false
---

There are many ways to integrate with the Operating System. In some cases it could be platform specific
(like Apple's TouchBar), in others it could be through APIs to have a more integrated look and feel
(like modifying the Title Bar), or using APIs that give you access to USB and Bluetooth devices.

The following is a list of all the guides currently available.
Each guide contains a practical example in a minimal, self-contained example app.
The easiest way to run these examples is by downloading [Electron Fiddle][fiddle].

Once Fiddle is installed, you can press on the "Open in Fiddle" button that you
will find below code samples like the following one:

```fiddle docs/latest/fiddles/quick-start
window.addEventListener('DOMContentLoaded', () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector)
    if (element) element.innerText = text
  }

  for (const type of ['chrome', 'node', 'electron']) {
    replaceText(`${type}-version`, process.versions[type])
  }
})
```

If there is still something that you do not know how to do, please take a look at the [API][app]
as there is a chance it might be documented just there (and also open an issue requesting the
guide!).

<!-- guide-table-start -->

| Guide                      | Description                                                                                               |
| :------------------------- | --------------------------------------------------------------------------------------------------------- |
| [window lifecycle]         | Learn how to properly handle your window's interaction based on the OS.                                   |
| [Dark mode]                | Discover how to respect the user's OS preference for a light or dark mode.                                |
| [In-App purchases]         | Enable In-App purchases when publishing via the Apple Store and iTunes Connect.                           |
| [URL handler]              | Make your application the default handler for a specific protocol like `mailto:`.                         |
| [Desktop Launcher Actions] | Learn how to add a custom entry on different Linux's system launcher.                                     |
| [macOS Dock]               | Configure your Electron app's icon in the macOS Dock and create shortcuts for custom tasks.               |
| [File drag & drop]         | Use the OS' drag & drop feature to bring content into/from your application.                              |
| [Notifications]            | Show notifications to your users in all supported platforms.                                              |
| [Online/Offline]           | Detect if your application has Internet access and behave accordingly.                                    |
| [Taskbar progress bar]     | Show progress information to your users directly from the taskbar of the OS.                              |
| [Recent documents]         | Provide access to a list of recent documents opened by the application via JumpList or dock menu.         |
| [Represented file]         | On macOS, you can set a represented file for any window in your application.                              |
| [Tray]                     | Add a try icon to your application with its own context menu.                                             |
| [Windows customization]    | The BrowserWindow module exposes many APIs that can change the look and behavior of your browser windows. |
| [Windows taskbar]          | Customize your app's Windows taskbar with Jumplist, thumbnails and toolbars, icon overlays, etc.          |
| [Windows on ARM]           | Improve the performance of your application for users running Windows on ARM.                             |

<!-- guide-table-end -->

<!-- Links -->

[advanced-installation]: installation.md
[app]: ../api/app.md
[app-ready]: ../api/app.md#event-ready
[app-when-ready]: ../api/app.md#appwhenready
[browser-window]: ../api/browser-window.md
[commonjs]: https://nodejs.org/docs/latest/api/modules.html#modules_modules_commonjs_modules
[package-json-main]: https://docs.npmjs.com/cli/v7/configuring-npm/package-json#main
[package-scripts]: https://docs.npmjs.com/cli/v7/using-npm/scripts
[process-model]: process-model.md

<!-- How tos -->

[dark mode]: ./dark-mode.md
[desktop launcher actions]: ./linux-desktop-actions.md
[device access]: ./devices.md
[macos dock]: ./macos-dock.md
[file drag & drop]: ./native-file-drag-drop.md
[in-app purchases]: ./in-app-purchases.md
[keyboard shortchuts]: ./keyboard-shortcuts.md
[notifications]: ./notifications.md
[online/offline]: ./online-offline-events.md
[represented file]: ./represented-file.md
[spellchecker]: ./spellchecker.md
[taskbar progress bar]: ./progress-bar.md
[recent documents]: ./recent-documents.md
[tray]: ./tray.md
[url handler]: ./launch-app-from-url-in-another-app.md
[window customization]: ./window-customization.md
[windows taskbar]: ./windows-taskbar.md

<!-- Tutorial links -->

[prerequisites]: tutorial-prerequisites.md
[scaffolding]: tutorial-scaffolding.md
[main-renderer]: ./tutorial-main-renderer.md
[application distribution]: distribution-overview.md
[code signing]: code-signing.md
[updates]: updates.md
