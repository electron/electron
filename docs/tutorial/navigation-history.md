---
title: "Navigation History"
description: "The NavigationHistory API allows you to manage and interact with the browsing history of your Electron application."
slug: navigation-history
hide_title: false
---

# Navigation History

## Overview

The [NavigationHistory](../api/navigation-history.md) API allows you to manage and interact with the browsing history of your Electron application. This powerful feature enables you to create intuitive navigation experiences for your users.

### Accessing NavigationHistory

To use NavigationHistory, access it through the [webContents](../api/web-contents.md) of your [BrowserWindow](../api/browser-window.md):

```js
const { BrowserWindow } = require('electron')

const mainWindow = new BrowserWindow({ width: 800, height: 600 })
const navigationHistory = mainWindow.webContents.navigationHistory
```

#### Navigating through history

Easily implement back and forward navigation:

```js @ts-type={navigationHistory:Electron.NavigationHistory}
// Go back
if (navigationHistory.canGoBack()) {
  navigationHistory.goBack()
}

// Go forward
if (navigationHistory.canGoForward()) {
  navigationHistory.goForward()
}
```

#### Accessing history entries

Retrieve and display the user's browsing history:

```js @ts-type={navigationHistory:Electron.NavigationHistory}
const entries = navigationHistory.getAllEntries()
entries.forEach((entry) => {
  console.log(`${entry.title}: ${entry.url}`)
})
```

#### Navigating to specific entries

Allow users to jump to any point in their browsing history:

```js @ts-type={navigationHistory:Electron.NavigationHistory}
// Navigate to the 5th entry in the history
navigationHistory.goToIndex(4)

// Navigate to the 2nd entry from the current position
navigationHistory.goToOffset(2)
```

Here's a full example that you can open with Electron Fiddle:

```fiddle docs/fiddles/features/navigation-history

```
