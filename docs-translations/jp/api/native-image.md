# nativeImage

Electronでは、画像を取得するAPI用に、ファイルパスまたは`nativeImage`インスタンスを渡します。`null`を渡すと、空のイメージが使われます。

例えば、トレイを作成したり、ウィンドウアイコンを設定する時、`String`としてイメージファイルパスを渡します:

```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png')
var window = new BrowserWindow({icon: '/Users/somebody/images/window.png'})
```

もしくは、`nativeImage`を返すクリップボードからイメージを読み込みます:

```javascript
var image = clipboard.readImage()
var appIcon = new Tray(image)
```

## 対応しているフォーマット

今のところ、`PNG` と `JPEG`の画像フォーマットに対応しています。透明や可逆圧縮に対応しているので、`PNG` を推奨します。

Windowsでは、ファイルパスから`ICO`アイコンをロードできます。

## 高解像度画像

高DPIに対応しているプラットフォームで、高解像度としてマークするためにイメージの基本ファイル名の後に`@2x`を追加できます。

例えば、`icon.png`は一般的な解像度の通常の画像で、`icon@2x.png`は、倍のDPI密度を持つ高解像度画像として扱われます。


同時に異なるDPI密度をディスプレイで対応したい場合、同じフォルダーに異なるサイズの画像を置き、DPIサフィックスなしでファイル名を使用します。

例:

```text
images/
├── icon.png
├── icon@2x.png
└── icon@3x.png
```


```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png')
```

次のDPIサフィックスに対応しています:

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

## テンプレート画像

テンプレート画像は、黒とクリアな色（とアルファチャンネル）で成り立ちます。テンプレート画像は画像単体での使用は想定しておらず、通常は最終的にほしい画像を生成するためにほかのコンテンツと合成します。

もっとも一般的なケースは、ライトとダークなメニュバー両方に切り替え可能なメニュバーアイコン用にテンプレート画像を使います。


**Note:** テンプレート画像は、macOSでのみサポートしています。

テンプレート画像として画像をマークするために、ファイル名の最後に`Template`をつけます。

例:

* `xxxTemplate.png`
* `xxxTemplate@2x.png`

## メソッド

`nativeImage`クラスは次のメソッドを持ちます:

### `nativeImage.createEmpty()`

空の`nativeImage` インスタンスを生成します。

### `nativeImage.createFromPath(path)`

* `path` String

`path`で指定したファイルから新しい`nativeImage`を生成します。

### `nativeImage.createFromBuffer(buffer[, scaleFactor])`

* `buffer` [Buffer][buffer]
* `scaleFactor` Double (optional)

 `buffer`から新しい`nativeImage`インスタンスを生成します。既定では、`scaleFactor`は1.0です。

### `nativeImage.createFromDataURL(dataURL)`

* `dataURL` String

`dataURL`から新しい `nativeImage`インスタンスを生成します。

## インスタンスメソッド

`nativeImage`のインスタンス上で提供されるメソッド:

```javascript
const nativeImage = require('electron').nativeImage

var image = nativeImage.createFromPath('/Users/somebody/images/icon.png')
```

### `image.toPNG()`

`PNG`エンコードされた画像を含む[Buffer][buffer]を返します。

### `image.toJPEG(quality)`

* `quality` Integer (**required**) - Between 0 - 100.

`JPEG`エンコードされた画像を含む[Buffer][buffer]を返します。

### `image.toDataURL()`

画像のURLデータを返します。

### `image.isEmpty()`

画像が空かどうかをブーリアン値で返します。

### `image.getSize()`

画像サイズを返します。

[buffer]: https://nodejs.org/api/buffer.html#buffer_class_buffer

### `image.setTemplateImage(option)`

* `option` Boolean

テンプレート画像としてマークします。

### `image.isTemplateImage()`

イメージがテンプレート画像かどうかをブーリアン値で返します。
