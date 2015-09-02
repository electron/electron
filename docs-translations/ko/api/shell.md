# shell

`shell` 모듈은 데스크톱 환경 통합에 관련한 유틸리티를 제공하는 모듈입니다.

다음 예제는 설정된 URL을 유저의 기본 브라우저로 엽니다:

```javascript
var shell = require('shell');
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

### `shell.openExternal(url)`

* `url` String

제공된 외부 프로토콜 URL을 기반으로 데스크톱의 기본 프로그램으로 엽니다. (예를 들어 mailto: URL은 유저의 기본 이메일 에이전트로 URL을 엽니다.)

역주: 폴더는 'file:\\\\C:\\'와 같이 지정하여 열 수 있습니다. (`\\`로 경로를 표현한 이유는 Escape 문자열을 참고하세요.)

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Move the given file to trash and returns boolean status for the operation.

지정한 파일을 휴지통으로 이동합니다. 작업의 성공여부를 boolean 형으로 리턴합니다.

### `shell.beep()`

비프음을 재생합니다.
