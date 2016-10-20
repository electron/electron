# clipboard

`clipboard`モジュールは、コピーとペースト操作を実行するメソッドを提供します。次の例は、クリップボードに文字列を書き込む方法を示しています：

```javascript
const clipboard = require('electron').clipboard
clipboard.writeText('Example String')
```

X Windowsシステム上では、セレクションクリップボードがあります。それを操作するために、それぞれのメソッドで、`selection`を通す必要があります。

```javascript
clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
```

## メソッド

`clipboard`モジュールには、次のメソッドがあります:

**Note:** 実験的APIには、そのようにマークしてあり、将来的には削除される可能性があります。

### `clipboard.readText([type])`

* `type` String (optional)

プレーンテキストとしてクリップボードの内容を返します。

### `clipboard.writeText(text[, type])`

* `text` String
* `type` String (optional)

プレーンテキストとしてクリップボードに`text`を書き込みます。

### `clipboard.readHTML([type])`

* `type` String (optional)

HTMLマークアップとして、クリップボードの内容を返します。

### `clipboard.writeHTML(markup[, type])`

* `markup` String
* `type` String (optional)

クリップボードにHTMLマークアップとして書き込みます。

### `clipboard.readImage([type])`

* `type` String (optional)

[NativeImage](native-image.md)としてクリップボードの内容を返します。

### `clipboard.writeImage(image[, type])`

* `image` [NativeImage](native-image.md)
* `type` String (optional)

`image` としてクリップボードに書き込みます。

### `clipboard.clear([type])`

* `type` String (optional)

クリップボードの内容をクリアします。

### `clipboard.availableFormats([type])`

* `type` String (optional)

`type`のクリップボードがサポートしているフォーマット配列を返します。

### `clipboard.has(data[, type])` _実験_

* `data` String
* `type` String (optional)

`data`で指定したフォーマットをクリップボードがサポートしているかどうかを返します。

```javascript
console.log(clipboard.has('<p>selection</p>'))
```

### `clipboard.read(data[, type])` _実験_

* `data` String
* `type` String (optional)

クリップボードから`data`を読み込みます。

### `clipboard.write(data[, type])`

* `data` Object
  * `text` String
  * `html` String
  * `image` [NativeImage](native-image.md)
* `type` String (optional)

```javascript
clipboard.write({text: 'test', html: '<b>test</b>'})
```
クリップボードに`data`を書き込みます。
