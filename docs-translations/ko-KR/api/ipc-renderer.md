# ipcRenderer

`ipcRenderer` 모듈은 [EventEmitter](https://nodejs.org/api/events.html) 클래스의
인스턴스입니다. 랜더러 프로세스에서 메인 프로세스로 동기/비동기 메시지를 주고 받는
방법을 제공합니다. 또한 메인 프로세스로부터 받은 메시지에 응답할 수도 있습니다.

[ipcMain](ipc-main.md)에서 코드 예제를 확인할 수 있습니다.

## 메시지 리스닝

`ipcRenderer`는 다음과 같은 이벤트 리스닝 메서드를 가지고 있습니다:

### `ipcRenderer.on(channel, callback)`

* `channel` String - 이벤트 이름
* `callback` Function

이벤트가 일어나면 `event` 객체와 임의의 인자와 함께 `callback` 함수가 호출됩니다.

### `ipcMain.removeListener(channel, callback)`

* `channel` String - 이벤트의 이름
* `callback` Function - `ipcMain.on(channel, callback)`에서 사용한 함수의 레퍼런스

메시지 수신을 완료한 후, 더 이상의 콜백이 필요하지 않을 때 또는 몇 가지 이유로 채널의
메시지 전송을 멈출수 없을 때, 이 함수를 통해 지정한 채널에 대한 콜백을 삭제할 수
있습니다.

### `ipcMain.removeAllListeners(channel)`

* `channel` String - 이벤트의 이름

이 ipc 채널에 등록된 *모든* 핸들러들을 삭제합니다.

### `ipcMain.once(channel, callback)`

`ipcMain.on()` 대신 이 함수를 사용할 경우 핸들러가 단 한 번만 호출됩니다.
`callback`이 한 번 호출된 이후 활성화되지 않습니다.

## 메시지 보내기

`ipcRenderer`는 다음과 같은 메시지 전송 메서드를 가지고 있습니다:

### `ipcRenderer.send(channel[, arg1][, arg2][, ...])`

* `channel` String - 이벤트 이름
* `arg` (optional)

`channel`을 통해 메인 프로세스에 비동기 메시지를 보냅니다. 그리고 필요에 따라 임의의
인자를 사용할 수도 있습니다. 메인 프로세스는 `ipcMain` 모듈의 `channel` 이벤트를 통해
이벤트를 리스닝 할 수 있습니다.

### `ipcRenderer.sendSync(channel[, arg1][, arg2][, ...])`

* `channel` String - 이벤트 이름
* `arg` (optional)

`channel`을 통해 메인 프로세스에 동기 메시지를 보냅니다. 그리고 필요에 따라 임의의
인자를 사용할 수도 있습니다. 메인 프로세스는 `ipc`를 통해 `channel` 이벤트를 리스닝
할 수 있습니다.

메인 프로세스에선 `ipcMain` 모듈의 `channel` 이벤트를 통해 받은
`event.returnValue`로 회신 할 수 있습니다.

__참고:__ 동기 메서드는 모든 랜더러 프로세스의 작업을 일시 중단시킵니다. 사용 목적이
확실하지 않다면 사용하지 않는 것이 좋습니다.

### `ipcRenderer.sendToHost(channel[, arg1][, arg2][, ...])`

* `channel` String - 이벤트 이름
* `arg` (optional)

`ipcRenderer.send`와 비슷하지만 이벤트를 메인 프로세스 대신 호스트 페이지내의
`<webview>` 요소로 보냅니다.
