# powerSaveBlocker

`powerSaveBlocker`モジュールは、省電力（スリープ）モードからシステムをブロックするのに使用され、それ故にアプリがシステムと画面をアクティブに維持することができます。

例:

```javascript
const powerSaveBlocker = require('electron').powerSaveBlocker;

var id = powerSaveBlocker.start('prevent-display-sleep');
console.log(powerSaveBlocker.isStarted(id));

powerSaveBlocker.stop(id);
```

## メソッド

`powerSaveBlocker`モジュールは次のメソッドを持ちます:

### `powerSaveBlocker.start(type)`

* `type` String - パワーセーブのブロック種類です。
  * `prevent-app-suspension` - アプリケーションがサスペンドになるのを防ぎます。システムのアクティブ状態を維持できますが、画面をオフにすることができます。使用例：ファイルのダウンロードまたは音楽の再生
  * `prevent-display-sleep`- ディスプレイがスリープになるのを防ぎます。システムと画面のアクティブ状態を維持できます。使用例：ビデオ再生

省電力モードに入るのを防止するシステムを開始します。パワーセーブブロッカーを確認する数字を返します。

**Note:** `prevent-display-sleep`は`prevent-app-suspension`より高い優先権を持ちます。最も優先権が高いタイプのみが影響します。言い換えれば。`prevent-display-sleep`はいつも`prevent-app-suspension`より高い優先権をもちます。

例えば、APIが`prevent-app-suspension`を要求するAをコールし、ほかのAPIが`prevent-display-sleep`を要求するBをコールすると、`prevent-display-sleep`はBのリクエストが止まるまで使われ、そのあと`prevent-app-suspension`が使われます。

### `powerSaveBlocker.stop(id)`

* `id` Integer - `powerSaveBlocker.start`によって、パワーセーブブロッカーIDが返されます。

指定したパワーセーブブロッカーを停止します。

### `powerSaveBlocker.isStarted(id)`

* `id` Integer - `powerSaveBlocker.start`によって、パワーセーブブロッカーIDが返されます。

`powerSaveBlocker`が開始したかどうかのブーリアン値を返します。
