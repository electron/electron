# app

> 애플리케이션의 이벤트 생명주기를 제어합니다.

밑의 예시는 마지막 윈도우가 종료되었을 때, 애플리케이션을 종료시키는 예시입니다:

```javascript
const {app} = require('electron')
app.on('window-all-closed', () => {
  app.quit()
})
```

## Events

`app` 객체는 다음과 같은 이벤트를 가지고 있습니다:

### Event: 'will-finish-launching'

애플리케이션이 기본적인 시작 준비를 마치면 발생하는 이벤트입니다.
Windows, Linux 운영체제에서의 `will-finish-launching` 이벤트는 `ready` 이벤트와
동일합니다. macOS에서의 이벤트는 `NSApplication`의
`applicationWillFinishLaunching`에 대한 알림으로 표현됩니다. 대개 이곳에서
`open-file`과 `open-url` 이벤트 리스너를 설정하고 crash reporter와 auto updater를
시작합니다.

대부분의 경우, 모든 것을 `ready` 이벤트 핸들러 안에서 해결해야 합니다.

### Event: 'ready'

Returns:

* `launchInfo` Object _macOS_

Electron이 초기화를 끝냈을 때 발생하는 이벤트입니다.

### Event: 'window-all-closed'

모든 윈도우가 종료되었을 때 발생하는 이벤트입니다.

만약 이 이벤트를 구독하지 않은 상태로 모든 윈도우가 닫혔을 때의 기본 동작은 앱을
종료하는 것입니다. 하지만, 이 이벤트를 구독하면, 앱을 종료할지 다른 일을 할지 제어할
수 있습니다. 만약 사용자가 `Cmd + Q`를 입력했거나 개발자가 `app.quit()`를 호출했다면,
Electron은 먼저 모든 윈도우의 종료를 시도하고 `will-quit` 이벤트를 발생시킵니다.
그리고 `will-quit` 이벤트가 발생했을 땐 `window-all-closed` 이벤트가 발생하지
않습니다.

**역자주:** 이 이벤트는 말 그대로 현재 애플리케이션에서 윈도우만 완전히 종료됬을 때
발생하는 이벤트입니다. 따라서 애플리케이션을 완전히 종료하려면 이 이벤트에서
`app.quit()`를 호출해 주어야 합니다.

### Event: 'before-quit'

Returns:

* `event` Event

애플리케이션 윈도우들이 닫히기 시작할 때 발생하는 이벤트입니다.
`event.preventDefault()` 호출은 이벤트의 기본 동작을 방지하기 때문에
이를 통해 애플리케이션의 종료를 방지할 수 있습니다.

### Event: 'will-quit'

Returns:

* `event` Event

모든 윈도우들이 종료되고 애플리케이션이 종료되기 시작할 때 발생하는 이벤트입니다.
`event.preventDefault()` 호출을 통해 애플리케이션의 종료를 방지할 수 있습니다.

`will-quit` 와 `window-all-closed` 이벤트의 차이점을 확인하려면 `window-all-closed`
이벤트의 설명을 참고하세요.

### Event: 'quit'

Returns:

* `event` Event
* `exitCode` Integer

애플리케이션이 종료될 때 발생하는 이벤트입니다.

### Event: 'open-file' _macOS_

Returns:

* `event` Event
* `path` String

사용자가 애플리케이션을 통해 파일을 열고자 할 때 발생하는 이벤트입니다.

`open-file` 이벤트는 보통 애플리케이션이 열려 있을 때 OS가 파일을 열기 위해
애플리케이션을 재사용할 때 발생합니다. 이 이벤트는 파일을 dock에 떨어트릴 때,
애플리케이션이 실행되기 전에도 발생합니다. 따라서 이 이벤트를 제대로 처리하려면
`open-file` 이벤트 핸들러를 애플리케이션이 시작하기 전에 등록해 놓았는지 확실히
확인해야 합니다. (`ready` 이벤트가 발생하기 전에)

이 이벤트를 처리할 땐 반드시 `event.preventDefault()`를 호출해야 합니다.

Windows에선 `process.argv` (메인 프로세스에서)를 통해 파일 경로를 얻을 수 있습니다.

### Event: 'open-url' _macOS_

Returns:

* `event` Event
* `url` String

유저가 애플리케이션을 통해 URL을 열고자 할 때 발생하는 이벤트입니다.
애플리케이션에서 URL을 열기 위해 반드시 URL 스킴이 등록되어 있어야 합니다.

이 이벤트를 처리할 땐 반드시 `event.preventDefault()`를 호출해야 합니다.

### Event: 'activate' _macOS_

Returns:

* `event` Event
* `hasVisibleWindows` Boolean

애플리케이션이 활성화 되었을 때 발생하는 이벤트입니다. 이 이벤트는 사용자가
애플리케이션의 dock 아이콘을 클릭했을 때 주로 발생합니다.

### Event: 'continue-activity' _macOS_

Returns:

* `event` Event
* `type` String - Activity를 식별하는 문자열.
  [`NSUserActivity.activityType`][activity-type]을 맵핑합니다.
* `userInfo` Object - 다른 기기의 activity에서 저장된 앱-특정 상태를 포함합니다.

다른 기기에서 받아온 activity를 재개하려고 할 때 [Handoff][handoff] 하는 동안
발생하는 이벤트입니다. 이 이벤트를 처리하려면 반드시 `event.preventDefault()`를
호출해야 합니다.

