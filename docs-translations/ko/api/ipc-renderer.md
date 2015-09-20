# ipc (renderer)

`ipc` (renderer) 모듈은 메인 프로세스로 동기 또는 비동기 메시지를 보내고 받는 방법을 제공합니다.
물론 메인 프로세스로부터 받은 메시지에 응답할 수도 있습니다.

**참고:** 만약 랜더러 프로세스에서 메인 프로세스의 모듈을 직접적 사용하고 싶다면 [remote](remote.md) 모듈을 사용하는 것을 고려해보는 것이 좋습니다.

[ipc (main process)](ipc-main-process.md)에서 예제를 확인 할 수 있습니다.

## Methods

`ipc` 모듈은 다음과 같은 메서드를 가지고 있습니다:

**참고:** 이 메소드들을 사용하여 `message`를 보낼 땐 반드시 메인 프로세스의
[`ipc (main process)`](ipc-main-process.md) 모듈에서도 이벤트 리스너를 등록해 두어야 합니다.

### `ipc.send(channel[, arg1][, arg2][, ...])`

* `channel` String - 이벤트 이름
* `arg` (optional)

`channel`을 통해 메인 프로세스에 비동기 메시지를 보냅니다.
옵션으로 `arg`에 한 개 또는 여러 개의 메시지를 포함할 수 있습니다. 모든 타입을 사용할 수 있습니다.
메인 프로세스는 `ipc`를 통해 `channel` 이벤트를 리스닝 할 수 있습니다.

### `ipc.sendSync(channel[, arg1][, arg2][, ...])`

* `channel` String - 이벤트 이름
* `arg` (optional)

`channel`을 통해 메인 프로세스에 동기 메시지를 보냅니다.
옵션으로 `arg`에 한 개 또는 여러 개의 메시지를 포함할 수 있습니다. 모든 타입을 사용할 수 있습니다.
메인 프로세스는 `ipc`를 통해 `channel` 이벤트를 리스닝 할 수 있습니다.

메인 프로세스에선 `ipc` 모듈의 `channel` 이벤트를 통해 받은 `event.returnValue`로 회신 할 수 있습니다.

**참고:** 동기 메시징은 모든 랜더러 프로세스의 작업을 일시 중단시킵니다. 이 메서드를 사용하는 것을 권장하지 않습니다.

### `ipc.sendToHost(channel[, arg1][, arg2][, ...])`

* `channel` String - 이벤트 이름
* `arg` (optional)

`ipc.send`와 비슷하지만 이벤트를 메인 프로세스 대신 호스트 페이지내의 `<webview>`로 보냅니다.
옵션으로 `arg`에 한 개 또는 여러 개의 메시지를 포함할 수 있습니다. 모든 타입을 사용할 수 있습니다.
