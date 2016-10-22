# globalShortcut

さまざまなショートカットの動作をカスタマイズするために、オペレーティングシステムのグローバルのキーボードショートカットを`globalShortcut`モジュールは登録したり、解除したりできます。

**Note:** ショートカットはグローバルです。アプリがキーボードフォーカスを持っていなくても動作します。`app`モジュールの `ready`イベントが出力されるまでは使うべきではありません。

```javascript
const electron = require('electron')
const app = electron.app
const globalShortcut = electron.globalShortcut

app.on('ready', function () {
  // Register a 'ctrl+x' shortcut listener.
  var ret = globalShortcut.register('ctrl+x', function () {
    console.log('ctrl+x is pressed')
  })

  if (!ret) {
    console.log('registration failed')
  }

  // Check whether a shortcut is registered.
  console.log(globalShortcut.isRegistered('ctrl+x'))
})

app.on('will-quit', function () {
  // Unregister a shortcut.
  globalShortcut.unregister('ctrl+x')

  // Unregister all shortcuts.
  globalShortcut.unregisterAll()
})
```

## メソッド

`globalShortcut`モジュールは次のメソッドを持ちます:

### `globalShortcut.register(accelerator, callback)`

* `accelerator` [Accelerator](accelerator.md)
* `callback` Function

`accelerator`のグローバルショートカットを登録します。`callback`は、ユーザーが登録しているショートカットを押したときにコールされます。

ほかのアプリケーションがすでにacceleratorを使用している時、この呼び出しは静かに失敗します。アプリケーション間でグローバルショートカットの争いをしてほしくないので、オペレーティングシステムはこの挙動を採用しています。

### `globalShortcut.isRegistered(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

このアプリケーションが`accelerator`に登録されているかどうかを返します。

acceleratorがすでにほかのアプリケーションで取得していると、このコールは、`false`を返します。アプリケーション間でグローバルショートカットの争いをしてほしくないので、オペレーティングシステムはこの挙動を採用しています。

### `globalShortcut.unregister(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

Unregisters the global shortcut of `accelerator`のグローバルショートカットを解除します。

### `globalShortcut.unregisterAll()`

全てのグローバルショートカットを解除します。
