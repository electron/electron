# app

앱 모듈은 어플리케이션의 생명주기(?) 제어를 책임집니다.

밑의 예제는 마지막 윈도우가 종료되었을 때, 어떻게 어플리케이션을 종료하는지 설명합니다.

```javascript
var app = require('app');
app.on('window-all-closed', function() {
  app.quit();
});
```

## Events

앱 객체는 밑의 이벤트들을 발현(?)시킵니다.

### Event: 'will-finish-launching'

'will-finish-launching' 이벤트는 어플리케이션이 기본적인 시동준비(?)를 마치면 발현(?)됩니다.
 윈도우 운영체제와 리눅스 운영체제 안에서, 'will-finish-launching' 이벤트는 'ready' 이벤트와 동일합니다.
OS X 운영체제 안에서는, 이 이벤트는 'NSApplication'의 'applicationWillFinishLaunching' 알림으로 표현됩니다.
유저는 대개 'open-file'과 'open-url' 이벤트를 위한 listeners을 세팅합니다.
그리고, crash reporter와 auto updater를 시작합니다.

대부분의 경우, 유저는 모든 것을 'ready' 이벤트 handler로 해결합니다.

### Event: 'ready'

Electron이 초기화를 끝냈을 때, 이벤트가 발현합니다.

### Event: 'window-all-closed'

모든 윈도우창이 종료되었을 때, 이벤트가 발현합니다.

이 이벤트는 어플리케이션이 정상 종료되지 않았을 때만 발현됩니다.
만약 유저가 'Cmd + Q' 또는 개발자가 'app.quit()'를 호출했다면, Electron은 먼저 모든 윈도우창 종료를 시도합니다.
그 다음에, 'will-quit' 이벤트를 발현시킵니다.
그리고, 'will-quit'가 발현된 경우에는 'window-all-closed' 이벤트는 발현되지 않습니다.

### Event: 'before-quit'

Returns:

* `event` Event

어플리케이션이 어플리케이션의 윈도우 종료를 시작하기 전에, 발현됩니다.
'event.preventDefault()' 호출은 어플리케이션의 강제종료 행위를 방지합니다.

### Event: 'will-quit'

Returns:

* `event` Event

모든 윈도우창이 종료되고 어플리케이션이 종료하고자 할 때, 이벤트가 발현됩니다.
'event.preventDefault()'의 호출은 어플리케이션 종료를 방지합니다.
'will-quit' 이벤트와 'window-all-closed' 이벤트와의 차이점을 확인하려면,
'window-all-close' 이벤트의 설명을 참조합시다.

### Event: 'quit'

어플리케이션이 종료될 때, 발현됩니다.

### Event: 'open-file'

Returns:

* `event` Event
* `path` String

어플리케이션을 이용하여 파일을 열고자 할 때, 발현됩니다.

'open-file' 이벤트는 보통 어플리케이션이 열려 있을 때와,
파일을 열기 위해 OS가 어플리케이션을 재사용할 때 발현됩니다.
'open-file'은 파일이 dock에 추가될 때와 어플리케이션이 실행하기 전에도 발현됩니다.
위의 케이스를 처리하기 위해서는, 'ready' 이벤트가 발현되기도 전에, 어플리케이션 시작 초기 시
'open-file' 이벤트에 linsten을 걸어 놨는지 반드시 확인해야 합니다.

이 이벤트를 처리하기 위한다면, 유저는 'even.preventDefault()'를 호출해야 합니다.

### Event: 'open-url'

Returns:

* `event` Event
* `url` String

유저가 어플리케이션을 이용하여 URL을 열고자 할 경우, 발현됩니다.
어플리케이션을 통해 열기 위해서는 URL 구조가 반드시 등록되어 있어야 합니다.
이 이벤트를 처리하기 위해선, 유저가 'event.preventDefault()'을 호출해야 합니다.

### Event: 'activate' _OS X_

Returns:

* `event` Event
* `hasVisibleWindows` Bool

어플리케이션이 활성화 되었을 때 발현됩니다.
(어플리케이션은 dock 아이콘을 click했을 때 주로 활성화됩니다.)

### Event: 'browser-window-blur'

Returns:

* `event` Event
* `window` BrowserWindow

[browserWindow](browser-window.md)가 흐려졌을 때, 호출됩니다.

### Event: 'browser-window-focus'

