# dialog

> 파일을 열거나 저장하고, 알림을 표시하기 위한 네이티브 시스템 대화 상자를 표시합니다.

다음 예시는 파일과 디렉터리를 다중으로 선택하는 대화 상자를 표시하는 예시입니다:

```javascript
let win = ...;  // 대화 상자를 사용할 BrowserWindow 객체
const {dialog} = require('electron');
console.log(dialog.showOpenDialog({properties: ['openFile', 'openDirectory', 'multiSelections']}));
```

대화 상자는 Electron의 메인 스레드에서 열립니다. 만약 렌더러 프로세스에서 대화 상자
객체를 사용하고 싶다면, `remote`를 통해 접근하는 방법을 고려해야 합니다:

```javascript
const {dialog} = require('electron').remote;
```

## Methods

`dialog` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `dialog.showOpenDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (optional)
* `options` Object
  * `title` String
  * `defaultPath` String
  * `buttonLabel` String - 확인 버튼을 위한 커스텀 라벨이며, 빈칸으로 둘 경우 기본
    라벨이 사용됩니다.
  * `filters` Array
  * `properties` Array - 대화 상자가 사용할 기능(모드)이 담긴 배열입니다.
    다음을 포함할 수 있습니다: `openFile`, `openDirectory`, `multiSelections`,
    `createDirectory`, `showHiddenFiles`.
* `callback` Function (optional)

사용할 대화 상자의 기능이 담긴 배열입니다. 다음을 포함할 수 있습니다: `openFile`,
`openDirectory`, `multiSelections`, `createDirectory`

작업에 성공하면 콜백으로 유저가 선택한 파일의 경로를 포함한 배열을 반환합니다. 그 외의
경우엔 `undefined`를 반환합니다.

`filters`를 지정하면 유저가 선택 가능한 파일 형식을 지정할 수 있습니다. 유저가 선택할
수 있는 타입에 제한을 두려면 다음과 같이 할 수 있습니다:

```javascript
{
  filters: [
    {name: 'Images', extensions: ['jpg', 'png', 'gif']},
    {name: 'Movies', extensions: ['mkv', 'avi', 'mp4']},
    {name: 'Custom File Type', extensions: ['as']},
    {name: 'All Files', extensions: ['*']}
  ]
}
```

`extensions` 배열은 반드시 와일드카드와 마침표를 제외한 파일 확장자를 포함시켜야
합니다. (예를 들어 `'png'`는 가능하지만 `'.png'`와 `'*.png'`는 안됩니다) 모든 파일을
보여주려면 `'*'`와 같은 와일드카드를 사용하면 됩니다. (다른 와일드카드는 지원하지
  않습니다)

`callback`이 전달되면 메서드가 비동기로 작동되며 결과는 `callback(filenames)`을
통해 전달됩니다.

**참고:** Windows와 Linux에선 파일 선택 모드, 디렉터리 선택 모드를 동시에 사용할 수
없습니다. 이러한 이유로 `properties`를 `['openFile', 'openDirectory']`로 설정하면
디렉터리 선택 대화 상자가 표시됩니다.

### `dialog.showSaveDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (optional)
* `options` Object
  * `title` String
  * `defaultPath` String
  * `buttonLabel` String - 확인 버튼을 위한 커스텀 라벨이며, 빈칸으로 둘 경우 기본
    라벨이 사용됩니다.
  * `filters` Array
* `callback` Function (optional)

작업에 성공하면 콜백으로 유저가 선택한 파일의 경로를 포함한 배열을 반환합니다. 그 외엔
`undefined`를 반환합니다.

`filters`를 지정하면 유저가 저장 가능한 파일 형식을 지정할 수 있습니다. 사용 방법은
`dialog.showOpenDialog`의 `filters` 속성과 같습니다.

`callback`이 전달되면 메서드가 비동기로 작동되며 결과는 `callback(filename)`을 통해
전달됩니다.

