# powerMonitor

> 파워의 상태 변경을 모니터링합니다.

이 모듈은 메인 프로세스에서만 사용할 수 있습니다. `app` 모듈의 `ready` 이벤트가
발생한 이후에만 사용할 수 없습니다.

예시:

```javascript
var app = require('app');

app.on('ready', () => {
  require('electron').powerMonitor.on('suspend', () => {
    console.log('절전모드로 진입합니다!');
  });
});
```

## Events

`power-monitor` 모듈은 다음과 같은 이벤트를 가지고 있습니다:

## Event: `suspend`

시스템이 절전모드로 진입할 때 발생하는 이벤트입니다.

## Event: `resume`

시스템의 절전모드가 해제될 때 발생하는 이벤트입니다.

## Event: `on-ac` _Windows_

시스템이 AC 어뎁터 충전기를 사용하기 시작할 때 발생하는 이벤트입니다.

## Event: `on-battery` _Windows_

시스템이 배터리를 사용하기 시작할 때 발생하는 이벤트입니다.
