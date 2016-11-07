# screen

> 화면 크기, 디스플레이, 커서 위치 등의 정보를 가져옵니다.

프로세스: [메인](../tutorial/quick-start.md#main-process), [렌더러](../tutorial/quick-start.md#renderer-process)

이 모듈은 `app` 모듈의 `ready` 이벤트가 발생하기 전까지 포함하거나 사용할 수
없습니다.

`screen`은 [EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter)를
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
})
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

## Events

`screen` 모듈은 다음과 같은 이벤트를 가지고 있습니다:

### Event: 'display-added'

Returns:

* `event` Event
* `newDisplay` [Display](structures/display.md)

`newDisplay` 가 추가되면 발생하는 이벤트입니다.

### Event: 'display-removed'

Returns:

* `event` Event
* `oldDisplay` [Display](structures/display.md)

`oldDisplay` 가 제거되면 발생하는 이벤트입니다.

### Event: 'display-metrics-changed'

Returns:

* `event` Event
* `display` [Display](structures/display.md)
* `changedMetrics` String[]

`display`에서 하나 또는 다수의 매트릭스가 변경될 때 발생하는 이벤트입니다.
`changedMetrics`는 변경에 대한 정보를 담은 문자열의 배열입니다.
`bounds`, `workArea`, `scaleFactor`, `rotation`등이 변경될 수 있습니다.

## Methods

`screen` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `screen.getCursorScreenPoint()`

Returns `Object`:

* `x` Integer
* `y` Integer

현재 마우스 포인터의 절대 위치.

### `screen.getPrimaryDisplay()`

Returns `Display` - 기본 디스플레이.

### `screen.getAllDisplays()`

Returns `Display[]` - 사용 가능한 모든 디스플레이의 배열.

### `screen.getDisplayNearestPoint(point)`

* `point` Object
  * `x` Integer
  * `y` Integer

Returns `Display` - 지정한 좌표에 가까운 디스플레이.

### `screen.getDisplayMatching(rect)`

* `rect` [Rectangle](structures/rectangle.md)

Returns `Display` - 지정한 범위에 가장 가깝게 교차한 디스플레이.
