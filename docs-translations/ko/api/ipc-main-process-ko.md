# ipc (main process)

랜더러 프로세스(웹 페이지)로 부터 동기 또는 비동기로 메시지를 받아 처리합니다.

랜더러로부터 발신된 메시지들은 모두 이 모듈에서 `channel` 이라는 특정 이벤트 이름을 통해 수신할 수 있습니다.
동기 메시지는 `event.returnValue`를 이용하여 반환값(답장)을 설정할 수 있습니다. 비동기 메시지라면 `event.sender.send(...)`를 사용하면 됩니다.

또한 메인 프로세스에서 랜더러 프로세스로 메시지를 보내는 것도 가능합니다.
자세한 내용은 [WebContents.send](browser-window-ko.md#webcontentssendchannel-args)를 참고 하세요.

보내진 메시지들을 처리하는 예제입니다:

```javascript
// 메인 프로세스에서 처리.
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
// 랜더러 프로세스에서의 처리 (web page).
var ipc = require('ipc');
console.log(ipc.sendSync('synchronous-message', 'ping')); // prints "pong"

ipc.on('asynchronous-reply', function(arg) {
  console.log(arg); // prints "pong"
});
ipc.send('asynchronous-message', 'ping');
```

## Class: Event

### Event.returnValue

동기 메시지를 설정합니다.

### Event.sender

메시지를 보내온 sender `WebContents` 객체입니다.
