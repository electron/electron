---
title: Tray
description: This guide will take you through the process of creating
  a Tray icon with its own context menu to the system's notification area.
slug: tray
hide_title: true
---

# Tray

## Overview

<!-- ✍ Update this section if you want to provide more details -->

This guide will take you through the process of creating a
[Tray](../api/tray.md) icon with
its own context menu to the system's notification area.

On MacOS and Ubuntu, the Tray will be located on the top
right corner of your screen, adjacent to your battery and wifi icons.
On Windows, the Tray will usually be located in the bottom right corner.

## Example

### main.js

First we must import `app`, `Tray`, `Menu`, `nativeImage` from `electron`.

```js
const { app, Tray, Menu, nativeImage } = require('electron')
```

Next we will create our Tray. To do this, we will use a
[`NativeImage`](../api/native-image.md) icon,
which can be created through any one of these
[methods](../api/native-image.md#methods).
Note that we wrap our Tray creation code within an
[`app.whenReady`](../api/app.md#appwhenready)
as we will need to wait for our electron app to finish initializing.

```js title='main.js'
let tray

app.whenReady().then(() => {
  const icon = nativeImage.createFromPath('path/to/asset.png')
  tray = new Tray(icon)

  // note: your contextMenu, Tooltip and Title code will go here!
})
```

Great! Now we can start attaching a context menu to our Tray, like so:

```js @ts-expect-error=[8]
const contextMenu = Menu.buildFromTemplate([
  { label: 'Item1', type: 'radio' },
  { label: 'Item2', type: 'radio' },
  { label: 'Item3', type: 'radio', checked: true },
  { label: 'Item4', type: 'radio' }
])

tray.setContextMenu(contextMenu)
```

The code above will create 4 separate radio-type items in the context menu.
To read more about constructing native menus, click
[here](../api/menu.md#menubuildfromtemplatetemplate).

Finally, let's give our tray a tooltip and a title.

```js @ts-type={tray:Electron.Tray}
tray.setToolTip('This is my application')
tray.setTitle('This is my title')
```

## Conclusion

After you start your electron app, you should see the Tray residing
in either the top or bottom right of your screen, depending on your
operating system.

```fiddle docs/fiddles/native-ui/tray
```
