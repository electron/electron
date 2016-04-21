# nativeImage

Create tray, dock, and application icons using PNG or JPG files.

In Electron, for the APIs that take images, you can pass either file paths or
`nativeImage` instances. An empty image will be used when `null` is passed.

For example, when creating a tray or setting a window's icon, you can pass an
image file path as a `String`:

```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png');
var window = new BrowserWindow({icon: '/Users/somebody/images/window.png'});
```

Or read the image from the clipboard which returns a `nativeImage`:

```javascript
var image = clipboard.readImage();
var appIcon = new Tray(image);
```

## Supported Formats

Currently `PNG` and `JPEG` image formats are supported. `PNG` is recommended
because of its support for transparency and lossless compression.

On Windows, you can also load an `ICO` icon from a file path.

## High Resolution Image

On platforms that have high-DPI support, you can append `@2x` after image's
base filename to mark it as a high resolution image.

For example if `icon.png` is a normal image that has standard resolution, then
`icon@2x.png` will be treated as a high resolution image that has double DPI
density.

If you want to support displays with different DPI densities at the same time,
you can put images with different sizes in the same folder and use the filename
without DPI suffixes. For example:

```text
images/
├── icon.png
├── icon@2x.png
└── icon@3x.png
```


```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png');
```

Following suffixes for DPI are also supported:

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

## Template Image

Template images consist of black and clear colors (and an alpha channel).
Template images are not intended to be used as standalone images and are usually
mixed with other content to create the desired final appearance.

The most common case is to use template images for a menu bar icon so it can
adapt to both light and dark menu bars.

**Note:** Template image is only supported on OS X.

To mark an image as a template image, its filename should end with the word
`Template`. For example:

* `xxxTemplate.png`
* `xxxTemplate@2x.png`

## Methods

The `nativeImage` class has the following methods:

### `nativeImage.createEmpty()`

Creates an empty `nativeImage` instance.

### `nativeImage.createFromPath(path)`

* `path` String

Creates a new `nativeImage` instance from a file located at `path`.

### `nativeImage.createFromBuffer(buffer[, scaleFactor])`

* `buffer` [Buffer][buffer]
* `scaleFactor` Double (optional)

Creates a new `nativeImage` instance from `buffer`. The default `scaleFactor` is
1.0.

### `nativeImage.createFromDataURL(dataURL)`

* `dataURL` String

Creates a new `nativeImage` instance from `dataURL`.

## Instance Methods

The following methods are available on instances of `nativeImage`:

```javascript
const nativeImage = require('electron').nativeImage;

var image = nativeImage.createFromPath('/Users/somebody/images/icon.png');
```

### `image.toPng()`

Returns a [Buffer][buffer] that contains the image's `PNG` encoded data.

### `image.toJpeg(quality)`

* `quality` Integer (**required**) - Between 0 - 100.

Returns a [Buffer][buffer] that contains the image's `JPEG` encoded data.

### `image.toDataURL()`

Returns the data URL of the image.

### `image.getNativeHandle()` _OS X_

Returns a [Buffer][buffer] that stores C pointer to underlying native handle of
the image. On OS X, a pointer to `NSImage` instance would be returned.

Notice that the returned pointer is a weak pointer to the underlying native
image instead of a copy, so you _must_ ensure that the associated
`nativeImage` instance is kept around.

### `image.isEmpty()`

Returns a boolean whether the image is empty.

### `image.getSize()`

Returns the size of the image.

[buffer]: https://nodejs.org/api/buffer.html#buffer_class_buffer

### `image.setTemplateImage(option)`

* `option` Boolean

Marks the image as template image.

### `image.isTemplateImage()`

Returns a boolean whether the image is a template image.
