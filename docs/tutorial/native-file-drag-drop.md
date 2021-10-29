---
title: "Native File Drag & Drop"
description: "Use the OS' drag & drop feature to bring content into/from your application."
slug: native-file-drag-drop
hide_title: false
---

# Native File Drag & Drop

## Overview

Certain kinds of applications that manipulate files might want to support
the operating system's native file drag & drop feature. Dragging files into
web content is common and supported by many websites. Electron additionally
supports dragging files and content out from web content into the operating
system's world.

To implement this feature in your app, you need to call the
[`webContents.startDrag(item)`](../api/web-contents.md#contentsstartdragitem)
API in response to the `ondragstart` event.

## Example

An example demonstrating how you can create a file on the fly to be dragged out of the window.

### Preload.js

In `preload.js` use the [`contextBridge`] to inject a method `window.electron.startDrag(...)` that will send an IPC message to the main process.

```js
const { contextBridge, ipcRenderer } = require('electron')
const path = require('path')

contextBridge.exposeInMainWorld('electron', {
  startDrag: (fileName) => {
    ipcRenderer.send('ondragstart', path.join(process.cwd(), fileName))
  }
})
```

### Index.html

Add a draggable element to `index.html`, and reference your renderer script:

```html
<div style="border:2px solid black;border-radius:3px;padding:5px;display:inline-block" draggable="true" id="drag">Drag me</div>
<script src="renderer.js"></script>
```

### Renderer.js

In `renderer.js` set up the renderer process to handle drag events by calling the method you added via the [`contextBridge`] above.

```javascript
document.getElementById('drag').ondragstart = (event) => {
  event.preventDefault()
  window.electron.startDrag('drag-and-drop.md')
}
```

### Main.js

In the Main process (`main.js` file), expand the received event with a path to the file that is
being dragged and an icon:

```fiddle docs/latest/fiddles/features/drag-and-drop
const { ipcMain } = require('electron')

ipcMain.on('ondragstart', (event, filePath) => {
  event.sender.startDrag({
    file: filePath,
    icon: '/path/to/icon.png'
  })
})
```

After launching the Electron application, try dragging and dropping
the item from the BrowserWindow onto your desktop. In this guide,
the item is a Markdown file located in the root of the project:

![Drag and drop](../images/drag-and-drop.gif)

[`contextBridge`]: ../api/context-bridge.md
