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

Starting with a working application from the
[Quick Start Guide](quick-start.md), add the following lines to the
`index.html` file:

```html
<a href="#" id="drag">Drag me</a>
<script src="renderer.js"></script>
```

and add the following lines to the `renderer.js` file:

```javascript
const { ipcRenderer } = require('electron')

document.getElementById('drag').ondragstart = (event) => {
  event.preventDefault()
  ipcRenderer.send('ondragstart', '/absolute/path/to/the/item')
}
```

The code above instructs the Renderer process to handle the `ondragstart` event
and forward the information to the Main process.

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
the item from the BroswerWindow onto your desktop. In this guide,
the item is a Markdown file located in the root of the project:

![Drag and drop](../images/drag-and-drop.gif)
