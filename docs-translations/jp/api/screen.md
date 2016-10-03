# screen

`screen`モジュールは、画面サイズ、ディスプレイ、カーソル位置などの情報を読み取ります。`app`モジュールの`ready`イベントが出力されるまで、このモジュールは使うべきではありません。

`screen`は [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)です。

**Note:** レンダラ―/デベロッパーツールで、`window.screen`はDOMプロパティで予約されているので、`var screen = require('electron').screen`と書いても動作しません。下の例では、代わりに変数名で`electronScreen`を使用しています。

画面全体にウィンドウを作成する例：

```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow

var mainWindow

app.on('ready', function () {
  var electronScreen = electron.screen
  var size = electronScreen.getPrimaryDisplay().workAreaSize
  mainWindow = new BrowserWindow({ width: size.width, height: size.height })
})
```
外部ディスプレイにウィンドウを作成する別の例：

```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow

var mainWindow

app.on('ready', function () {
  var electronScreen = electron.screen
  var displays = electronScreen.getAllDisplays()
  var externalDisplay = null
  for (var i in displays) {
    if (displays[i].bounds.x != 0 || displays[i].bounds.y != 0) {
      externalDisplay = displays[i]
      break
    }
  }

  if (externalDisplay) {
    mainWindow = new BrowserWindow({
      x: externalDisplay.bounds.x + 50,
      y: externalDisplay.bounds.y + 50
    })
  }
})
```

## `Display` オブジェクト

`Display`オブジェクトはシステムに接続された物理ディスプレイを示します。ヘッドレスシステムでは、擬似`Display`があるかもしれませんし、`Display`はリモートや仮想ディスプレイに相当するかもしれません。

* `display` object
  * `id` Integer - ディスプレイに紐づいた一意な識別子です。
  * `rotation` Integer - 0, 1, 2, 3を設定でき、それぞれは時計回りで、0, 90, 180, 270度の画面の回転を示します。
  * `scaleFactor` Number - 出力装置のピクセルスケールファクター
  * `touchSupport` String - `available`, `unavailable`, `unknown`を設定できます。
  * `bounds` Object
  * `size` Object
  * `workArea` Object
  * `workAreaSize` Object

## イベント

`screen`モジュールは次のイベントを出力します:

### イベント: 'display-added'

返り値:

* `event` Event
* `newDisplay` Object

`newDisplay`が追加されたときに出力されます。

### イベント: 'display-removed'

返り値:

* `event` Event
* `oldDisplay` Object

`oldDisplay`が削除されたときに出力されます。

### イベント: 'display-metrics-changed'

返り値:

* `event` Event
* `display` Object
* `changedMetrics` Array

`display`で1つ以上のメトリックが変わったときに出力されます。`changedMetrics`は変更を説明する文字列の配列です。変更内容には`bounds`と`workArea`, `scaleFactor`、 `rotation`があり得ます。

## メソッド

`screen`モジュールは次のメソッドを持ちます:

### `screen.getCursorScreenPoint()`

現在のマウスの絶対位置を返します。

### `screen.getPrimaryDisplay()`

プライマリディスプレイを返します。

### `screen.getAllDisplays()`

現在利用可能なディスプレイの配列を返します。

### `screen.getDisplayNearestPoint(point)`

* `point` Object
  * `x` Integer
  * `y` Integer

指定したポイントに近いディスプレイを返します。

### `screen.getDisplayMatching(rect)`

* `rect` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

提供された範囲と、もっとも重複しているディスプレイを返します。
