# desktopCapturer

`desktopCapturer` 모듈은 `getUserMedia`에서 사용 가능한 소스를 가져올 때 사용할 수
있습니다.

```javascript
// 렌더러 프로세스 내부
var desktopCapturer = require('electron').desktopCapturer;

desktopCapturer.getSources({types: ['window', 'screen']}, function(error, sources) {
  if (error) throw error;
  for (var i = 0; i < sources.length; ++i) {
    if (sources[i].name == "Electron") {
      navigator.webkitGetUserMedia({
        audio: false,
        video: {
          mandatory: {
            chromeMediaSource: 'desktop',
            chromeMediaSourceId: sources[i].id,
            minWidth: 1280,
            maxWidth: 1280,
            minHeight: 720,
            maxHeight: 720
          }
        }
      }, gotStream, getUserMediaError);
      return;
    }
  }
});

function gotStream(stream) {
  document.querySelector('video').src = URL.createObjectURL(stream);
}

function getUserMediaError(e) {
  console.log('getUserMediaError');
}
```

`navigator.webkitGetUserMedia` 호출에 대해 제약된 객체를 생성할 때
`desktopCapturer`에서 소스를 사용한다면 `chromeMediaSource`은 반드시
`"desktop"`으로 지정되어야 하며, `audio` 도 반드시 `false`로 지정되어야 합니다.

만약 전체 데스크탑에서 오디오와 비디오 모두 캡쳐를 하고 싶을 땐 `chromeMediaSource`를
`"screen"` 그리고 `audio`를 `true`로 지정할 수 있습니다. 이 방법을 사용하면
`chromeMediaSourceId`를 지정할 수 없습니다.


## Methods

`desktopCapturer` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `desktopCapturer.getSources(options, callback)`

* `options` Object
  * `types` Array - 캡쳐될 데스크탑 소스의 타입에 대한 리스트를 배열로 지정합니다.
    사용 가능한 타입은 `screen`과 `window`입니다.
  * `thumbnailSize` Object (optional) - 섬네일 크기를 조정할 때 최대한 맞춰질 크기,
    기본값은 `{width: 150, height: 150}`입니다.
* `callback` Function

모든 데스크탑 소스를 요청합니다. 요청의 처리가 완료되면 `callback`은
`callback(error, sources)` 형식으로 호출됩니다.

`sources`는 `Source` 객체의 배열입니다. 각 `Source`는 캡쳐된 화면과 독립적인
윈도우를 표현합니다. 그리고 다음과 같은 속성을 가지고 있습니다:

* `id` String - `navigator.webkitGetUserMedia` API에서 사용할 수 있는 캡쳐된 윈도우
  또는 화면의 id입니다. 포맷은 `window:XX` 또는 `screen:XX`로 표현되며 `XX` 는
  무작위로 생성된 숫자입니다.
* `name` String - 캡쳐된 화면과 윈도우에 대해 묘사된 이름입니다. 만약 소스가
  화면이라면, `Entire Screen` 또는 `Screen <index>`가 될 것이고 소스가 윈도우라면,
  해당 윈도우의 제목이 반환됩니다.
* `thumbnail` [NativeImage](native-image.md) - 섬네일 네이티브 이미지.

**참고:** `source.thumbnail`의 크기는 언제나 `options`의 `thumnbailSize`와 같다고
보장할 수 없습니다. 섬네일의 크기는 화면과 윈도우의 크기에 의존하여 조정됩니다.
