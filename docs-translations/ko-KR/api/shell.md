# shell

> 파일과 URL을 각 기본 어플리케이션을 통해 관리합니다.

`shell` 모듈은 데스크톱 환경 통합에 관련한 유틸리티를 제공하는 모듈입니다.

다음 예시는 설정된 URL을 유저의 기본 브라우저로 엽니다:

```javascript
const { shell } = require('electron');

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
* `options` Object (optional) _OS X_
  * `activate` Boolean - `true`로 설정하면 어플리케이션을 바로 활성화 상태로
    실행합니다. 기본값은 `true`입니다.

제공된 외부 프로토콜 URL을 기반으로 데스크톱의 기본 프로그램으로 엽니다. (예를 들어
mailto: URL은 유저의 기본 이메일 에이전트로 URL을 엽니다.) 어플리케이션이 해당 URL을
열 수 있을 때 `true`를 반환합니다. 아니라면 `false`를 반환합니다.

**역주:** 탐색기로 폴더만 표시하려면 `'file://경로'`와 같이 지정하여 열 수 있습니다.

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

지정한 파일을 휴지통으로 이동합니다. 작업의 성공여부를 boolean 형으로 리턴합니다.

### `shell.beep()`

비프음을 재생합니다.
