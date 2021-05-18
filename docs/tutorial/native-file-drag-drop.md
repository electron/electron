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

The ipc message from the renderer to the main process needs to come from `preload.js` to stay compliant with security guidelines. Add the following code to `preload.js`:

```js
const { contextBridge, ipcRenderer } = require('electron')
const fs = require('fs')

contextBridge.exposeInMainWorld('electron', {
  startDrag: (fileName) => {
    // Create a new file to copy - you can also copy existing files.
    fs.writeFileSync(fileName, '# Test drag and drop')

    ipcRenderer.send('ondragstart', process.cwd() + `/${fileName}`)
  }
})
```

Add a draggable element to `index.html`, and reference your renderer script:

```html
<div style="border:2px solid black;border-radius:3px;padding:5px;display:inline-block" draggable="true" id="drag">Drag me</div>
<script src="renderer.js"></script>
```

and instruct the renderer process (in `renderer.js`) to handle drag events by calling the `contextBridge` method you added in `preload.js`:

`renderer.js`:

```javascript
document.getElementById('drag').ondragstart = (event) => {
  event.preventDefault()
  window.electron.startDrag('drag-and-drop.md')
}
```

In the Main process(`main.js` file), expand the received event with a path to the file that is
being dragged and an icon:

```javascript fiddle='docs/fiddles/features/drag-and-drop'
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