### `dialog.showMessageBox([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (optional)
* `options` Object
  * `type` String - `"none"`, `"info"`, `"error"`, `"question"`, `"warning"` 중
    하나를 사용할 수 있습니다. Windows에선 따로 `icon`을 설정하지 않은 이상
    "question"과 "info"는 같은 아이콘으로 표시됩니다.
  * `buttons` Array - 버튼들의 라벨을 포함한 배열입니다. Windows에서 빈 배열로 둘
    경우, "OK" 버튼 하나가 포함됩니다.
  * `defaultId` Integer - 메시지 박스가 열렸을 때 기본적으로 선택될 버튼 배열의
    버튼 인덱스입니다.
  * `title` String - 대화 상자의 제목입니다. 몇몇 플랫폼에선 보이지 않을 수 있습니다.
  * `message` String - 대화 상자의 본문 내용입니다.
  * `detail` String - 메시지의 추가 정보입니다.
  * `icon` [NativeImage](native-image.md)
  * `cancelId` Integer - 유저가 대화 상자의 버튼을 클릭하지 않고 대화 상자를 취소했을
    때 반환되는 버튼의 인덱스입니다. 기본적으로 버튼 리스트가 "cancel" 또는 "no"
    라벨을 가지고 있을 때 해당 버튼의 인덱스를 반환합니다. 따로 두 라벨이 지정되지
    않은 경우 0을 반환합니다. macOS와 Windows에선 `cancelId` 지정 여부에 상관없이
    "Cancel" 버튼이 언제나 `cancelId`로 지정됩니다.
  * `noLink` Boolean - Windows에서 Electron은 ("Cancel"이나 "Yes"와 같은) 흔히
    사용되는 버튼을 찾으려고 시도하고 대화 상자 내에서 해당 버튼을 커맨드 링크처럼
    만듭니다. 이 기능으로 앱을 좀 더 현대적인 Windows 앱처럼 만들 수 있습니다. 이
    기능을 원하지 않으면 `noLink`를 true로 지정하면 됩니다.
* `callback` Function (optional)

대화 상자를 표시합니다. `browserWindow`를 지정하면 대화 상자가 완전히 닫힐 때까지
지정한 창을 사용할 수 없습니다. 완료 시 유저가 선택한 버튼의 인덱스를 반환합니다.

**역자주:** 부정을 표현하는 "아니오", "취소"와 같은 한글 단어는 지원되지 않습니다. 만약
macOS 또는 Windows에서 "확인", "취소"와 같은 순서로 버튼을 지정하게 될 때 Alt + f4로
해당 대화 상자를 끄게 되면 "확인"을 누른 것으로 판단되어 버립니다. 이를 해결하려면
"Cancel"을 대신 사용하거나 BrowserWindow API를 사용하여 대화 상자를 직접 구현해야
합니다.

`callback`이 전달되면 메서드가 비동기로 작동되며 결과는 `callback(response)`을 통해
전달됩니다.

### `dialog.showErrorBox(title, content)`

에러 메시지를 보여주는 대화 상자를 표시합니다.

이 함수는 `app` 모듈의 `ready` 이벤트가 발생하기 전까지 사용할 수 있습니다. 이 메서드는
보통 애플리케이션이 시작되기 전에 특정한 에러를 표시하기 위해 사용됩니다. 만약
Linux에서 `ready` 이벤트가 발생하기 전에 이 API를 호출할 경우, 메시지는 stderr를
통해서 표시되며 GUI 대화 상자는 표시되지 않습니다.

## Sheets

macOS에선, `browserWindow` 인수에 `BrowserWindow` 객체 참조를 전달하면 대화
상자가 해당 윈도우에 시트처럼 표시되도록 표현할 수 있습니다. 윈도우의 객체 참조가
제공되지 않으면 모달 형태로 표시됩니다.

`BrowserWindow.getCurrentWindow().setSheetOffset(offset)`을 통해 윈도우에 부착될
시트의 위치를 조정할 수 있습니다.
