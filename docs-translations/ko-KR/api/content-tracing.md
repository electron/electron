# contentTracing

> 성능상의 병목 현상과 느린 작업을 찾기 위해 Chromium의 콘텐츠 모듈에서 추적 데이터를
수집합니다.

이 모듈은 웹 인터페이스를 포함하고 있지 않으며 Chrome 브라우저에서
`chrome://tracing/` 페이지를 열고 생성된 파일을 로드하면 결과를 볼 수 있습니다.

```javascript
const {contentTracing} = require('electron')

const options = {
  categoryFilter: '*',
  traceOptions: 'record-until-full,enable-sampling'
}

contentTracing.startRecording(options, () => {
  console.log('Tracing started')

  setTimeout(() => {
    contentTracing.stopRecording('', (path) => {
      console.log('Tracing data recorded to ' + path)
    })
  }, 5000)
})
```

## Methods

`content-tracing` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `contentTracing.getCategories(callback)`

* `callback` Function

카테고리 그룹 세트를 가져옵니다. 카테고리 그룹은 도달된 코드 경로를 변경할 수 있습니다.

모든 child 프로세스가 `getCategories` 요청을 승인하면 `callback`이 한 번 호출되며
인수에 카테고리 그룹의 배열이 전달됩니다.

### `contentTracing.startRecording(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

모든 프로세스에서 레코딩을 시작합니다.

레코딩은 지역적으로 즉시 실행됩니다. 그리고 비동기로 child 프로세스는 곧
EnableRecording 요청을 받게 됩니다. 모든 child 프로세스가 `startRecording` 요청을
승인하면 `callback`이 한 번 호출됩니다.

`categoryFilter`는 어떤 카테고리 그룹이 트레이싱 되어야 하는지 필터링할 수 있습니다.
필터는 `-` 접두사를 통해 특정 카테고리 그룹을 제외할 수 있습니다. 카테고리 패턴은 같은
리스트 내에서 포함과 제외를 함께 사용할 수 없습니다.

예시:

* `test_MyTest*`,
* `test_MyTest*,test_OtherStuff`,
* `"-excluded_category1,-excluded_category2`

`traceOptions`은 어떤 종류의 트레이싱을 사용할 수 있는지 지정하고 콤마로 리스트를
구분합니다.

사용할 수 있는 옵션은 다음과 같습니다:

* `record-until-full`
* `record-continuously`
* `trace-to-console`
* `enable-sampling`
* `enable-systrace`

첫번째부터 3번째까지의 옵션은 추적 레코딩 모드입니다. 이에 따라 상호 배타적입니다.
만약 레코딩 모드가 한 개 이상 지정되면 마지막 지정한 모드만 사용됩니다. 어떤 모드도
설정되지 않았다면 `record-until-full` 모드가 기본으로 사용됩니다.

추적 옵션은 `traceOptions`이 파싱되어 적용되기 전까지 다음과 같은 기본값이 사용됩니다.

`record-until-full`이 기본 모드, `enable-sampling`과 `enable-systrace`옵션은
포함되지 않음

## `contentTracing.stopRecording(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

모든 프로세스에서 레코딩을 중지합니다.

Child 프로세스는 일반적으로 추적 데이터와 희귀한 플러시 그리고 추적 데이터를 메인
프로세스로 보내는 작업에 대해 캐싱 합니다. 이러한 일을 하는 이유는 IPC를 통해 추적
데이터를 보내는 작업은 매우 비싼 연산을 동반하기 때문입니다. 우리는 추적에 의한 런타임
오버헤드를 피하고자 합니다. 그래서 추적이 끝나면 모든 child 프로세스에 보류된 추적
데이터를 플러시 할 것인지 물어봅니다.

모든 child 프로세스가 `stopRecording` 요청을 승인하면 `callback`에 추적 데이터
파일을 포함하여 한 번 호출됩니다.

추적 데이터는 `resultFilePath` 해당 경로가 비어있는 경우에 한 해 해당 경로에
작성되거나 임시 파일에 작성됩니다. 실제 파일 경로는 null이 아닌 이상 `callback`을
통해 전달됩니다.

### `contentTracing.startMonitoring(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

모든 프로세스에서 모니터링을 시작합니다.

모니터링은 지역적으로 즉시 시작됩니다. 그리고 이내 자식 프로세스들이
`startMonitoring` 비동기 요청을 받습니다.

모든 자식 프로세스가 `startMonitoring` 요청을 승인하면 `callback`이 한 번 호출됩니다.

### `contentTracing.stopMonitoring(callback)`

* `callback` Function

모든 프로세스에서 모니터링을 중단합니다.

모든 자식 프로세스가 `stopMonitoring` 요청을 승인하면 `callback`이 한 번 호출됩니다.

### `contentTracing.captureMonitoringSnapshot(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

현재 모니터링 추적 데이터를 가져옵니다.

자식 프로세스들은 일반적으로 추적 데이터를 캐싱하며 드물게 플러시 하거나 메인
프로세스로 추적 데이터를 보냅니다. 왜냐하면 IPC를 통해 추적 데이터를 보내는데에는
많은 자원을 소비하기 때문입니다. 그리고 우리는 추적시 발생하는 불필요한 런타임
오버헤드를 피하고자 합니다. 그래서 추적이 끝나면 반드시 비동기로 자식 프로세스들의
보류된 추적 데이터를 플러시 할 것인지 물어봅니다.

모든 자식 프로세스가 `captureMonitoringSnapshot` 요청을 승인하면 추적 데이터 파일을
포함하는 `callback`이 한 번 호출됩니다.

### `contentTracing.getTraceBufferUsage(callback)`

* `callback` Function

추적 버퍼 % 전체 상태의 프로세스간 최대치를 가져옵니다. TraceBufferUsage 값이
결정되면 `callback`이 한 번 호출됩니다.

### `contentTracing.setWatchEvent(categoryName, eventName, callback)`

* `categoryName` String
* `eventName` String
* `callback` Function

`callback`은 지정된 이벤트가 어떤 작업을 발생시킬 때마다 호출됩니다.

### `contentTracing.cancelWatchEvent()`

Watch 이벤트를 중단합니다. 만약 추적이 활성화되어 있다면 이 메서드는 watch 이벤트
콜백과 race가 일어날 것입니다.
