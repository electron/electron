# systemPreferences

> 시스템 설정을 가져옵니다.

```javascript
const {systemPreferences} = require('electron')
console.log(systemPreferences.isDarkMode())
```

## Events

`systemPreferences` 객체는 다음 이벤트를 발생시킵니다:

### Event: 'accent-color-changed' _Windows_

Returns:

* `event` Event
* `newColor` String - 사용자에 의해 시스템 강조색으로 설정 된 새 RGBA 색상.

### Event: 'inverted-color-scheme-changed' _Windows_

Returns:

* `event` Event
* `invertedColorScheme` Boolean - 고대비 테마 같은 반전된 색상 스킴을
  사용중이라면 `true`, 아니면 `false`.

## Methods

### `systemPreferences.isDarkMode()` _macOS_

Returns `Boolean` - 시스템이 어두운 모드인지 여부.

### `systemPreferences.isSwipeTrackingFromScrollEventsEnabled()` _macOS_

Returns `Boolean` - 페이지 간의 스와이프가 설정되어 있는지 여부.

### `systemPreferences.postNotification(event, userInfo)` _macOS_

* `event` String
* `userInfo` Object

macOS 의 기본 알림으로 `event` 를 전달합니다. `userInfo` 는 알림과 함께 전송되는
사용자 정보 딕셔너리를 포함하는 객체입니다.

### `systemPreferences.postLocalNotification(event, userInfo)` _macOS_

* `event` String
* `userInfo` Object

macOS 의 기본 알림으로 `event` 를 전달합니다. `userInfo` 는 알림과 함께 전송되는
사용자 정보 딕셔너리를 포함하는 객체입니다.

### `systemPreferences.subscribeNotification(event, callback)` _macOS_

* `event` String
* `callback` Function

macOS의 기본 알림을 구독하며, 해당하는 `event`가 발생하면 `callback`이
`callback(event, userInfo)` 형태로 호출됩니다. `userInfo`는 알림과 함께 전송되는
사용자 정보 딕셔너리를 포함하는 객체입니다.

구독자의 `id`가 반환되며 `event`를 구독 해제할 때 사용할 수 있습니다.

이 API는 후드에서 `NSDistributedNotificationCenter`를 구독하며, `event`의 예시
값은 다음과 같습니다:

* `AppleInterfaceThemeChangedNotification`
* `AppleAquaColorVariantChanged`
* `AppleColorPreferencesChangedNotification`
* `AppleShowScrollBarsSettingChanged`

### `systemPreferences.unsubscribeNotification(id)` _macOS_

* `id` Integer

`id`와 함께 구독자를 제거합니다.

### `systemPreferences.subscribeLocalNotification(event, callback)` _macOS_

* `event` String
* `callback` Function

`subscribeNotification`와 같습니다. 하지만 로컬 기본값에 대해
`NSNotificationCenter`를 사용합니다. 이는 `NSUserDefaultsDidChangeNotification`와
같은 이벤트에 대해 필수적입니다.

### `systemPreferences.unsubscribeLocalNotification(id)` _macOS_

* `id` Integer

`unsubscribeNotification`와 같지만, `NSNotificationCenter`에서 구독자를 제거합니다.

### `systemPreferences.getUserDefault(key, type)` _macOS_

* `key` String
* `type` String - `string`, `boolean`, `integer`, `float`, `double`, `url`,
  `array`, `dictionary` 값이 될 수 있습니다.

시스템 설정에서 `key`에 해당하는 값을 가져옵니다.

macOS에선 API가 `NSUserDefaults`를 읽어들입니다. 유명한 `key`와 `type`은 다음과
같습니다:

* `AppleInterfaceStyle: string`
* `AppleAquaColorVariant: integer`
* `AppleHighlightColor: string`
* `AppleShowScrollBars: string`
* `NSNavRecentPlaces: array`
* `NSPreferredWebServices: dictionary`
* `NSUserDictionaryReplacementItems: array`

### `systemPreferences.isAeroGlassEnabled()` _Windows_

이 메서드는 [DWM 컴포지션][dwm-composition] (Aero Glass)가 활성화 되어있을 때
`true`를 반환합니다. 아닌 경우 `false`를 반환합니다.

다음은 투명한 윈도우를 만들지, 일반 윈도우를 만들지를 판단하여 윈도우를 생성하는
예시입니다 (투명한 윈도우는 DWM 컴포지션이 비활성화되어있을 시 작동하지 않습니다):

```javascript
const {BrowserWindow, systemPreferences} = require('electron')
let browserOptions = {width: 1000, height: 800}

// 플랫폼이 지원하는 경우에만 투명 윈도우를 생성.
if (process.platform !== 'win32' || systemPreferences.isAeroGlassEnabled()) {
  browserOptions.transparent = true
  browserOptions.frame = false
}

// 원도우 생성
let win = new BrowserWindow(browserOptions)

// 페이지 로드.
if (browserOptions.transparent) {
  win.loadURL(`file://${__dirname}/index.html`)
} else {
  // 투명 윈도우 상태가 아니라면, 기본적인 스타일 사용
  win.loadURL(`file://${__dirname}/fallback.html`)
}
```

[dwm-composition]:https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx

### `systemPreferences.getAccentColor()` _Windows_

사용자의 현재 시스템 전체 색상 환경설정을 RGBA 16진 문자열 형태로 반환합니다.

```js
const color = systemPreferences.getAccentColor() // `"aabbccdd"`
const red = color.substr(0, 2) // "aa"
const green = color.substr(2, 2) // "bb"
const blue = color.substr(4, 2) // "cc"
const alpha = color.substr(6, 2) // "dd"
```

### `systemPreferences.isInvertedColorScheme()` _Windows_

Returns `Boolean` - 고대비 테마 같은 반전된 색상 스킴이 활성화 되있다면 `true`,
아니면 `false`.
