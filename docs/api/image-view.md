# ImageView

> A View that displays an image.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

Useful for showing splash screens that will be swapped for `WebContentsView`s
when the content finishes loading.

Note that `ImageView` is experimental and may be changed or removed in the future.

```js
const { BaseWindow, ImageView, nativeImage } = require('electron')
const path = require('node:path')

const image = nativeImage.createFromPath(path.join(__dirname, 'image.png'))
const { width, height } = image.getSize()

const win = new BaseWindow({ width, height, useContentSize: true })

const view = new ImageView()
view.setImage(image)
view.setBounds({ x: 0, y: 0, width, height })
win.contentView.addChildView(view)
```

## Class: ImageView extends `View`

> A View that displays an image.

Process: [Main](../glossary.md#main-process)

`ImageView` inherits from [`View`](view.md).

`ImageView` is an [EventEmitter][event-emitter].

### `new ImageView()` _Experimental_

Creates an ImageView.

### Instance Methods

The following methods are available on instances of the `ImageView` class, in
addition to those inherited from [View](view.md):

#### `image.setImage(image)` _Experimental_

* `image` NativeImage

Sets the image for this `ImageView`. Note that only image formats supported by
`NativeImage` can be used with an `ImageView`.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
