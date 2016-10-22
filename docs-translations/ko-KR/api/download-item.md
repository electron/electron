# DownloadItem

> 원격 소스로부터의 파일 다운로드를 제어합니다.

`DownloadItem`은 `EventEmitter`를 상속받았으며 Electron의 다운로드 아이템을
표현합니다. 이 클래스 객체는 `Session` 클래스의 `will-download` 이벤트에 사용되며
사용자가 다운로드 아이템을 다룰 수 있도록 도와줍니다.

```javascript
// 메인 프로세스
win.webContents.session.on('will-download', (event, item, webContents) => {
  // Set the save path, making Electron not to prompt a save dialog.
  item.setSavePath('/tmp/save.pdf')

  item.on('updated', (event, state) => {
    if (state === 'interrupted') {
      console.log('Download is interrupted but can be resumed')
    } else if (state === 'progressing') {
      if (item.isPaused()) {
        console.log('Download is paused')
      } else {
        console.log(`Received bytes: ${item.getReceivedBytes()}`)
      }
    }
  })
  item.once('done', (event, state) => {
    if (state === 'completed') {
      console.log('Download successfully')
    } else {
      console.log(`Download failed: ${state}`)
    }
  })
})
```

## Events

### Event: 'updated'

Returns:

* `event` Event
* `state` String

다운로드가 업데이트되었으며 아직 끝나지 않았을 때 발생하는 이벤트입니다.

`state`는 다음 중 하나가 될 수 있습니다:

* `progressing` - 다운로드가 진행중입니다.
* `interrupted` - 다운로드가 중지되었으며 다시 재개할 수 있습니다.

### Event: 'done'

Returns:

* `event` Event
* `state` String

다운로드가 종료될 때 발생하는 이벤트입니다. 이 이벤트는 다운로드 중 문제가 발생하여
중단되거나, 모두 성공적으로 완료된 경우, `downloadItem.cancel()` 같은 메서드를 통해
취소하는 경우의 종료 작업이 모두 포함됩니다.

`state`는 다음 중 하나가 될 수 있습니다:

* `completed` - 다운로드가 성공적으로 완료되었습니다.
* `cancelled` - 다운로드가 취소되었습니다.
* `interrupted` - 다운로드가 중지되었으며 다시 재개할 수 있습니다.

## Methods

`downloadItem` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `downloadItem.setSavePath(path)`

* `path` String - 다운로드 아이템을 저장할 파일 경로를 지정합니다.

이 API는 세션의 `will-download` 콜백 함수에서만 사용할 수 있습니다. 사용자가 API를
통해 아무 경로도 설정하지 않을 경우 Electron은 기본 루틴 파일 저장을 실행합니다.
(파일 대화 상자를 엽니다)

### `downloadItem.getSavePath()`

Returns `String` - 다운로드 아이템의 저장 경로. 이 경로는
`downloadItem.setSavePath(path)`로 설정된 경로나 나타난 저장 대화상자에서 선택한
경로 중 하나가 될 수 있습니다.

### `downloadItem.pause()`

다운로드를 일시 중지합니다.

### `downloadItem.isPaused()`

Returns `Boolean` - 다운로드가 일시 중지되었는지 여부.

### `downloadItem.resume()`

중디된 다운로드를 재개합니다.

### `downloadItem.canResume()`

Returns `Boolean` - 다운로드를 재개할 수 있는지 여부.

### `downloadItem.cancel()`

다운로드를 취소합니다.

### `downloadItem.getURL()`

Returns `String` - 아이템을 다운로드 한 URL.

### `downloadItem.getMimeType()`

Returns `String` - 파일의 MIME 타입.

### `downloadItem.hasUserGesture()`

Returns `Boolean` - 유저 제스쳐(작업)로인한 다운로드인지 여부.

### `downloadItem.getFilename()`

Returns `String` - 다운로드 아이템의 파일 이름.

**참고:** 실제 파일 이름과 로컬 디스크에 저장되는 파일의 이름은 서로 다를 수 있습니다.
예를 들어 만약 사용자가 파일을 저장할 때 파일 이름을 바꿨다면 실제 파일 이름과 저장
파일 이름은 서로 달라지게 됩니다.

### `downloadItem.getTotalBytes()`

Returns `Integer` - 다운로드 아이템의 바이트 단위의 전체 크기. 크기를 알 수
없으면 0.

### `downloadItem.getReceivedBytes()`

Returns `Integer` - 다운로드 아이템의 수신된 바이트.

### `downloadItem.getContentDisposition()`

Return `String` - 응답 헤더의 Content-Disposition 필드.

### `downloadItem.getState()`

Return `String` - 현재 상태.

값은 다음이 될 수 있습니다:

* `progressing` - 다운로드가 진행중입니다.
* `completed` - 다운로드가 성공적으로 완료되었습니다.
* `cancelled` - 다운로드가 취소되었습니다.
* `interrupted` - 다운로드가 중지되었습니다.