사용자 activity는 activity의 소스 애플리케이션과 같은 개발자 팀 ID를 가지는
애플리케이션 안에서만 재개될 수 있고, activity의 타입을 지원합니다. 지원하는
activity의 타입은 애플리케이션 `Info.plist`의 `NSUserActivityTypes` 키에 열거되어
있습니다.

### Event: 'browser-window-blur'

Returns:

* `event` Event
* `window` BrowserWindow

[browserWindow](browser-window.md)에 대한 포커스가 사라졌을 때 발생하는 이벤트입니다.

### Event: 'browser-window-focus'

Returns:

* `event` Event
* `window` BrowserWindow

[browserWindow](browser-window.md)에 대한 포커스가 발생했을 때 발생하는 이벤트입니다.

**역자주:** _포커스_ 는 창을 클릭해서 활성화 시켰을 때를 말합니다.

### Event: 'browser-window-created'

Returns:

* `event` Event
* `window` BrowserWindow

새로운 [browserWindow](browser-window.md)가 생성되었을 때 발생하는 이벤트입니다.

### Event: 'web-contents-created'

Returns:

* `event` Event
* `webContents` WebContents

새로운 [webContents](web-contents.md)가 생성되었을 때 발생하는 이벤트입니다.

### Event: 'certificate-error'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `error` String - 에러 코드
* `certificate` Object
  * `data` String - PEM 인코딩된 데이터
  * `issuerName` String - 인증서 발급자의 공통 이름
  * `subjectName` String - 대상의 공통 이름
  * `serialNumber` String - 문자열로 표현된 hex 값
  * `validStart` Integer - 초 단위의 인증서가 유효하기 시작한 날짜
  * `validExpiry` Integer - 초 단위의 인증서가 만료되는 날짜
  * `fingerprint` String - 인증서의 지문
* `callback` Function
  * `isTrusted` Boolean - 인증서를 신뢰할지 여부

`url`에 대한 `certificate` 인증서의 유효성 검증에 실패했을 때 발생하는 이벤트입니다.
인증서를 신뢰한다면 `event.preventDefault()` 와 `callback(true)`를 호출하여
기본 동작을 방지하고 인증을 승인할 수 있습니다.

```javascript
const {app} = require('electron')

app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  if (url === 'https://github.com') {
    // 확인 로직.
    event.preventDefault()
    callback(true)
  } else {
    callback(false)
  }
})
```

### Event: 'select-client-certificate'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `certificateList` [Certificate[]](structures/certificate.md)
* `callback` Function
  * `certificate` [Certificate](structures/certificate.md)

클라이언트 인증이 요청되었을 때 발생하는 이벤트입니다.

`url`은 클라이언트 인증서를 요청하는 탐색 항목에 해당합니다.
그리고 `callback`은 목록에서 필터링된 항목과 함께 호출될 필요가 있습니다.
이 이벤트에서의 `event.preventDefault()` 호출은 초기 인증 때 저장된 데이터를 사용하는
것을 막습니다.

```javascript
const {app} = require('electron')

app.on('select-client-certificate', (event, webContents, url, list, callback) => {
  event.preventDefault()
  callback(list[0])
})
```

### Event: 'login'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function
  * `username` String
  * `password` String

`webContents`가 기본 인증을 요청할 때 발생하는 이벤트입니다.

기본 동작은 인증 요청을 모두 취소시킵니다.
동작을 새로 정의하려면 반드시 `event.preventDefault()`를 호출한 후
`callback(username, password)` 형태의 콜백을 호출하여 인증을 처리해야 합니다.

```javascript
const {app} = require('electron')

app.on('login', (event, webContents, request, authInfo, callback) => {
  event.preventDefault()
  callback('username', 'secret')
})
```

### Event: 'gpu-process-crashed'

Returns:

* `event` Event
* `killed` Boolean

GPU 처리가 충돌하거나 종료되었을 때 발생하는 이벤트입니다.

### Event: 'accessibility-support-changed' _macOS_ _Windows_

Returns:

* `event` Event
* `accessibilitySupportEnabled` Boolean - Chrome의 접근성 지원이 활성화되어있으면
  `true`를 반환하고 아니라면 `false`를 반환합니다.

Chrome의 접근성 지원이 변경될 때 발생하는 이벤트입니다. 이 이벤트는 스크린 리더와 같은
접근성 기술이 활성화되거나 비활성화될 때 발생합니다.
자세한 내용은 https://www.chromium.org/developers/design-documents/accessibility 를
참고하세요.

## Methods

`app` 객체는 다음과 같은 메서드를 가지고 있습니다:

**참고:** 몇몇 메서드는 특정 플랫폼에서만 작동합니다.

### `app.quit()`

모든 윈도우 종료를 시도합니다. `before-quit` 이벤트가 먼저 발생합니다.
모든 윈도우가 성공적으로 종료되면 `will-quit` 이벤트가 발생하고 기본 동작에 따라
애플리케이션이 종료됩니다.

이 함수는 모든 `beforeunload`와 `unload` 이벤트 핸들러가 제대로 실행됨을 보장합니다.
`beforeunload` 이벤트 핸들러에서 `false`를 반환했을 때 윈도우 종료가 취소 될 수
있습니다.

