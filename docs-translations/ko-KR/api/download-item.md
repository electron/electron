# DownloadItem

`DownloadItem`은 EventEmitter를 상속받았으며 Electron의 다운로드 아이템을 표현합니다.
이 클래스 객체는 `Session` 모듈의 `will-download` 이벤트에 사용되며 사용자가 다운로드
아이템을 다룰 수 있도록 도와줍니다.

```javascript
// 메인 프로세스
win.webContents.session.on('will-download', function(event, item, webContents) {
  // Set the save path, making Electron not to prompt a save dialog.
  item.setSavePath('/tmp/save.pdf');
  console.log(item.getMimeType());
  console.log(item.getFilename());
  console.log(item.getTotalBytes());
  item.on('updated', function() {
    console.log('Received bytes: ' + item.getReceivedBytes());
  });
  item.on('done', function(e, state) {
    if (state == "completed") {
      console.log("Download successfully");
    } else {
      console.log("Download is cancelled or interrupted that can't be resumed");
    }
  });
});
```

## Events

### Event: 'updated'

`downloadItem`이 갱신될 때 발생하는 이벤트입니다.

### Event: 'done'

* `event` Event
* `state` String
  * `completed` - 다운로드가 성공적으로 완료되었습니다.
  * `cancelled` - 다운로드가 취소되었습니다.
  * `interrupted` - 다운로드 중 파일 서버로부터의 연결이 끊겼습니다.

다운로드가 종료될 때 발생하는 이벤트입니다. 이 이벤트는 다운로드 중 문제가 발생하여
중단되거나, 모두 성공적으로 완료된 경우, `downloadItem.cancel()` 같은 메서드를 통해
취소하는 경우의 종료 작업이 모두 포함됩니다.

## Methods

`downloadItem` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `downloadItem.setSavePath(path)`

* `path` String - 다운로드 아이템을 저장할 파일 경로를 지정합니다.

이 API는 세션의 `will-download` 콜백 함수에서만 사용할 수 있습니다. 사용자가 API를
통해 아무 경로도 설정하지 않을 경우 Electron은 기본 루틴 파일 저장을 실행합니다.
(파일 대화 상자를 엽니다)

### `downloadItem.pause()`

다운로드를 일시 중지합니다.

### `downloadItem.resume()`

중디된 다운로드를 재개합니다.

### `downloadItem.cancel()`

다운로드를 취소합니다.

### `downloadItem.getURL()`

다운로드 아이템의 URL을 표현하는 문자열을 반환합니다.

### `downloadItem.getMimeType()`

다우로드 아이템의 MIME 타입을 표현하는 문자열을 반환합니다.

### `downloadItem.hasUserGesture()`

현재 다운로드가 유저 제스쳐(작업)로인한 다운로드인지 여부를 반환합니다.

### `downloadItem.getFilename()`

다운로드 아이템의 파일 이름을 표현하는 문자열을 반환합니다.

**참고:** 실제 파일 이름과 로컬 디스크에 저장되는 파일의 이름은 서로 다를 수 있습니다.
예를 들어 만약 사용자가 파일을 저장할 때 파일 이름을 바꿨다면 실제 파일 이름과 저장
파일 이름은 서로 달라지게 됩니다.

### `downloadItem.getTotalBytes()`

현재 아이템의 전체 다운로드 크기를 정수로 반환합니다. 크기가 unknown이면 0을
반환합니다.

### `downloadItem.getReceivedBytes()`

현재 아이템의 다운로드 완료된 바이트 값을 정수로 반환합니다.

### `downloadItem.getContentDisposition()`

응답 헤더에서 Content-Disposition 필드를 문자열로 반환합니다.