Returns:

* `event` Event
* `window` BrowserWindow

[browserWindow](browser-window.md)을 포커싱했을 때, 호출됩니다.
(포커싱이란? 클릭 또는 활성화했을 때를 의미)

### Event: 'browser-window-created'

Returns:

* `event` Event
* `window` BrowserWindow

새로운 [browserWindow](browser-window.md)가 생성될 때 발현됩니다.

### Event: 'select-certificate'

유저인증이 요청되었을 때 발현됩니다.

Returns:

* `event` Event
* `webContents` [WebContents](browser-window.md#class-webcontents)
* `url` String
* `certificateList` [Objects]
  * `data` PEM encoded data
  * `issuerName` Issuer's Common Name
* `callback` Function

```javascript
app.on('select-certificate', function(event, host, url, list, callback) {
  event.preventDefault();
  callback(list[0]);
})
```

'url' 유저인증 요청의 탐색 항목에 대응합니다.
그리고, 'callback' 는 리스트로 필터링된 항목과 함께 호출될 필요가 있습니다.

'event.preventDefault()' 호출은 기록소로부터 처음인증된 정보를 사용하는
어플리케이션을 막습니다.

### Event: 'gpu-process-crashed'

GPU가 충돌을 일으켰을 때, 발현됩니다.

## Methods

'app' 객체는 밑의 함수를 포함하고 있습니다:

**Note:** 어떤 함수들은 표시된 특정한 운영체제에서만 사용가능합니다.

### `app.quit()`

모든 윈도우창 종료를 시도합니다. 'before-quit' 이벤트가 먼저 발현됩니다.
모든 윈도우창이 성공적으로 종료되었다면, 'will-quit' 이벤트가 발현하고,
디폴트설정으로 어플리케이션이 종료됩니다.

이 함수는 모든 'beforeunload'과 'unload' 이벤트 처리기가 제대로 실행되었음을 보장합니다.
(즉, 'beforeunload'와 'unload'가 정상 실행되었을 때, 실행가능합니다.)

'beforeunload' 이벤트 처리기가 'false'를 반환하였을 경우, 윈도우 창 종료가 취소 될 수 있습니다.


### `app.getAppPath()`

현재 어플리케이션의 디렉토리를 반환합니다.

### `app.getPath(name)`

* `name` String

특정한 디렉토리의 경로나 'name' 파일의 경로를 찾아 반환합니다.
실패할 경우, 'Error'를 반환합니다.

유저는 다음과 같은 이름으로 경로를 요쳥할 수 있습니다:

* `home` = 유저의 홈 디렉토리.
* `appData` = 유저의 어플리케이션 데이터 디렉토리, 디폴트설정으로:
  * `%APPDATA%` = on Windows
  * `$XDG_CONFIG_HOME` 또는 `~/.config` = on Linux
  * `~/Library/Application Support` = on OS X
* `userData` = 유저 app의 설정파일을 저장하는 디렉토리,
디폴트설정으로 'appData' 디렉토리에 유저 app의 이름을 추가한 형식.
* `temp` = 임시 디렉토리.
* `userDesktop` = 현재 로그인한 유저의 데스트탑 디렉토리.
* `exe` = 현재 실행가능한 파일.
* `module` = `libchromiumcontent` 라이브러리.

### `app.setPath(name, path)`

* `name` String
* `path` String

특정한 디렉토리나 파일이름이 'name'인 'path'를 재정의합니다.
만약 지정된 디렉토리의 경로가 존재하지 않는다면, 디렉토리가 새로 생성됩니다.
실패시, 'Error'를 반환합니다.

유저는 'app.getPath'에 정의되어 있는 'name' 경로만 재정의할 수 있습니다.

디폴트설정으로, 웹페이지의 쿠키와 캐시는 'userData' 디렉토리 밑에 저장됩니다.

만약 유저가 이 위치를 변경하고자 한다면, 'app' 모듈의 'ready' 이벤트가 발현되기 전,
유저는 반드시 'userData' 경로를 재정의해야 합니다.

### `app.getVersion()`

로드된 어플리케이션의 버전을 반환합니다.

만약 'package.json' 파일에서 어플리케이션의 버전을 찾지 못한다면,
현재 번들 또는 실행 파일의 버전이 반환됩니다.

### `app.getName()`

응용프로그램(=어플리케이션)의 'package.json' 파일에 있는
현재 응용프로그램의 이름을 반환합니다.

NPM 모듈 스펙에 따라, 대부분 'package.json'의 'name' 필드는 소문자 이름입니다.
유저는 'productName'의 필드를 지정해야 합니다.
'productName'는 응용프로그램의 대문자 이름입니다.
'productName'는 Electron에서 선호하는 'name' 입니다.

### `app.getLocale()`

현재 응용프로그램의 locale을 반환합니다.

### `app.resolveProxy(url, callback)`

* `url` URL
* `callback` Function

'url'을 위해 프록시 정보를 해석합니다.
'callback'은 'callback(proxy)' 요청이 수행될 때,
호출됩니다.

### `app.addRecentDocument(path)`

* `path` String

최근 문서 목록에 'path'를 추가합니다.

목록은 운영체제에 의해 관리됩니다.
Windows 운영체제에서는, 작업표시줄에서 참조할 수 있습니다.
OS X 운영체제에서는, dock 메뉴에서 참조할 수 있습니다.

### `app.clearRecentDocuments()`

최근 문서 목록을 모두 지웁니다.

### `app.setUserTasks(tasks)` _Windows_

* `tasks` Array - 'Task' 객체의 배열

Windows 운영체제에서 JumpList의 [Tasks][tasks] 카테고리로 'tasks'를 추가합니다.

'tasks'는 밑의 형식을 따르는 'Task' 객체의 배열입니다:

`Task` Object
* `program` String - 프로그램을 실행할 경로, 대부분 유저는 현재 프로그램을 여는 'process.execPath'를 지정합니다.
* `arguments` String - 'program'이 실행될 때의 명령문 인자.
* `title` String - JumpList에 표시할 문장.
* `description` String - 이 task 객체에 대한 설명.
* `iconPath` String - 아이콘을 포함한 임의의 리소스 파일이며, JumpList에 표시될 아이콘의 절대 경로.
유저는 대부분 프로그램의 아이콘을 표시하기 위해 'process.execPath'를 지정합니다.
* `iconIndex` Integer - 아이콘 파일안의 아이콘 색인(index). 만약, 아이콘 파일 안에 두 개 이상의 아이콘이 있을 경우,
 아이콘을 특정하기 위해 이 값을 설정합니다. 단, 하나의 아이콘만 있을 경우는 0 값을 가집니다.

### `app.commandLine.appendSwitch(switch[, value])`

Append a switch (with optional `value`) to Chromium's command line.
Chromium의 명령문에 (선택적으로, 'value'와 함께) switch를 추가합니다.

**Note:** 이 함수는 'process.argv'에 영향을 주지 않습니다.
그리고, 보통 이 함수는 개발자들이 하드웨어 수준의 Chrominum의 행동을 제어하기 위해 주로 사용됩니다.

### `app.commandLine.appendArgument(value)`

Chromium의 명령문에 인자를 추가합니다. 인자는 올바르게 인용될 것입니다.

**Note:** 'process.argv'에 영향을 주지는 않습니다.

### `app.dock.bounce([type])` _OS X_

* `type` String (optional) - 'critical'이나 'informational'. 디폴트 설정은 'informational'

'critical'이 input으로 전달되면, dock 아이콘은 응용프로그램이 활성화되거나, 요청이 취소될 때까지
계속 bounce합니다.

'informational'이 input으로 전달되면, dock 아이콘은 1초만 bounce합니다.
그러나, 응용프로그램이 활성화되거나 요청이 취소될 때까지 요청은 계속 활성화되어 있습니다.

요청을 대표하는 ID를 반환합니다.

### `app.dock.cancelBounce(id)` _OS X_

* `id` Integer

'id'의 반송을 취소합니다.


### `app.dock.setBadge(text)` _OS X_

* `text` String

dock의 badge 구역에 표현될 문장을 설정합니다.

### `app.dock.getBadge()` _OS X_

dock의 badge 라인을 반환합니다.


### `app.dock.hide()` _OS X_

dock 아이콘을 숨깁니다.

### `app.dock.show()` _OS X_

dock 아이콘을 보여줍니다.

### `app.dock.setMenu(menu)` _OS X_

* `menu` Menu

어플리케이션의 [dock menu][dock-menu]을 결정합니다.

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
