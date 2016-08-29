# shell

> 파일과 URL을 각 기본 애플리케이션을 통해 관리합니다.

`shell` 모듈은 데스크톱 환경 통합에 관련한 유틸리티를 제공하는 모듈입니다.

다음 예시는 설정된 URL을 유저의 기본 브라우저로 엽니다:

```javascript
const {shell} = require('electron');

shell.openExternal('https://github.com');
```

## Methods

`shell` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

지정한 파일을 탐색기에서 보여줍니다. 가능한 경우 탐색기 내에서 파일을 선택합니다.

### `shell.openItem(fullPath)`

* `fullPath` String

지정한 파일을 데스크톱 기본 프로그램으로 엽니다.

### `shell.openExternal(url[, options])`

* `url` String
* `options` Object (optional) _macOS_
  * `activate` Boolean - `true`로 설정하면 애플리케이션을 바로 활성화 상태로
    실행합니다. 기본값은 `true`입니다.

제공된 외부 프로토콜 URL을 기반으로 데스크톱의 기본 프로그램으로 엽니다. (예를 들어
mailto: URL은 유저의 기본 이메일 에이전트로 URL을 엽니다.) 애플리케이션이 해당 URL을
열 수 있을 때 `true`를 반환합니다. 아니라면 `false`를 반환합니다.

**역자주:** 탐색기로 폴더만 표시하려면 `'file://경로'`와 같이 지정하여 열 수 있습니다.

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

지정한 파일을 휴지통으로 이동합니다. 작업의 성공여부를 boolean 형으로 리턴합니다.

### `shell.beep()`

비프음을 재생합니다.

### `shell.writeShortcutLink(shortcutPath[, operation], options)` _Windows_

* `shortcutPath` String
* `operation` String (optional) - 기본값은 `create`이며 다음 값 중 한 가지가 될 수
  있습니다:
  * `create` - 새 바로가기를 생성하고 필요하다면 덮어씁니다.
  * `update` - 이미 존재하는 바로가기의 특정한 속성을 갱신합니다.
  * `replace` - 이미 존재하는 바로가기를 덮어씁니다. 바로가기가 존재하지 않으면
    실패합니다.
* `options` Object
  * `target` String - 이 바로가기로부터 실행될 대상입니다.
  * `cwd` String (optional) - 작업 디렉토리입니다. 기본값은 없습니다.
  * `args` String (optional) - 이 바로가기로부터 실행될 때 `target`에 적용될 인수
    값입니다. 기본값은 없습니다.
  * `description` String (optional) - 바로가기의 설명입니다. 기본값은 없습니다.
  * `icon` String (optional) - 아이콘의 경로입니다. DLL 또는 EXE가 될 수 있습니다.
    `icon`과 `iconIndex`는 항상 같이 설정되어야 합니다. 기본값은 없으며 `target`의
    아이콘을 사용합니다.
  * `iconIndex` Integer (optional) - `icon`이 DLL 또는 EXE일 때 사용되는 아이콘의
    리소스 ID이며 기본값은 0입니다.
  * `appUserModelId` String (optional) - 애플리케이션 사용자 모델 ID입니다.
    기본값은 없습니다.

`shortcutPath`에 바로가기 링크를 생성합니다. 바로가기 생성에 성공하면 `true`를
반환하고 그렇지 않으면 `false`를 반환합니다.

### `shell.readShortcutLink(shortcutPath)` _Windows_

* `shortcutPath` String

`shortcutPath`에 위치한 바로가기 링크를 해석합니다. `shell.writeShortcutLink`
메서드의 `options`에 묘사된 속성을 포함하는 객체를 반환합니다.

오류가 발생하면 예외가 throw됩니다.
