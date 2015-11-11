# ipcMain

`ipcMain` 모듈은 메인 프로세스에서 사용할 때 랜더러 프로세스(웹 페이지)에서 전달된 동기/비동기 메시지를 주고 받는 방법을 제공합니다.
랜더러 프로세스에서 메시지를 전달하면 이 모듈을 통해 메시지를 받을 수 있습니다.

## 메시지 전송

물론 메시지를 받는 것 말고도 메인 프로세스에서 랜더러 프로세스로 보내는 것도 가능합니다.
자세한 내용은 [webContents.send](web-contents.md#webcontentssendchannel-args)를 참고하세요.

* 메시지를 전송할 때 이벤트 이름은 `channel`이 됩니다.
* 메시지에 동기로 응답할 땐 반드시 `event.returnValue`를 설정해야 합니다.
* 메시지를 비동기로 응답할 땐 `event.sender.send(...)` 메서드를 사용할 수 있습니다.

다음 예제는 랜더러 프로세스와 메인 프로세스간에 메시지를 전달하고 받는 예제입니다:

```javascript
// 메인 프로세스
var ipc = require('ipc');
ipc.on('asynchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.sender.send('asynchronous-reply', 'pong');
});

ipc.on('synchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.returnValue = 'pong';
});
```

```javascript
// 랜더러 프로세스 (웹 페이지)
var ipc = require('ipc');
console.log(ipc.sendSync('synchronous-message', 'ping')); // prints "pong"

ipc.on('asynchronous-reply', function(arg) {
  console.log(arg); // prints "pong"
});
ipc.send('asynchronous-message', 'ping');
```

## 메시지 리스닝

`ipcMain`은 다음과 같은 이벤트 리스닝 메서드를 가지고 있습니다:

### `ipc.on(channel, callback)`

* `channel` String - 이벤트 이름
* `callback` Function

이벤트가 발생하면 `event` 객체와 `arg` 메시지와 함께 `callback`이 호출됩니다.

## IPC 이벤트

`callback`에서 전달된 `event` 객체는 다음과 같은 메서드와 속성을 가지고 있습니다:

### `Event.returnValue`

이 메시지를 지정하면 동기 메시지를 전달합니다.

### `Event.sender`

메시지를 보낸 `webContents` 객체를 반환합니다. `event.sender.send` 메서드를 통해 비동기로 메시지를 전달할 수 있습니다.
자세한 내용은 [webContents.send][webcontents-send]를 참고하세요.

[webcontents-send]: web-contents.md#webcontentssendchannel-args
