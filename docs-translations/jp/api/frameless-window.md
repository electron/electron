# Frameless Window

フレームの無いウィンドウは、ウェブページの一部ではなく、ツールバーのようなウィンドウのパーツで、[chrome](https://developer.mozilla.org/en-US/docs/Glossary/Chrome)ではないウィンドウです。 オプションとして[`BrowserWindow`](browser-window.md)クラスがあります。

## フレームの無いウィンドウを作成する

フレームの無いウィンドウを作成するために、[BrowserWindow](browser-window.md)の `options`で、`frame` を `false`に設定する必要があります。

```javascript
const BrowserWindow = require('electron').BrowserWindow;
var win = new BrowserWindow({ width: 800, height: 600, frame: false });
```

### OS Xでの別の方法

Mac OS X 10.10 Yosemite以降では、Chrome無しのウィンドウを指定する方法があります。`frame`を`false`に設定しタイトルバーとウィンドウコントロールの両方を無効にする代わりに、タイトルバーを隠しコンテンツをフルウィンドウサイズに広げたいけど、標準的なウィンドウ操作用にウィンドウコントロール("トラフィックライト")を維持したいかもしれません。新しい`titleBarStyle`オプションを指定することで、そうできます。

```javascript
var win = new BrowserWindow({ 'titleBarStyle': 'hidden' });
```

## 透明なウィンドウ

 `transparent`オプションを`true`に設定すると、フレームの無い透明なウィンドウを作成できます。

```javascript
var win = new BrowserWindow({ transparent: true, frame: false });
```

### 制限

* 透明領域をクリックすることはできません。この問題を解決するためにウィンドウの輪郭を設定するAPIを導入しようとしています。詳細は、[our issue](https://github.com/electron/electron/issues/1335) を参照してください。
* 透明なウィンドウはサイズ変更できません。いくつかのプラットフォーム上では、`resizable`の`true`設定は、いくつかのプラットフォーム上で、動作を停止する透明ウィンドウを作成するかもしれません。
* `blur`フィルターはウェブページのみに適用され、ウィンドウの下のコンテンツ（例えば、ユーザーのシステム上でほかのアプリケーションを開く）に、ぼやける効果を適用する方法はありません。
* Windows オペレーティングシステム上では、DMMが無効のとき透明なウィンドウは動作しません。
* Linuxユーザーは、GPUを無効化するためにコマンドラインで`--enable-transparent-visuals --disable-gpu`を設定でき、透明ウィンドウを作成するためにARGBを許可でき、これは、 [いくつかのNVidiaドライバー上でアルファチャンネルが動作しない](https://code.google.com/p/chromium/issues/detail?id=369209) という上流のバグ原因になります。
* Macでは、透明なウィンドウでネイティブのウィンドウシャドーを表示できません。

## ドラッグ可能領域

既定では、フレーム無しウィンドウはドラッグできません。（OSの標準的なタイトルバーのような）ドラッグできる領域をElectronに指定するには、CSSで`-webkit-app-region: drag`を指定する必要があり、アプリはドラッグできる領域からドラッグできない領域を除外するために、`-webkit-app-region: no-drag`を使えます。現在のところ長方形領域のみサポートしています。

ウィンドウ全体をドラッグできるようにするには、`body`のスタイルに`-webkit-app-region: drag`を追加します。

```html
<body style="-webkit-app-region: drag">
</body>
```

ウィンドウ全体度ドラッグできるようにするには、ボタンをドラッグできないように指定する必要があり、そうしなければユーザーがそれをクリックする可能性があります。

```css
button {
  -webkit-app-region: no-drag;
}
```

カスタムタイトルバーをドラッグできるように設定すると、タイトルバーのすべてのボタンをドラッグできないようにする必要があります。

## テキスト選択

フレームの無いウィンドウでは、ドラッグを可能にする動作とテキスト選択がぶつかるかもしれません。例えば、タイトルバーをドラッグしたとき、うっかりタイトルバーのテキストを選択してしまうかもしれません。これを防ぐために、次の例のようにドラッグできる領域内のテキスト選択を無効にする必要があります。

```css
.titlebar {
  -webkit-user-select: none;
  -webkit-app-region: drag;
}
```

## コンテキストメニュー

いくつかのプラットフォームでは、ドラッグ可能な領域は、クライアントフレーム無しとして扱われるので、その上で右クリックすると、システムメニューがポップアップします。コンテキストメニューをすべてのプラットフォームで正しく動作するようにするためには、ドラッグ可能領域でカスタムコンテキストメニューを使用しないでください。
