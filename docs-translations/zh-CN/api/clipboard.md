# clipboard

`clipboard` 模块提供方法来供复制和粘贴操作 .
下面例子展示了如何将一个字符串写道 clipboard 上:

```javascript
const clipboard = require('electron').clipboard
clipboard.writeText('Example String')
```

在 X Window 系统上, 有一个可选的 clipboard. 你可以为每个方法使用 `selection` 来控制它:

```javascript
clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
```

## 方法

`clipboard` 模块有以下方法:

**注意:** 测试 APIs 已经标明，并且在将来会被删除 .

### `clipboard.readText([type])`

* `type` String (可选)
 
以纯文本形式从 clipboard 返回内容 .

### `clipboard.writeText(text[, type])`

* `text` String
* `type` String (可选)

以纯文本形式向 clipboard 添加内容 .

### `clipboard.readHTML([type])`

* `type` String (可选)

返回 clipboard 中的标记内容.

### `clipboard.writeHTML(markup[, type])`

* `markup` String
* `type` String (可选)

向 clipboard 添加 `markup` 内容 .

### `clipboard.readImage([type])`

* `type` String (可选)

从 clipboard 中返回 [NativeImage](native-image.md) 内容.

### `clipboard.writeImage(image[, type])`

* `image` [NativeImage](native-image.md)
* `type` String (可选)

向 clipboard 中写入 `image` .

### `clipboard.readRTF([type])`

* `type` String (可选)

从 clipboard 中返回 RTF 内容. 

### `clipboard.writeRTF(text[, type])`

* `text` String
* `type` String (可选)

向 clipboard 中写入 RTF 格式的 `text` .

### `clipboard.clear([type])`

* `type` String (可选)

清空 clipboard 内容.

### `clipboard.availableFormats([type])`

* `type` String (可选)

返回 clipboard 支持的格式数组 .

### `clipboard.has(data[, type])` _Experimental_

* `data` String
* `type` String (可选)

返回 clipboard 是否支持指定 `data` 的格式.

```javascript
console.log(clipboard.has('<p>selection</p>'))
```

### `clipboard.read(data[, type])` _Experimental_

* `data` String
* `type` String (可选)

读取 clipboard 的 `data`.

### `clipboard.write(data[, type])`

* `data` Object
  * `text` String
  * `html` String
  * `image` [NativeImage](native-image.md)
* `type` String (可选)

```javascript
clipboard.write({text: 'test', html: '<b>test</b>'})
```
向 clipboard 写入 `data` .