### `app.exit(exitCode)`

* `exitCode` Integer

`exitCode`와 함께 애플리케이션을 즉시 종료합니다.

모든 윈도우는 사용자의 동의 여부에 상관없이 즉시 종료되며 `before-quit` 이벤트와
`will-quit` 이벤트가 발생하지 않습니다.

### `app.relaunch([options])`

* `options` Object (optional)
  * `args` String[] (optional)
  * `execPath` String (optional)

현재 인스턴스가 종료되면 애플리케이션을 재시작합니다.

기본적으로 새 인스턴스는 같은 작업 디렉토리의 것과 함께 현재 인스턴스의 명령줄 인수를
사용합니다. 만약 `args`가 지정되면, `args`가 기본 명령줄 인수 대신에 전달됩니다.
`execPath`가 지정되면, 현재 애플리케이션 대신 `execPath`가 실행됩니다.

참고로 이 메서드는 애플리케이션을 종료하지 않으며, 애플리케이션을 다시 시작시키려면
`app.relaunch`를 호출한 후 `app.quit` 또는 `app.exit`를 실행해주어야 합니다.

`app.relaunch`가 여러 번 호출되면, 현재 인스턴스가 종료된 후 여러 인스턴스가
시작됩니다.

다음은 현재 인스턴스를 즉시 종료시킨 후 새로운 명령줄 인수를 추가하여 새
인스턴스의 애플리케이션을 실행하는 예시입니다:

```javascript
const {app} = require('electron')

app.relaunch({args: process.argv.slice(1).concat(['--relaunch'])})
app.exit(0)
```

### `app.isReady()`

Returns `Boolean` - Electron 이 초기화를 마쳤으면 `true`, 아니면 `false`.

### `app.focus()`

Linux에선, 첫 번째로 보여지는 윈도우가 포커스됩니다. macOS에선, 애플리케이션을 활성화
앱 상태로 만듭니다. Windows에선, 애플리케이션의 첫 윈도우에 포커스 됩니다.

### `app.hide()` _macOS_

최소화를 하지 않고 애플리케이션의 모든 윈도우들을 숨깁니다.

### `app.show()` _macOS_

숨긴 애플리케이션 윈도우들을 다시 보이게 만듭니다. 자동으로 포커스되지 않습니다.

### `app.getAppPath()`

Returns `String` - 현재 애플리케이션 디렉토리.

### `app.getPath(name)`

* `name` String

Returns `String` -`name` 에 관련한 특정 디렉터리 또는 파일의 경로. 실패시
`Error` 발생.

**역자주:** 이 메서드는 운영체제에서 지정한 특수 디렉터리를 가져오는데 사용할 수 있습니다.

`name`은 다음 목록에 있는 경로 중 하나를 선택해 사용할 수 있습니다:

* `home` - 사용자의 홈 디렉터리.
* `appData` - 각 사용자의 애플리케이션 데이터 디렉터리. 기본 경로는 다음과 같습니다:
  * `%APPDATA%` - Windows
  * `$XDG_CONFIG_HOME` 또는 `~/.config` - Linux
  * `~/Library/Application Support` - macOS
* `userData` - 애플리케이션의 설정을 저장하는 디렉터리.
  이 디렉터리는 기본적으로 `appData`에 애플리케이션 이름으로 생성된 폴더가 지정됩니다.
* `temp` - 임시 폴더 디렉터리.
* `exe` - 현재 실행중인 Electron 바이너리 파일.
* `module` - `libchromiumcontent` 라이브러리.
* `desktop` - 사용자의 데스크탑 디렉터리.
* `documents` - 사용자의 "내 문서" 디렉터리.
* `downloads` - 사용자의 다운로드 디렉터리.
* `music` - 사용자의 음악 디렉터리.
* `pictures` - 사용자의 사진 디렉터리.
* `videos` - 사용자의 동영상 디렉터리.
* `pepperFlashSystemPlugin` - 시스템 버전의 Pepper Flash 플러그인의 전체 경로.

### `app.setPath(name, path)`

* `name` String
* `path` String

`name`에 대한 특정 디렉터리나 파일의 경로인 `path`를 재정의합니다.
만약 지정한 디렉터리의 경로가 존재하지 않으면 디렉터리가 이 메서드를 통해 새로
생성됩니다. 재정의에 실패했을 땐 `Error`를 반환합니다.

이 메서드는 `app.getPath`에 정의되어 있는 `name`의 경로만 재정의할 수 있습니다.

기본적으로, 웹 페이지의 쿠키와 캐시는 `userData` 디렉터리에 저장됩니다.
만약 이 위치를 변경하고자 한다면, 반드시 `app` 모듈의 `ready` 이벤트가 발생하기 전에
`userData` 경로를 재정의해야 합니다.

### `app.getVersion()`

Returns `String` - 로드된 애플리케이션의 버전. 만약 `package.json` 파일에서
애플리케이션의 버전을 찾을 수 없는 경우, 현재 번들 또는 실행 파일의 버전을
반환합니다.

### `app.getName()`

Returns `String` - `package.json` 에 기술된 현재 애플리케이션의 이름.

npm 모듈 규칙에 따라 대부분의 경우 `package.json`의 `name` 필드는 소문자 이름을
사용합니다. 하지만 Electron은 `name`대신 `productName` 필드를 주로 사용하기 때문에
반드시 이 필드도 같이 지정해야 합니다. 이 필드는 맨 앞글자가 대문자인 애플리케이션
전체 이름을 지정해야 합니다.

