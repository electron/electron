# shell

> 파일과 URL을 각 기본 애플리케이션을 통해 관리합니다.

`shell` 모듈은 데스크톱 환경 통합에 관련한 유틸리티를 제공하는 모듈입니다.

다음 예시는 설정된 URL을 유저의 기본 브라우저로 엽니다:

```javascript
const {shell} = require('electron')

shell.openExternal('https://github.com')
```

## Methods

`shell` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

Returns `Boolean` - 아이템 성공적으로 보여졌는지 여부.

지정한 파일을 파일 매니저에서 보여줍니다. 가능한 경우 파일을 선택합니다.

### `shell.openItem(fullPath)`

* `fullPath` String

Returns `Boolean` - 아이템 성공적으로 열렸는지 여부.

지정한 파일을 데스크톱 기본 프로그램으로 엽니다.

### `shell.openExternal(url[, options])`

* `url` String
* `options` Object (optional) _macOS_
  * `activate` Boolean - `true`로 설정하면 애플리케이션을 바로 활성화 상태로
    실행합니다. 기본값은 `true`입니다.

Returns `Boolean` - 애플리케이션이 URL 을 열 수 있었는지 여부.

제공된 외부 프로토콜 URL을 기반으로 데스크톱의 기본 프로그램으로 엽니다. (예를 들어
mailto: URL은 유저의 기본 이메일 에이전트로 URL을 엽니다).

**역자주:** 탐색기로 폴더만 표시하려면 `'file://경로'`와 같이 지정하여 열 수 있습니다.

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Returns `Boolean` - 아이템이 성공적으로 휴지통으로 이동되었는지 여부.

지정한 파일을 휴지통으로 이동시키고 작업의 상태를 boolean 형으로 반환합니다.

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
* `options` [ShortcutDetails](structures/shortcut-details.md)

Returns `Boolean` - 바로가기 생성 여부.

`shortcutPath`에 바로가기 링크를 생성하거나 갱신합니다.

### `shell.readShortcutLink(shortcutPath)` _Windows_

* `shortcutPath` String

Returns [`ShortcutDetails`](structures/shortcut-details.md)

Resolves the shortcut link at `shortcutPath`.
`shortcutPath`에 위치한 바로가기 링크를 해석합니다. `shell.writeShortcutLink`
메서드의 `options`에 묘사된 속성을 포함하는 객체를 반환합니다.

오류가 발생하면 예외가 throw됩니다.
