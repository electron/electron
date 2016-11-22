# ipcMain

> 메인 프로세스에서 렌더러 프로세스로 비동기 통신을 합니다.

프로세스: [메인](../tutorial/quick-start.md#main-process)

`ipcMain` 모듈은 [EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter)
클래스의 인스턴스입니다. 메인 프로세스에서 사용하면, 렌더러
프로세스(웹 페이지)에서 전달된 동기, 비동기 메시지를 주고 받는 방법을
제공합니다. 렌더러 프로세스에서 메시지를 전달하면 이 모듈을 통해 메시지를 받을
수 있습니다.

## 메시지 전송

물론 메시지를 받는 것 말고도 메인 프로세스에서 렌더러 프로세스로 보내는 것도
가능합니다. 자세한 내용은 [webContents.send][web-contents-send]를 참고하세요.

* 메시지를 전송할 때 이벤트 이름은 `channel`이 됩니다.
* 메시지에 동기로 응답할 땐 반드시 `event.returnValue`를 설정해야 합니다.
* 메시지를 비동기로 응답할 땐 `event.sender.send(...)` 메서드를 사용할 수 있습니다.

다음 예시는 렌더러 프로세스와 메인 프로세스간에 메시지를 전달하고 받는 예시입니다:

```javascript
// 메인 프로세스
const {ipcMain} = require('electron')
ipcMain.on('asynchronous-message', (event, arg) => {
  console.log(arg)  // "ping" 출력
  event.sender.send('asynchronous-reply', 'pong')
})

ipcMain.on('synchronous-message', (event, arg) => {
  console.log(arg)  // "ping" 출력
  event.returnValue = 'pong'
})
```

```javascript
// 렌더러 프로세스 (웹 페이지)
const {ipcRenderer} = require('electron')
console.log(ipc.sendSync('synchronous-message', 'ping')) // "pong" 출력

ipcRenderer.on('asynchronous-reply', (event, arg) => {
  console.log(arg) // "pong" 출력
})
ipcRenderer.send('asynchronous-message', 'ping')
```

## Methods

`ipcMain`은 다음과 같은 이벤트 리스닝 메서드를 가지고 있습니다:

### `ipcMain.on(channel, listener)`

* `channel` String
* `listener` Function

`channel`에 대해 이벤트를 리스닝합니다. 새로운 메시지가 도착하면 `listener`가
`listener(event, args...)` 형식으로 호출됩니다.

### `ipcMain.once(channel, listener)`

* `channel` String
* `listener` Function

이벤트에 대해 한 번만 작동하는 `listener` 함수를 등록합니다. 이 `listener`는
등록된 후 `channel`에 보내지는 메시지에 한해 호출됩니다. 호출된 이후엔 리스너가
삭제됩니다.

### `ipcMain.removeListener(channel, listener)`

* `channel` String
* `listener` Function

메시지 수신을 완료한 후, 더 이상의 콜백이 필요하지 않을 때 또는 몇 가지 이유로
채널의 메시지 전송을 멈출수 없을 때, 이 함수를 통해 지정한 채널에 대한 콜백을
삭제할 수 있습니다.

지정한 `channel`에 대한 리스너를 저장하는 배열에서 지정한 `listener`를 삭제합니다.

### `ipcMain.removeAllListeners(channel)`

* `channel` String (optional)

이 ipc 채널에 등록된 모든 핸들러들을 삭제하거나 지정한 `channel`을 삭제합니다.

## Event 객체

`callback`에서 전달된 `event` 객체는 다음과 같은 메서드와 속성을 가지고 있습니다:

### `event.returnValue`

이 메시지를 지정하면 동기 메시지를 전달합니다.

### `event.sender`

메시지를 보낸 `webContents` 객체를 반환합니다. `event.sender.send` 메서드를 통해
비동기로 메시지를 전달할 수 있습니다. 자세한 내용은
[webContents.send][web-contents-send]를 참고하세요.

[web-contents-send]: web-contents.md#webcontentssendchannel-arg1-arg2-