### `app.setName(name)`

* `name` String

현재 애플리케이션의 이름을 덮어씌웁니다.

### `app.getLocale()`

Returns `String` - 현재 애플리케이션의
[로케일](https://ko.wikipedia.org/wiki/%EB%A1%9C%EC%BC%80%EC%9D%BC). 반환될 수
있는 값은 [여기](locales.md)에서 찾아볼 수 있습니다.

**참고:** 패키징된 앱을 배포할 때, `locales` 폴더도 같이 배포해야 합니다.

**참고:** Windows에선 `ready` 이벤트가 발생한 이후에 이 메서드를 호출해야 합니다.

### `app.addRecentDocument(path)` _macOS_ _Windows_

* `path` String

최근 문서 목록에 `path`를 추가합니다.

이 목록은 OS에 의해 관리됩니다. 최근 문서 목록은 Windows의 경우 작업 표시줄에서 찾을
수 있고, macOS의 경우 dock 메뉴에서 찾을 수 있습니다.

### `app.clearRecentDocuments()` _macOS_ _Windows_

최근 문서 목록을 모두 비웁니다.

### `app.setAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - 프로토콜의 이름, `://` 제외. 만약 앱을 통해 `electron://`과
  같은 링크를 처리하고 싶다면, 이 메서드에 `electron` 인수를 담아 호출하면 됩니다.
* `path` String (optional) _Windows_ - 기본값은 `process.execPath`입니다.
* `args` String[] (optional) _Windows_ - 기본값은 빈 배열입니다.

Returns `Boolean` - 호출 성공 여부.

이 메서드는 지정한 프로토콜(URI scheme)에 대해 현재 실행파일을 기본 핸들러로
등록합니다. 이를 통해 운영체제와 더 가깝게 통합할 수 있습니다. 한 번 등록되면,
`your-protocol://`과 같은 모든 링크에 대해 호출시 현재 실행 파일이 실행됩니다.
모든 링크, 프로토콜을 포함하여 애플리케이션의 인수로 전달됩니다.

Windows에선 실행시에 선택적 매개변수를 통해 경로, 실행 파일, 인수, 실행 파일로 전달될
인수의 배열을 제공할 수 있습니다.

**참고:** macOS에선, 애플리케이션의 `info.plist`에 등록해둔 프로토콜만 사용할 수
있습니다. 이는 런타임에서 변경될 수 없습니다. 이 파일은 간단히 텍스트 에디터를
사용하거나, 애플리케이션을 빌드할 때 스크립트가 생성되도록 할 수 있습니다. 자세한
내용은 [Apple의 참조 문서][CFBundleURLTypes]를 확인하세요.

이 API는 내부적으로 Windows 레지스트리와 LSSetDefaultHandlerForURLScheme를 사용합니다.

호출에 성공하면 `true`를 반환하고 그렇지 않다면 `false`를 반환합니다.

### `app.removeAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - 프로토콜의 이름, `://` 제외.
* `path` String (optional) _Windows_ - 기본값은 `process.execPath`
* `args` String[] (optional) _Windows_ - 기본값은 빈 배열

Returns `Boolean` - 호출 성공 여부.

이 메서드는 현재 실행파일이 지정한 프로토콜(URI scheme)에 대해 기본 핸들러인지를
확인합니다. 만약 그렇다면, 이 메서드는 앱을 기본 핸들러에서 제거합니다.

### `app.isDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - `://`를 제외한 프로토콜의 이름.
* `path` String (optional) _Windows_ - 기본값은 `process.execPath`
* `args` String[] (optional) _Windows_ - 기본값은 빈 배열

Returns `Boolean`

이 메서드는 현재 실행 파일이 지정한 프로토콜에 대해 기본 동작인지 확인합니다.
(URI 스킴) 만약 그렇다면 `true`를 반환하고 아닌 경우 `false`를 반환합니다.

**참고:** macOS에선, 응용 프로그램이 프로토콜에 대한 기본 프로토콜 동작으로
등록되었는지를 확인하기 위해 이 메서드를 사용할 수 있습니다. 또한 macOS에서
`~/Library/Preferences/com.apple.LaunchServices.plist`를 확인하여 검증할 수도
있습니다. 자세한 내용은 [Apple의 참조 문서][LSCopyDefaultHandlerForURLScheme]를
참고하세요.

이 API는 내부적으로 Windows 레지스트리와 LSSetDefaultHandlerForURLScheme를 사용합니다.

### `app.setUserTasks(tasks)` _Windows_

* `tasks` [Task[]](structures/task.md) - `Task` 객체의 배열

Windows에서 사용할 수 있는 JumpList의 [Tasks][tasks] 카테고리에 `task`를 추가합니다.

`tasks`는 다음과 같은 구조를 가지는 `Task` 객체의 배열입니다:

`Task` Object:

* `program` String - 실행할 프로그램의 경로.
  보통 현재 작동중인 애플리케이션의 경로인 `process.execPath`를 지정합니다.
* `arguments` String - `program`이 실행될 때 사용될 명령줄 인수.
* `title` String - JumpList에 표시할 문자열.
* `description` String - 이 작업에 대한 설명.
* `iconPath` String - JumpList에 표시될 아이콘의 절대 경로. 아이콘을 포함하고
  있는 임의의 리소스 파일을 사용할 수 있습니다. 보통 애플리케이션의 아이콘을
  그대로 사용하기 위해 `process.execPath`를 지정합니다.
* `iconIndex` Integer - 아이콘 파일의 인덱스. 만약 아이콘 파일이 두 개 이상의
  아이콘을 가지고 있을 경우, 사용할 아이콘의 인덱스를 이 옵션으로 지정해 주어야
  합니다. 단, 아이콘을 하나만 포함하고 있는 경우 0을 지정하면 됩니다.

Returns `Boolean` - 호출 성공 여부.

**참고:** 점프 목록을 커스터마이징 하려면 대신 `app.setJumpList(categories)` 를
사용하세요.

### `app.getJumpListSettings()` _Windows_

Returns `Object`:

* `minItems` Integer - 점프 목록에서 보여질 항목의 최소 수 (이 값에 대한 자세한
  설명은 [MSDN 문서][JumpListBeginListMSDN])를 보세요.
* `removedItems` [JumpListItem[]](structures/jump-list-item.md) - 점프 목록의 사용자 정의 카테고리에서 사용자가 삭제한
  항목에 해당하는 `JumpListItem` 객체 배열. 이 항목들은 **다음**
  `app.setJumpList()` 호출로 다시 추가하면 안됩니다. 윈도우는 삭제된 항목을
  포함하는 카테고리를 표시하지 않을 것 입니다.

### `app.setJumpList(categories)` _Windows_

* `categories` [JumpListCategory[]](structures/jump-list-category.md) or `null` - `JumpListCategory` 객체의 배열.

애플리케이션에 사용자 정의 점프 목록을 설정하거나 삭제하고 다음 문자열 중 하나를
반환:

* `ok` - 잘못된 것이 없음.
* `error` - 하나 이상의 에러 발생. 가능성 높은 원인을 파악하기 위해 런타임 로그
  활성화하세요.
* `invalidSeparatorError` - 점프 목록의 사용자 정의 카테고리에서 구분자 추가
  시도. 구분자는 표준 `Tasks` 카테고리에서만 가능 합니다.
* `fileTypeRegistrationError` - 앱이 등록하지 않은 파일 유형을 점프 목록에
  추가하려고 시도함.
* `customCategoryAccessDeniedError` - 사용자 개인 정보 보호와 그룹 정책 설정에
  따라 점프 목록에 사용자 정의 카테고리 추가가 불가능 합니다.

만약 `categories` 가 `null` 이면 이전 사용자 점프 목록 설정은 앱을 위한 표준
점프 목록으로 대체됩니다 (윈도우에 의해 관리됨).

`JumpListCategory` 객체는 다음 속성을 가져야 합니다:

* `type` String - 다음 중 하나:
  * `tasks` - 이 카테고리의 항목은 표준 `Tasks` 카테고리에 위치할 것 입니다.
    이 카테고리는 하나만 존재하며, 항상 점프 목록의 하단에 보여집니다.
  * `frequent` - 앱에 의해 자주 열린 파일의 목록을 보여줍니다. 카테고리의
    이름과 항목들은 윈도우에 의해 설정 됩니다.
  * `recent` - 앱에 의해 최근에 열린 파일의 목록을 보여줍니다. 카테고리의
    이름과 항목들은 윈도우에 의해 설정 됩니다. `app.addRecentDocument(path)` 을
    사용하면 간접적으로 이 카테고리에 항목이 추가될 것 입니다.
  * `custom` - 작업 또는 파일 링크를 보여주며, 앱에 의해 `name` 설정되어야 합니다.
* `name` String - `type` 이 `custom` 이면 꼭 설정되어야 하고, 그 외는 생략합니다.
* `items` Array - `type` 이 `tasks` 면 `JumpListItem` 객체의 배열, 그 외는
  생략합니다.

**참고:** `JumpListCategory` 객체가 `type`, `name` 속성 둘 다 없다면 `type` 은
`tasks` 로 가정합니다. `name` 속성이 설정되었지만 `type` 속성이 생략된 경우
`type` 은 `custom` 으로 가정합니다.

**참고:** 사용자는 사용자 카테고리에서 항목을 삭제할 수 있습니다. 그리고 윈도우는
`app.setJumpList(categories)` 의 다음 성공적인 호출 이후까지 삭제된 항목을 다시
추가하는 것을 금지할 것 입니다. 그 이전에 커스텀 카테고리에 삭제된 항목을 다시
추가하려 한다면 커스텀 카테고리가 전부 점프 목록에서 빠질 것 입니다. 제거된 항목
목록은 `app.getJumpListSettings()` 를 사용해 얻을 수 있습니다.

`JumpListItem` 객체는 다음 속성을 가져야 합니다:

* `type` String - 다음 중 하나:
  * `task` - 특정 인수로 앱을 실행시킬 작업.
  * `separator` - 표준 `Tasks` 카테고리에서 항목을 구분할 수 있습니다.
  * `file` - 점프 목록을 만든 앱을 사용하여 파일을 열 파일 링크. 이것이
    동작하려면 그 파일 형식을 앱이 처리할 수 있게 등록되있어야 합니다. (하지만,
    그것이 기본 처리기일 필요는 없습니다.).
* `path` String - 파일을 열기 위한 경로. `type` 이 `file` 경우에만 설정되어야
  합니다.
* `program` String - 실행하기 위한 프로그램의 경로. 일반적으로 현재 프로그램을
  열기 위해 `process.execPath` 를 지정해야 합니다.
* `args` String - `program` 이 실행됐을 때의 커맨드 라인 인수. `type` 이
  `task` 일 경우만 설정되어야 합니다.
* `title` String - 점프 목록에서 항목에 표시될 글자. `type` 이 `task` 일 경우만
  설정되어야 합니다.
* `description` String - 작업의 설명 (툴팁으로 표시됨). `type` 이 `task` 일
  경우만 설정되어야 합니다.
* `iconPath` String - 점프 목록에서 보여질 아이콘의 절대 경로. 아이콘을 포함하는
  임의의 자원 파일 경로일 수 있습니다. (예. `.ico`, `.exe`, `.dll`). 일반적으로
  프로그램 아이콘을 보여주기 위해 `process.execPath` 를 명시할 수 있습니다.
* `iconIndex` Integer - 리소스 파일의 아이콘 인덱스. 리소스 파일이 여러 아이콘을
  포함하고 있다면 이 작업을 위해 표시되어야 할 아이콘의 0 기준 인덱스를 명시할
  수 있다. 리소스 파일이 하나의 아이콘만 가지고 있다면 이 속성은 0 이어야 합니다.

사용자 점프 목록을 생성하는 간단한 예제 입니다:

```javascript
const {app} = require('electron')

app.setJumpList([
  {
    type: 'custom',
    name: 'Recent Projects',
    items: [
      { type: 'file', path: 'C:\\Projects\\project1.proj' },
      { type: 'file', path: 'C:\\Projects\\project2.proj' }
    ]
  },
  { // has a name so `type` is assumed to be "custom"
    name: 'Tools',
    items: [
      {
        type: 'task',
        title: 'Tool A',
        program: process.execPath,
        args: '--run-tool-a',
        icon: process.execPath,
        iconIndex: 0,
        description: 'Runs Tool A'
      },
      {
        type: 'task',
        title: 'Tool B',
        program: process.execPath,
        args: '--run-tool-b',
        icon: process.execPath,
        iconIndex: 0,
        description: 'Runs Tool B'
      }
    ]
  },
  { type: 'frequent' },
  { // has no name and no type so `type` is assumed to be "tasks"
    items: [
      {
        type: 'task',
        title: 'New Project',
        program: process.execPath,
        args: '--new-project',
        description: 'Create a new project.'
      },
      { type: 'separator' },
      {
        type: 'task',
        title: 'Recover Project',
        program: process.execPath,
        args: '--recover-project',
        description: 'Recover Project'
      }
    ]
  }
])
```

### `app.makeSingleInstance(callback)`

* `callback` Function
  * `argv` String[] - 두번째 인스턴스의 명령줄 인수의 배열
  * `workingDirectory` String - 두번째 인스턴스의 작업 디렉토리

현재 애플리케이션을 단일 인스턴스 애플리케이션으로 만들어줍니다. 이 메서드는
애플리케이션이 여러 번 실행됐을 때 다중 인스턴스가 생성되는 대신 한 개의 주
인스턴스만 유지되도록 만들 수 있습니다. 이때 중복 생성된 인스턴스는 주 인스턴스에
신호를 보내고 종료됩니다.

`callback`은 주 인스턴스가 생성된 이후 또 다른 인스턴스가 생성됐을 때
`callback(argv, workingDirectory)` 형식으로 호출됩니다. `argv`는 두 번째
인스턴스의 명령줄 인수이며 `workingDirectory`는 현재 작업중인 디렉터리입니다.
보통 대부분의 애플리케이션은 이러한 콜백이 호출될 때 주 윈도우를 포커스하고
최소화되어있으면 창 복구를 실행합니다.

`callback`은 `app`의 `ready` 이벤트가 발생한 후 실행됨을 보장합니다.

이 메서드는 현재 실행된 애플리케이션이 주 인스턴스인 경우 `false`를 반환하고
애플리케이션의 로드가 계속 진행 되도록 합니다. 그리고 두 번째 중복된 인스턴스
생성인 경우 `true`를 반환합니다. (다른 인스턴스에 인수가 전달됬을 때) 이 불리언
값을 통해 중복 생성된 인스턴스는 즉시 종료시켜야 합니다.

macOS에선 사용자가 Finder에서 애플리케이션의 두 번째 인스턴스를 열려고 했을 때
자동으로 단일 인스턴스화 하고 `open-file`과 `open-url` 이벤트를 발생시킵니다.
그러나 사용자가 애플리케이션을 CLI 터미널에서 실행하면 운영체제 시스템의 싱글
인스턴스 메커니즘이 무시되며 그대로 중복 실행됩니다. 따라서 macOS에서도 이
메서드를 통해 확실히 중복 실행을 방지하는 것이 좋습니다.

다음 예시는 두 번째 인스턴스가 생성되었을 때 중복된 인스턴스를 종료하고 주
애플리케이션 인스턴스의 윈도우를 활성화 시키는 예시입니다:

```javascript
const {app} = require('electron')
let myWindow = null

const shouldQuit = app.makeSingleInstance((commandLine, workingDirectory) => {
  // 애플리케이션을 중복 실행했습니다. 주 애플리케이션 인스턴스를 활성화 합니다.
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore()
    myWindow.focus()
  }
})

if (shouldQuit) {
  app.quit()
}

// 윈도우를 생성하고 각종 리소스를 로드하고 작업합니다.
app.on('ready', () => {
})
```

### `app.releaseSingleInstance()`

모든 `makeSingleInstance`에 의해 생성된 제한을 해제합니다. 이 메서드는 다시 여러
인스턴스의 애플리케이션이 나란히 실행될 수 있도록 합니다.

### `app.setUserActivity(type, userInfo[, webpageURL])` _macOS_

* `type` String - 고유하게 activity를 식별합니다.
  [`NSUserActivity.activityType`][activity-type]을 맵핑합니다.
* `userInfo` Object - 다른 기기에서 사용하기 위해 저장할 앱-특정 상태.
* `webpageURL` String - 적당한 앱이 기기에 설치되지 않았을 때 브라우저에서 로드할
  웹 페이지. 스킴은 반드시 `http` 또는 `https`가 되어야 합니다.

`NSUserActivity`를 만들고 현재 activity에 설정합니다. 이 activity는 이후 다른
기기와 [Handoff][handoff]할 때 자격으로 사용됩니다.

### `app.getCurrentActivityType()` _macOS_

Returns `String` - 현재 작동중인 activity의 타입.

### `app.setAppUserModelId(id)` _Windows_

* `id` String

[애플리케이션의 사용자 모델 ID][app-user-model-id]를 `id`로 변경합니다.

### `app.importCertificate(options, callback)` _LINUX_

* `options` Object
  * `certificate` String - pkcs12 파일의 위치.
  * `password` String -  인증서의 암호.
* `callback` Function
  * `result` Integer - 가져오기의 결과.

pkcs12 형식으로된 인증서를 플랫폼 인증서 저장소로 가져옵니다. `callback`은 가져오기의
결과를 포함하는 `result` 객체를 포함하여 호출됩니다. 값이 `0` 일 경우 성공을 의미하며
다른 값은 모두 Chrominum의 [net_error_list](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h)에
따라 실패를 의미합니다.

### `app.disableHardwareAcceleration()`

현재 애플리케이션의 하드웨어 가속을 비활성화합니다.

이 메서드는 `app`의 `ready` 이벤트가 발생하기 전에만 호출할 수 있습니다.

### `app.setBadgeCount(count)` _Linux_ _macOS_

* `count` Integer

Returns `Boolean` - 호출 성공 여부.

현재 앱에 대해 카운터 뱃지를 설정합니다. count 를 `0`으로 설정하면 뱃지를
숨깁니다.

macOS에선 독 아이콘에 표시됩니다. Linux에선 Unity 런처에서만 작동합니다.

**참고:** Unity 런처는 이 기능을 작동하기 위해 `.desktop` 파일을 필요로 합니다.
이에 대한 자세한 내용은 [데스크톱 환경 통합][unity-requiremnt]을 참고하세요.

### `app.getBadgeCount()` _Linux_ _macOS_

Returns `Integer` - 현재 카운터 뱃지에 표시중인 값.

### `app.isUnityRunning()` _Linux_

Returns `Boolean` - 현재 데스크톱 환경이 Unity 인지 여부.

### `app.getLoginItemSettings()` _macOS_ _Windows_

Returns `Object`:

* `openAtLogin` Boolean - 앱이 로그인시 열리도록 설정되어있는 경우 `true`를 반환.
* `openAsHidden` Boolean - 앱이 로구인시 숨겨진 채로 열리도록 설정되어있는 경우
  `true`를 반환. 이 설정은 macOS에서만 지원됩니다.
* `wasOpenedAtLogin` Boolean - 자동으로 로그인할 때 애플리케이션이 열려있었는지
  여부. 이 설정은 macOS에서만 지원됩니다.
* `wasOpenedAsHidden` Boolean - 앱이 숨겨진 로그인 항목처럼 열려있었는지 여부.
  이는 앱이 시작시 어떤 윈도우도 열지 않을 것을 표시합니다. 이 설정은 macOS에서만
  지원됩니다.
* `restoreState` Boolean - 앱이 이전 세션에서 상태를 복원하여 로그인 항목처럼
  열려있었는지 여부. 이는 앱이 마지막으로 종료되었던 때에 열려있었던 윈도우를
  복원하는 것을 표시합니다. 이 설정은 macOS에서만 지원됩니다.

**참고:** 이 API 는 [MAS 빌드][mas-builds]에 영향을 주지 않습니다.

### `app.setLoginItemSettings(settings)` _macOS_ _Windows_

* `settings` Object
  * `openAtLogin` Boolean - `true`로 지정하면 로그인시 애플리케이션을 열도록 하며
    `false`로 지정시 로그인 항목에서 삭제합니다.
  * `openAsHidden` Boolean - `true`로 지정하면 애플리케이션을 숨겨진 채로 열도록
    합니다. 기본값은 `false`입니다. 사용자가 시스템 설정에서 이 설정을 변경할 수
    있으며 앱이 열렸을 때 현재 값을 확인하려면
    `app.getLoginItemStatus().wasOpenedAsHidden`을 확인해야 합니다. 이 설정은
    macOS에서만 지원됩니다.

앱의 로그인 항목 설정을 지정합니다.

**참고:** 이 API 는 [MAS 빌드][mas-builds]에 영향을 주지 않습니다.

### `app.isAccessibilitySupportEnabled()` _macOS_ _Windows_

Returns `Boolean` - Chrome의 접근성 지원이 활성화되어있으면 `true`를 그렇지
않다면 `false`를 반환합니다. 이 API는 사용할 수 있는 스크린 리더와 같은 접근성
기술이 감지되었을 때 `true`를 반환합니다. 자세한 내용은
https://www.chromium.org/developers/design-documents/accessibility 를 참고하세요.

### `app.setAboutPanelOptions(options)` _macOS_

* `options` Object
  * `applicationName` String (optional) - 앱 이름.
  * `applicationVersion` String (optional) - 앱 버전.
  * `copyright` String (optional) - 저작권 정보.
  * `credits` String (optional) - 크레딧 정보.
  * `version` String (optional) - 앱 빌드 버전 숫자.

정보 패널의 옵션을 설정합니다. 앱의 `.plist` 에 정의된 값보다 우선합니다. 자세한
내용은 [애플 문서][about-panel-options]를 참조하세요.

### `app.commandLine.appendSwitch(switch[, value])`

* `switch` String -명령줄 스위치
* `value` String (optional) - 주어진 스위치에 대한 값

Chrominum의 명령줄에 스위치를 추가합니다. `value`는 추가적인 값을 뜻하며 옵션입니다.

**참고:** 이 메서드는 `process.argv`에 영향을 주지 않습니다. 개발자들은 보통
Chrominum의 로우 레벨 수준의 동작을 제어하기 위해 주로 사용합니다.

### `app.commandLine.appendArgument(value)`

* `value` String - 명령줄에 덧붙여질 인수

Chrominum의 명령줄에 인수를 추가합니다. 인수는 올바르게 인용됩니다.

**참고:** 이 메서드는 `process.argv`에 영향을 주지 않습니다.

### `app.dock.bounce([type])` _macOS_

* `type` String (optional) - `critical` 또는 `informational`을 지정할 수 있습니다.
  기본값은 `informational` 입니다.

`critical`이 전달되면 dock 아이콘이 애플리케이션이 활성화되거나 요청이 중지되기
전까지 통통 튀는 바운스 효과를 적용합니다.

`informational`이 전달되면 dock 아이콘이 1초만 통통 튑니다. 하지만 애플리케이션이
활성화되거나 요청이 중지되기 전까지 요청은 계속 활성화로 유지 됩니다.

또한 요청을 취소할 때 사용할 수 있는 ID를 반환합니다.

### `app.dock.cancelBounce(id)` _macOS_

* `id` Integer

`app.dock.bounce([type])` 메서드에서 반환한 `id`의 바운스 효과를 취소합니다.

### `app.dock.downloadFinished(filePath)` _macOS_

* `filePath` String

`filePath`가 다운로드 폴더에 들어있다면 다운로드 스택을 바운스합니다.

### `app.dock.setBadge(text)` _macOS_

* `text` String

dock의 badge에 표시할 문자열을 설정합니다.

### `app.dock.getBadge()` _macOS_

Returns `String` - dock의 badge에 설정된 문자열.

### `app.dock.hide()` _macOS_

dock 아이콘을 숨깁니다.

### `app.dock.show()` _macOS_

dock 아이콘을 표시합니다.

### `app.dock.isVisible()` _macOS_

Returns `Boolean` - dock 아이콘이 보이는 상태인지 여부. `app.dock.show()` 호출은
비동기이므로 해당 메서드를 호출한 후 바로 이 메서드를 호출하면 `true`를 반환하지
않을 수 있습니다.

### `app.dock.setMenu(menu)` _macOS_

* `menu` [Menu](menu.md)

애플리케이션의 [dock menu][dock-menu]를 설정합니다.

### `app.dock.setIcon(image)` _macOS_

* `image` [NativeImage](native-image.md)

dock 아이콘의 `image`를 설정합니다.

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[CFBundleURLTypes]: https://developer.apple.com/library/ios/documentation/General/Reference/InfoPlistKeyReference/Articles/CoreFoundationKeys.html#//apple_ref/doc/uid/TP40009249-102207-TPXREF115
[LSCopyDefaultHandlerForURLScheme]: https://developer.apple.com/library/mac/documentation/Carbon/Reference/LaunchServicesReference/#//apple_ref/c/func/LSCopyDefaultHandlerForURLScheme
[handoff]: https://developer.apple.com/library/ios/documentation/UserExperience/Conceptual/Handoff/HandoffFundamentals/HandoffFundamentals.html
[activity-type]: https://developer.apple.com/library/ios/documentation/Foundation/Reference/NSUserActivity_Class/index.html#//apple_ref/occ/instp/NSUserActivity/activityType
[unity-requiremnt]: ../tutorial/desktop-environment-integration.md#unity-launcher-shortcuts-linux
[mas-builds]: docs/tutorial/mac-app-store-submission-guide.md
[JumpListBeginListMSDN]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378398(v=vs.85).aspx
[about-panel-options]: https://developer.apple.com/reference/appkit/nsapplication/1428479-orderfrontstandardaboutpanelwith?language=objc
