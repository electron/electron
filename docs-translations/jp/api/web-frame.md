# webFrame

`web-frame`モジュールは現在のウェブページンのレンダリングのカスタマイズをできるようにします。

現在のページの倍率を200%にする例です。

```javascript
var webFrame = require('electron').webFrame

webFrame.setZoomFactor(2)
```

## メソッド

`web-frame`モジュールは次のメソッドを持ちます：

### `webFrame.setZoomFactor(factor)`

* `factor` Number - 拡大倍数

指定した倍数に拡大倍数を変更します。拡大倍数は、拡大率を100で割った数字なので、300%だと3.0です。

### `webFrame.getZoomFactor()`

現在の拡大倍数を返します。

### `webFrame.setZoomLevel(level)`

* `level` Number - 拡大レベル

指定したレベルに拡大レベルを変更します。オリジナルサイズは0で、1 つ上下させると20%拡大か縮小になり、既定の制限ではオリジナルサイズの300%と50%です。

### `webFrame.getZoomLevel()`

返事あの拡大レベルを返します。

### `webFrame.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

最大と最小の拡大レベルを設定します。

### `webFrame.setSpellCheckProvider(language, autoCorrectWord, provider)`

* `language` String
* `autoCorrectWord` Boolean
* `provider` Object

inputフィールドやtextエリアでスペルチェックの提供を設定します。

`provider`は、単語が正しいスペルかどうかを返す`spellCheck`メソッドを持つオブジェクトでなければなりません。


プロバイダーとして[node-spellchecker][spellchecker]を使用する例です:

```javascript
webFrame.setSpellCheckProvider('en-US', true, {
  spellCheck: function (text) {
    return !(require('spellchecker').isMisspelled(text))
  }
})
```

### `webFrame.registerURLSchemeAsSecure(scheme)`

* `scheme` String

セキュアなスキーマーとして`scheme`を登録します。

セキュアなスキーマーは、今テンスの混在警告をトリガーしません。例えば、アクティブなネットワーク攻撃では破壊できないので`https`と`data`はセキュアなスキーマーです。

### `webFrame.registerURLSchemeAsBypassingCSP(scheme)`

* `scheme` String

リソースは現在のページのコンテンツセキュリティポリシーにかかわらず `scheme`からロードします。

### `webFrame.registerURLSchemeAsPrivileged(scheme)`

* `scheme` String

セキュアとして`scheme`を登録し、リソースの今テンスセキュリティポリシーを回避して、ServiceWorkerの登録とAPIのフェッチをサポートします。

### `webFrame.insertText(text)`

* `text` String

フォーカスが当たっているエレメントに`text`を挿入します。

### `webFrame.executeJavaScript(code[, userGesture])`

* `code` String
* `userGesture` Boolean (オプション) - 既定では`false`です。

ページで`code`を評価します。

ブラウザウィンドウで、 `requestFullScreen`のようないくつかのHTML APIは、ユーザージェスチャーによってのみ起動できます。`userGesture`を`true` に設定すると、この制限は削除されます。

[spellchecker]: https://github.com/atom/node-spellchecker
