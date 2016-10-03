# shell

`shell`モジュールはデスクトップ統合に関係した機能を提供します。

ユーザーのデフォルトのブラウザでURLを開く例です：

```javascript
const shell = require('electron').shell

shell.openExternal('https://github.com')
```

## メソッド

`shell`モジュールは次のメソッドを持ちます:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

ファイルマネジャーでファイルを表示します。もし可能ならファイルを選択します。

### `shell.openItem(fullPath)`

* `fullPath` String

デスクトップの既定のやり方で指定したファイルを開きます。

### `shell.openExternal(url)`

* `url` String

デスクトップの既定のやり方で指定した外部のプロトコルURLで開きます。（例えば、mailto:URLでユーザーの既定メールエージェントを開きます）

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

ゴミ箱へ指定したファイルを移動し、操作結果をブーリアン値を返します。

### `shell.beep()`

ビープ音を再生します。
