# Tray

`Tray`は、オペレーティングシステムの通知エリアでアイコンで表示され、通常コンテキストメニューが付随します。

```javascript
const electron = require('electron');
const app = electron.app;
const Menu = electron.Menu;
const Tray = electron.Tray;

var appIcon = null;
app.on('ready', function(){
  appIcon = new Tray('/path/to/my/icon');
  var contextMenu = Menu.buildFromTemplate([
    { label: 'Item1', type: 'radio' },
    { label: 'Item2', type: 'radio' },
    { label: 'Item3', type: 'radio', checked: true },
    { label: 'Item4', type: 'radio' }
  ]);
  appIcon.setToolTip('This is my application.');
  appIcon.setContextMenu(contextMenu);
});

```

__プラットフォームの制限:__

* Linuxでは、サポートしている場合アプリインディケーターが使われ、サポートされていなければ代わりに`GtkStatusIcon`が使われます。
* アプリインディケーターを持っているLinuxディストリビューションでは、トレイアイコンを動作させるために`libappindicator1`をインストールする必要があります。
* コンテキストメニューがあるときは、インディケーターのみが表示されます。
* アプリインディケーターがLinuxで使われると、`click`イベントは無視されます。

すべてのプラットフォームで正確に同じ挙動を維持したい場合は、`click`イベントに依存せず、常にトレイアイコンにコンテキストメニューを付随させるようにします。

## クラス: Tray

`Tray`は[EventEmitter][event-emitter]です。

### `new Tray(image)`

* `image` [NativeImage](native-image.md)

`image`で新しいトレイアイコンを作成します。

## イベント

`Tray`モジュールは次のイベントを出力します。

**Note:** いくつかのイベントは、特定のオペレーティングシステム向けに提供され、そのようにラベルで表示します。

### イベント: 'click'

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - トレイアイコンのバウンド
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

トレイアイコンがクリックされたときに出力されます。

__Note:__  `バウンド` 再生はmacOSとWindoesのみで実装されています。

### イベント: 'right-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - トレイアイコンのバウンド
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

トレイアイコンが右クリックされると出力されます。

### イベント: 'double-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - トレイアイコンのバウンド
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

トレイアイコンがダブルクリックされたら出力されます。

### イベント: 'balloon-show' _Windows_

トレイバルーンを表示したときに出力されます。

### イベント: 'balloon-click' _Windows_

トレイバルーンがクリックされたときに出力されます。

### イベント: 'balloon-closed' _Windows_

タイムアウトもしくはユーザーの操作で閉じて、トレイバルーンがクロースされたときに出力されます。

### イベント: 'drop' _macOS_

トレイアイコンでアイテムがドラグアンドドロップされたときに出力されます。

### イベント: 'drop-files' _macOS_

* `event`
* `files` Array - ドロップされたアイテムのフルパス

トレイアイコンでファイルがドロップされたときに出力されます。

### イベント: 'drag-enter' _macOS_

トレイアイコンにドラッグ操作が入ったときに出力されます。

### イベント: 'drag-leave' _macOS_

トレイアイコンででドラッグ操作が行われたときに出力されます。

### イベント: 'drag-end' _macOS_

トレイ上でドラッグ操作が終了したか、ほかの場所で終了したときに出力されます。

## Methods

`Tray`モジュールは次のメソッドを持ちます。

**Note:** いくつかのメソッドは、特定のオペレーティングシステム向けに提供され、そのようにラベルで表示します。

### `Tray.destroy()`

ただちにトレイアイコンを終了します。

### `Tray.setImage(image)`

* `image` [NativeImage](native-image.md)

トレイアイコンの`image`を設定します。

### `Tray.setPressedImage(image)` _macOS_

* `image` [NativeImage](native-image.md)

macOSで押されたときにトレイアイコンの`image`を設定します。

### `Tray.setToolTip(toolTip)`

* `toolTip` String

トレイアイコン用のホバーテキストを設定します。

### `Tray.setTitle(title)` _macOS_

* `title` String

ステータスバーで、トレイアイコンのわきに表示するタイトルを設定します。

### `Tray.setHighlightMode(highlight)` _macOS_

* `highlight` Boolean

トレイアイコンがクリックされた時、トレイアイコンの背景をハイライト（青色）するかどうかを設定します。既定ではTrueです。

### `Tray.displayBalloon(options)` _Windows_

* `options` Object
  * `icon` [NativeImage](native-image.md)
  * `title` String
  * `content` String

トレイバルーンを表示します。

### `Tray.popUpContextMenu([menu, position])` _macOS_ _Windows_

* `menu` Menu (optional)
* `position` Object (optional) - ポップアップ位置
  * `x` Integer
  * `y` Integer

トレイアイコンのコンテキストメニューをポップアップします。`menu`が渡されたとき、`menu`はトレイコンテキストメニューの代わりに表示されます。

`position`はWindowsのみで提供され、既定では(0, 0) です。

### `Tray.setContextMenu(menu)`

* `menu` Menu

アイコン用のコンテキストメニューを設定します。

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter
