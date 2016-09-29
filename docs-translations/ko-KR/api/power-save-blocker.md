# powerSaveBlocker

> 시스템이 저전력 (슬립) 모드로 진입하는 것을 막습니다.

예시:

```javascript
const {powerSaveBlocker} = require('electron');

const id = powerSaveBlocker.start('prevent-display-sleep');
console.log(powerSaveBlocker.isStarted(id));

powerSaveBlocker.stop(id);
```

## Methods

`powerSaveBlocker` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `powerSaveBlocker.start(type)`

* `type` String - Power save blocker 종류
  * `prevent-app-suspension` - 저전력 모드 등으로 인한 애플리케이션 작동 중단을
    방지합니다. 시스템을 항시 활성화 상태로 만듭니다. 하지만 화면은 자동으로 꺼질 수
    있습니다. 사용 예시: 파일 다운로드, 음악 재생 등.
  * `prevent-display-sleep` - 슬립 모드 등으로 인한 애플리케이션의 작동 중단을
    방지합니다. 시스템을 항시 활성화 상태로 만들고 슬립 모드(화면 꺼짐)를 방지합니다.
    사용 예시: 비디오 재생 등.

시스템이 저전력 모드(슬립)로 진입하는 것을 막기 시작합니다. 정수로 된 식별 ID를
반환합니다.

**참고:** `prevent-display-sleep` 모드는 `prevent-app-suspension` 보다 우선 순위가
높습니다. 두 모드 중 가장 높은 우선 순위의 모드만 작동합니다. 다시 말해
`prevent-display-sleep` 모드는 언제나 `prevent-app-suspension` 모드의 효과를
덮어씌웁니다.

예를 들어 A-요청이 `prevent-app-suspension` 모드를 사용하고 B-요청이
`prevent-display-sleep`를 사용하는 API 호출이 있었다 하면 `prevent-display-sleep`
모드를 사용하는 B의 작동이 중단(stop)되기 전까지 작동하다 B가 중단되면
`prevent-app-suspension` 모드를 사용하는 A가 작동하기 시작합니다.

### `powerSaveBlocker.stop(id)`

* `id` Integer - `powerSaveBlocker.start`로부터 반환되는 power save blocker 식별
ID.

설정한 power save blocker를 중지합니다.

### `powerSaveBlocker.isStarted(id)`

* `id` Integer - `powerSaveBlocker.start`로부터 반환되는 power save blocker 식별
ID.

지정한 id의 `powerSaveBlocker`가 실행 중인지 확인합니다.
