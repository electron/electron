# screen

> 화면 크기, 디스플레이, 커서 위치 등의 정보를 가져옵니다.

이 모듈은 `app` 모듈의 `ready` 이벤트가 발생하기 전까지 사용할 수 없습니다. (호출 또는
모듈 포함)

`screen`은 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)를
상속 받았습니다.

**참고:** 렌더러 / DevTools에선 이미 DOM 속성이 `window.screen`을 가지고 있으므로
`screen = require('screen')` 형식으로 모듈을 사용할 수 없습니다.

다음 예시는 화면 전체를 채우는 윈도우 창을 생성합니다:


```javascript
const electron = require('electron')
const {app, BrowserWindow} = electron

let win

app.on('ready', () => {
  const {width, height} = electron.screen.getPrimaryDisplay().workAreaSize
  win = new BrowserWindow({width, height})
});
```

다음 예시는 확장 디스플레이에 윈도우를 생성합니다:

```javascript
const electron = require('electron')
const {app, BrowserWindow} = require('electron')

let win

app.on('ready', () => {
  let displays = electron.screen.getAllDisplays()
  let externalDisplay = displays.find((display) => {
    return display.bounds.x !== 0 || display.bounds.y !== 0
  })

  if (externalDisplay) {
    win = new BrowserWindow({
      x: externalDisplay.bounds.x + 50,
      y: externalDisplay.bounds.y + 50
    })
  }
})
```

## `Display` 객체

`Display` 객체는 시스템에 연결된 물리적인 디스플레이를 표현합니다. 헤드레스(headless)
시스템에선 가짜 `Display` 객체가 보여지거나 리모트(remote), 가상 디스플레이에
해당하게 됩니다.

* `display` object
  * `id` Integer - 디스플레이에 관련된 유일 식별자.
  * `rotation` Integer - 값은 0, 90, 180, 270이 될 수 있고, 각 값은 시계 방향을
    기준으로 0, 90, 180, 270도의 화면 회전 상태를 표현합니다.
  * `scaleFactor` Number - 기기의 픽셀 스케일 크기.
  * `touchSupport` String - 터치 스크린의 여부, `available`, `unavailable`,
    `unknown` 값으로 반환됩니다.
  * `bounds` Object
  * `size` Object
  * `workArea` Object
  * `workAreaSize` Object

## Events

`screen` 모듈은 다음과 같은 이벤트를 가지고 있습니다:

### Event: 'display-added'

Returns:

* `event` Event
* `newDisplay` Object

새로운 디스플레이가 추가되면 발생하는 이벤트입니다.

### Event: 'display-removed'

Returns:

* `event` Event
* `oldDisplay` Object

기존의 디스플레이가 제거되면 발생하는 이벤트입니다.

### Event: 'display-metrics-changed'

Returns:

* `event` Event
* `display` Object
* `changedMetrics` Array

`display`에서 하나 또는 다수의 매트릭스가 변경될 때 발생하는 이벤트입니다.
`changedMetrics`는 변경에 대한 정보를 담은 문자열의 배열입니다.
`bounds`, `workArea`, `scaleFactor`, `rotation`등이 변경될 수 있습니다.

## Methods

`screen` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `screen.getCursorScreenPoint()`

현재 마우스 포인터의 절대 위치를 반환합니다.

### `screen.getPrimaryDisplay()`

기본 디스플레이를 반환합니다.

### `screen.getAllDisplays()`

사용 가능한 모든 디스플레이를 배열로 반환합니다.

### `screen.getDisplayNearestPoint(point)`

* `point` Object
  * `x` Integer
  * `y` Integer

지정한 좌표에 가까운 디스플레이를 반환합니다.

### `screen.getDisplayMatching(rect)`

* `rect` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

지정한 범위에 가장 가깝게 교차한 디스플레이를 반환합니다.
