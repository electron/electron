# NativeImage

In atom-shell for the APIs that take images, you can pass either file paths or
`NativeImage` instances. When passing `null`, an empty image will be used.

For example when creating tray or setting window's icon, you can pass image's
file path as `String` to represent an image:

```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png');
var window = new BrowserWindow({icon: '/Users/somebody/images/window.png'});
```

Or read the image from clipboard:

```javascript
var clipboard = require('clipboard');
var image = clipboard.readImage();
var appIcon = new Tray(image);
```

## Supported formats

Currently `PNG` and `JPEG` are supported, and it is recommended to use `PNG`
because it supports alpha channel and image is usually not compressed.

## High resolution image

On platforms that have high-DPI support, you can append `@2x` after image's
file name's base name to mark it as a high resolution image.

For example if `icon.png` is a normal image that has standard resolution, the
`icon@2x.png` would be treated as a high resolution image that has double DPI
dense.

If you want to support displays with different DPI denses at the same time, you
can put images with different sizes in the same folder, and use the filename
without DPI suffixes, like this:

```text
images/
├── icon.png
├── icon@2x.png
└── icon@3x.png
```


```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png');
```

Following suffixes as DPI denses are also supported:

* `@1x`
* `@1.25x`
* `@1.33x`
* `@1.4x`
* `@1.5x`
* `@1.8x`
* `@2x`
* `@2.5x`
* `@3x`
* `@4x`
* `@5x`

## Template image

Template images consist of black and clear colors (and an alpha channel).
Template images are not intended to be used as standalone images and are usually
mixed with other content to create the desired final appearance.

The most common case is to use template image for menu bar icon so it can adapt
to both light and dark menu bars.

Template image is only supported on Mac.

To mark an image as template image, its filename should end with the word
`Template`, examples are:

* `xxxTemplate.png`
* `xxxTemplate@2x.png`

## nativeImage.createEmpty()

Creates an empty `NativeImage` instance.

## nativeImage.createFromPath(path)

* `path` String

Creates a new `NativeImage` instance from file located at `path`.

## nativeImage.createFromBuffer(buffer[, scaleFactor])

* `buffer` [Buffer][buffer]
* `scaleFactor` Double

Creates a new `NativeImage` instance from `buffer`. The `scaleFactor` is 1.0 by
default.

## nativeImage.createFromDataUrl(dataUrl)

* `dataUrl` String

Creates a new `NativeImage` instance from `dataUrl`.

## Class: NativeImage

This class is used to represent an image.

### NativeImage.toPng()

Returns a [Buffer][buffer] that contains image's `PNG` encoded data.

### NativeImage.toJpeg(quality)

* `quality` Integer

Returns a [Buffer][buffer] that contains image's `JPEG` encoded data.

### NativeImage.toDataUrl()

Returns the data URL of image.

### NativeImage.isEmpty()

Returns whether the image is empty.

### NativeImage.getSize()

Returns the size of the image.

[buffer]: https://iojs.org/api/buffer.html#buffer_class_buffer
