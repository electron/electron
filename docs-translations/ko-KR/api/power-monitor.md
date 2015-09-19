# power-monitor

`power-monitor` 모듈은 PC의 파워 상태를 나타냅니다. (주로 노트북 등에서 사용됩니다)
이 모듈은 메인 프로세스에서만 사용할 수 있으며, (remote 모듈(RPC)을 사용해도 작동이 됩니다)
메인 프로세스의 `app` 모듈에서 `ready` 이벤트를 호출하기 전까지 사용할 수 없습니다.

예제:

```javascript
var app = require('app');

app.on('ready', function() {
  require('power-monitor').on('suspend', function() {
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

## Event: `on-ac`

시스템이 AC 어뎁터 충전기를 사용하기 시작할 때 발생하는 이벤트입니다.

## Event: `on-battery`

시스템이 배터리를 사용하기 시작할 때 발생하는 이벤트입니다.
