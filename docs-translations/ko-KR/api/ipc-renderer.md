# ipcRenderer

> 렌더러 프로세스에서 메인 프로세스로 비동기 통신을 합니다.

`ipcRenderer` 모듈은 [EventEmitter](https://nodejs.org/api/events.html) 클래스의
인스턴스입니다. 렌더러 프로세스에서 메인 프로세스로 동기/비동기 메시지를 주고 받는
방법을 제공합니다. 또한 메인 프로세스로부터 받은 메시지에 응답할 수도 있습니다.

[ipcMain](ipc-main.md)에서 코드 예시를 확인할 수 있습니다.

## 메시지 리스닝

`ipcRenderer`는 다음과 같은 이벤트 리스닝 메서드를 가지고 있습니다:

### `ipcMain.on(channel, listener)`

* `channel` String
* `listener` Function

`channel`에 대해 이벤트를 리스닝합니다. 새로운 메시지가 도착하면 `listener`가
`listener(event, args...)` 형식으로 호출됩니다.

### `ipcMain.once(channel, listener)`

* `channel` String
* `listener` Function

이벤트에 대해 한 번만 작동하는 `listener` 함수를 등록합니다. 이 `listener`는 등록된
후 `channel`에 보내지는 메시지에 한해 호출됩니다. 호출된 이후엔 리스너가 삭제됩니다.

### `ipcMain.removeListener(channel, listener)`

* `channel` String
* `listener` Function

메시지 수신을 완료한 후, 더 이상의 콜백이 필요하지 않을 때 또는 몇 가지 이유로 채널의
메시지 전송을 멈출수 없을 때, 이 함수를 통해 지정한 채널에 대한 콜백을 삭제할 수
있습니다.

지정한 `channel`에 대한 리스너를 저장하는 배열에서 지정한 `listener`를 삭제합니다.

### `ipcMain.removeAllListeners(channel)`

* `channel` String (optional)

이 ipc 채널에 등록된 모든 핸들러들을 삭제하거나 지정한 `channel`을 삭제합니다.

## 메시지 보내기

`ipcRenderer`는 다음과 같은 메시지 전송 메서드를 가지고 있습니다:

### `ipcRenderer.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (optional)

`channel`을 통해 메인 프로세스에 비동기 메시지를 보냅니다. 그리고 필요에 따라 임의의
인수를 사용할 수도 있습니다. 인수들은 내부적으로 JSON 포맷으로 직렬화 되며, 이후 함수와
프로토타입 체인은 포함되지 않게 됩니다.

메인 프로세스는 `ipcMain` 모듈의 `channel` 이벤트를 통해
이벤트를 리스닝 할 수 있습니다.

### `ipcRenderer.sendSync(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (optional)

`channel`을 통해 메인 프로세스에 동기 메시지를 보냅니다. 그리고 필요에 따라 임의의
인자를 사용할 수도 있습니다. 인수들은 내부적으로 JSON 포맷으로 직렬화 되며, 이후 함수와
프로토타입 체인은 포함되지 않게 됩니다.

메인 프로세스는 `ipcMain` 모듈을 통해 `channel` 이벤트를 리스닝 할 수 있고,
`event.returnValue`로 회신 할 수 있습니다.

**참고:** 동기 메서드는 모든 렌더러 프로세스의 작업을 일시 중단시킵니다. 사용 목적이
확실하지 않다면 사용하지 않는 것이 좋습니다.

### `ipcRenderer.sendToHost(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (optional)

`ipcRenderer.send`와 비슷하지만 이벤트를 메인 프로세스 대신 호스트 페이지내의
`<webview>` 요소로 보냅니다.
