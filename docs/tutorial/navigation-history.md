---
title: "Navigation History"
description: "The NavigationHistory API allows you to manage and interact with the browsing history of your Electron application."
slug: navigation-history
hide_title: false
---

# Navigation History

## Overview

The [NavigationHistory](../api/navigation-history.md) class allows you to manage and interact with the browsing history of your Electron application. This powerful feature enables you to create intuitive navigation experiences for your users.

## Accessing NavigationHistory

Navigation history is stored per [`WebContents`](../api/web-contents.md) instance. To access a specific instance of the NavigationHistory class, use the WebContents class's [`contents.navigationHistory` instance property](https://www.electronjs.org/docs/latest/api/web-contents#contentsnavigationhistory-readonly).

```js
const { BrowserWindow } = require('electron')

const mainWindow = new BrowserWindow()
const { navigationHistory } = mainWindow.webContents
```

## Navigating through history

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

## Accessing history entries

Retrieve and display the user's browsing history:

```js @ts-type={navigationHistory:Electron.NavigationHistory}
const entries = navigationHistory.getAllEntries()

entries.forEach((entry) => {
  console.log(`${entry.title}: ${entry.url}`)
})
```

Each navigation entry corresponds to a specific page. The indexing system follows a sequential order:

- Index 0: Represents the earliest visited page.
- Index N: Represents the most recent page visited.

## Navigating to specific entries

Allow users to jump to any point in their browsing history:

```js @ts-type={navigationHistory:Electron.NavigationHistory}
// Navigate to the 5th entry in the history, if the index is valid
navigationHistory.goToIndex(4)

// Navigate to the 2nd entry forward from the current position
if (navigationHistory.canGoToOffset(2)) {
  navigationHistory.goToOffset(2)
}
```

Here's a full example that you can open with Electron Fiddle:

```fiddle docs/fiddles/features/navigation-history

```
