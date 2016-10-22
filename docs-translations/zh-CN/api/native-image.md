# nativeImage

在 Electron 中, 对所有创建 images 的 api 来说, 你可以使用文件路径或 `nativeImage` 实例. 如果使用 `null` ，将创建一个空的image 对象.

例如, 当创建一个 tray 或设置窗口的图标时候，你可以使用一个字符串的图片路径 :

```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png')
var window = new BrowserWindow({icon: '/Users/somebody/images/window.png'})
```

或者从剪切板中读取图片，它返回的是 `nativeImage`:

```javascript
var image = clipboard.readImage()
var appIcon = new Tray(image)
```

## 支持的格式

当前支持 `PNG` 和 `JPEG` 图片格式. 推荐 `PNG` ，因为它支持透明和无损压缩.

在 Windows, 你也可以使用 `ICO` 图标的格式.

## 高分辨率图片

如果平台支持 high-DPI，你可以在图片基础路径后面添加 `@2x` ，可以标识它为高分辨率的图片.

例如，如果 `icon.png` 是一个普通图片并且拥有标准分辨率，然后 `icon@2x.png`将被当作高分辨率的图片处理，它将拥有双倍 DPI 密度.

如果想同时支持展示不同分辨率的图片，你可以将拥有不同size 的图片放在同一个文件夹下，不用 DPI 后缀.例如 :

```text
images/
├── icon.png
├── icon@2x.png
└── icon@3x.png
```


```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png')
```

也支持下面这些 DPI 后缀:

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

## 模板图片

模板图片由黑色和清色(和一个 alpha 通道)组成.
模板图片不是单独使用的，而是通常和其它内容混合起来创建期望的最终效果.

最常见的用力是将模板图片用到菜单栏图片上，所以它可以同时适应亮、黑不同的菜单栏.

**注意:** 模板图片只在 macOS 上可用.

为了将图片标识为一个模板图片，它的文件名应当以 `Template` 结尾. 例如:

* `xxxTemplate.png`
* `xxxTemplate@2x.png`

## 方法

`nativeImage` 类有如下方法:

### `nativeImage.createEmpty()`

创建一个空的 `nativeImage` 实例.

### `nativeImage.createFromPath(path)`

* `path` String

从指定 `path` 创建一个新的 `nativeImage` 实例 .

### `nativeImage.createFromBuffer(buffer[, scaleFactor])`

* `buffer` [Buffer][buffer]
* `scaleFactor` Double (可选)

从 `buffer` 创建一个新的 `nativeImage` 实例 .默认  `scaleFactor` 是 1.0.

### `nativeImage.createFromDataURL(dataURL)`

* `dataURL` String

从 `dataURL` 创建一个新的 `nativeImage` 实例 .

## 实例方法

`nativeImage` 有如下方法:

```javascript
const nativeImage = require('electron').nativeImage

var image = nativeImage.createFromPath('/Users/somebody/images/icon.png')
```

### `image.toPNG()`

返回一个 [Buffer][buffer] ，它包含了图片的 `PNG` 编码数据.

### `image.toJPEG(quality)`

* `quality` Integer (**必须**) - 在 0 - 100 之间.

返回一个 [Buffer][buffer] ，它包含了图片的 `JPEG` 编码数据.

### `image.toDataURL()`

返回图片数据的 URL.

### `image.getNativeHandle()` _macOS_

返回一个保存了 c 指针的 [Buffer][buffer] 来潜在处理原始图像.在macOS, 将会返回一个 `NSImage` 指针实例.

注意那返回的指针是潜在原始图像的弱指针，而不是一个复制，你_必须_ 确保与 `nativeImage` 的关联不间断 .

### `image.isEmpty()`

返回一个 boolean ，标识图片是否为空.

### `image.getSize()`

返回图片的 size.

[buffer]: https://nodejs.org/api/buffer.html#buffer_class_buffer

### `image.setTemplateImage(option)`

* `option` Boolean

将图片标识为模板图片.

### `image.isTemplateImage()`

返回一个 boolean ，标识图片是否是模板图片.