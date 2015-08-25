# ipc (renderer)

`ipc` 모듈은 메인 프로세스로 메시지를 동기 또는 비동기로 보내고 받을 수 있는 몇 가지 방법을 제공합니다.
만약 랜더러 프로세스에서 메인 프로세스의 모듈을 직접적으로 사용하고 싶다면 [remote](remote-ko.md) 모듈을 사용하는 것을 고려해보는 것이 좋습니다.

[ipc (main process)](ipc-main-process-ko.md)에서 예제를 볼 수 있습니다.

## ipc.send(channel[, args...])

지정한 `channel`을 통해 `args..`를 비동기로 메시지를 보냅니다. 메인 프로세스는 `ipc` 모듈의 `channel` 이벤트를 통해 메시지를 받을 수 있습니다.

## ipc.sendSync(channel[, args...])

지정한 `channel`을 통해 `args..`를 동기로 메시지를 보냅니다. 그리고 메인 프로세스에서 보낸 결과를 반환합니다.
메인 프로세스는 `ipc` 모듈의 `channel` 이벤트를 통해 메시지를 받을 수 있습니다. 그리고 `event.returnValue`를 통해 반환값을 설정할 수 있습니다.

역자 주: `channel`은 이벤트 이름입니다.

**알림:** 보통 개발자들은 해당 API를 사용하려 하지 않습니다. 동기 ipc 작업은 랜더러 프로세스의 모든 작업을 중단시킵니다.

## ipc.sendToHost(channel[, args...])

`ipc.send`와 비슷하지만 메시지를 메인 프로세스 대신 호스트 페이지로 보냅니다.

이 메소드는 보통 `<webview>`와 호스트 페이지 간의 통신에 사용됩니다.
