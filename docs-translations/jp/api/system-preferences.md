# systemPreferences

> システムの環境設定を取得します。

```javascript
const {systemPreferences} = require('electron')
console.log(systemPreferences.isDarkMode())
```

## メソッド

### `systemPreferences.isDarkMode()` _macOS_

macOS がダークモードならば `true` を返し、通常モードなら `false` を返します。

### `systemPreferences.subscribeNotification(event, callback)` _macOS_

* `event` String
* `callback` Function

macOS のネイティブな通知を購読します。 `callback` は `callback(event, userInfo)` として `event` の発生に対応して呼ばれます。`userInfo` は通知によって送られてくるユーザー情報のオブジェクトです。

この関数が返す `id` は `event`の購読をやめる際に使用します。

内部ではこの API は `NSDistributedNotificationCenter` を購読するので、`event` の例は以下のようなものがあります。

* `AppleInterfaceThemeChangedNotification`
* `AppleAquaColorVariantChanged`
* `AppleColorPreferencesChangedNotification`
* `AppleShowScrollBarsSettingChanged`

### `systemPreferences.unsubscribeNotification(id)` _macOS_

* `id` Integer

`id` の購読をやめます。

### `systemPreferences.subscribeLocalNotification(event, callback)` _macOS_

`subscribeNotification` と同じですが、 `NSNotificationCenter` を購読します。下記のような `event` を捕まえるために必要です。

* `NSUserDefaultsDidChangeNotification`

### `systemPreferences.unsubscribeLocalNotification(id)` _macOS_

`unsubscribeNotification` と同じですが、`NSNotificationCenter` による購読をやめます。

### `systemPreferences.getUserDefault(key, type)` _macOS_

* `key` String
* `type` String - 右記の値を入れられます `string`, `boolean`, `integer`, `float`, `double`,
  `url`, `array`, `dictionary`

システム環境設定の `key` の値を取得します。

この API は macOS の `NSUserDefaults` から情報を取得します。よく使われる `key` 及び `type` には下記のものがあります。

* `AppleInterfaceStyle: string`
* `AppleAquaColorVariant: integer`
* `AppleHighlightColor: string`
* `AppleShowScrollBars: string`
* `NSNavRecentPlaces: array`
* `NSPreferredWebServices: dictionary`
* `NSUserDictionaryReplacementItems: array`

### `systemPreferences.isAeroGlassEnabled()` _Windows_
[DWM composition][dwm-composition] (Aero Glass) が有効だと `true` を返し、そうでないと `false` を返します。

使用例として、例えば透過ウィンドウを作成するかしないか決めるときに使います(DWM composition が無効だと透過ウィンドウは正常に動作しません)

```javascript
const {BrowserWindow, systemPreferences} = require('electron')
let browserOptions = {width: 1000, height: 800}

// プラットフォームがサポートしている場合に限り透過ウィンドウを作成します。
if (process.platform !== 'win32' || systemPreferences.isAeroGlassEnabled()) {
  browserOptions.transparent = true
  browserOptions.frame = false
}

// ウィンドウを作成
let win = new BrowserWindow(browserOptions)

// 分岐
if (browserOptions.transparent) {
  win.loadURL(`file://${__dirname}/index.html`)
} else {
  // 透過がサポートされてないなら、通常のスタイルの html を表示する
  win.loadURL(`file://${__dirname}/fallback.html`)
}
```

[dwm-composition]:https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx
