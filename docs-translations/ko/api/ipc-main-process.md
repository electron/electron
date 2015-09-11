# ipc (main process)

`ipc` (main process) 모듈은 메인 프로세스에서 사용할 때 랜더러 프로세스(웹 페이지)에서 전달된 동기 또는 비동기 메시지를 보내고 받는 방법을 제공합니다.
랜더러 프로세스에서 메시지를 전달하면 이 모듈을 통해 메시지를 받을 수 있습니다.

## 메시지 전송

물론 메인 프로세스에서 랜더러 프로세스로 메시지를 보내는 것도 가능합니다.
자세한 내용은 [WebContents.send](web-contents.md#webcontentssendchannel-args)를 참고하세요.

- 메시지를 전송할 때 이벤트 이름은 `channel`이 됩니다.
- 메시지에 동기로 응답할 땐 반드시 `event.returnValue`를 설정해야 합니다.
- 메시지를 비동기로 응답할 땐 `event.sender.send(...)` 메서드를 사용할 수 있습니다.

랜더러 프로세스와 메인 프로세스간에 메시지를 전달하고 받는 예제입니다:

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
// 랜더러 프로세스 (web page)
var ipc = require('ipc');
console.log(ipc.sendSync('synchronous-message', 'ping')); // prints "pong"

ipc.on('asynchronous-reply', function(arg) {
  console.log(arg); // prints "pong"
});
ipc.send('asynchronous-message', 'ping');
```

## 메시지 리스닝

`ipc` 모듈은 다음과 같은 이벤트 메서드를 가지고 있습니다:

### `ipc.on(channel, callback)`

* `channel` String - 이벤트 이름
* `callback` Function

이벤트가 발생하면 `callback`에 `event` 객체와 `arg` 메시지가 포함되어 호출됩니다.

## IPC Events

`callback`에서 전달된 `event` 객체는 다음과 같은 메서드와 속성을 가지고 있습니다:

### `Event.returnValue`

이 메시지를 지정하면 동기 메시지를 전달합니다.

### `Event.sender`

메시지를 보낸 `WebContents` 객체를 반환합니다.

### `Event.sender.send(channel[, arg1][, arg2][, ...])`

* `channel` String - The event name.
* `arg` (optional)

랜더러 프로세스로 비동기 메시지를 전달합니다.
옵션으로 `arg`에 한 개 또는 여러 개의 메시지를 포함할 수 있습니다. 모든 타입을 사용할 수 있습니다.
