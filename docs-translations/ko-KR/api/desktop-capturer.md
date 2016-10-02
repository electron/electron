# desktopCapturer

> 멀티미디어 소스에 대해 접근하고 [`navigator.webkitGetUserMedia`] API를 통해
> 오디오나 비디오를 데스크톱으로부터 캡쳐할 수 있도록 합니다.

다음 예시는 창 제목이 `Electron`인 데스크톱 창에 대해 비디오 캡쳐하는 방법을
보여줍니다.

```javascript
// 렌더러 프로세스에서.
const {desktopCapturer} = require('electron')

desktopCapturer.getSources({types: ['window', 'screen']}, (error, sources) => {
  if (error) throw error
  for (let i = 0; i < sources.length; ++i) {
    if (sources[i].name === 'Electron') {
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
      }, handleStream, handleError)
      return
    }
  }
})

function handleStream (stream) {
  document.querySelector('video').src = URL.createObjectURL(stream)
}

function handleError (e) {
  console.log(e)
}
```


`desktopCapturer`로부터 제공된 소스로부터 비디오 캡쳐를 하려면
[`navigator.webkitGetUserMedia`]로 전달되는 속성에 `chromeMediaSource: 'desktop'`,
와 `audio: false` 가 반드시 포함되어야 합니다.

전체 데스크톱의 오디오와 비디오를 모두 캡쳐하려면 [`navigator.webkitGetUserMedia`]로
전달되는 속성에 `chromeMediaSource: 'screen'`, 와 `audio: true` 가 반드시
포함되어야 하지만 `chromeMediaSourceId` 속성은 포함되어선 안됩니다.

## Methods

`desktopCapturer` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `desktopCapturer.getSources(options, callback)`

* `options` Object
  * `types` Array - 캡쳐될 데스크톱 소스의 종류를 나열하는 문자열을 담은 배열, 종류는
    `screen` 또는 `window`가 될 수 있습니다.
  * `thumbnailSize` Object (optional) -  미디어 소스 섬네일의 크기가 맞춰져야 할
    제안된 크기, 기본값은 `{width: 150, height: 150}`입니다.
* `callback` Function

사용할 수 있는 데스크톱 미디어 소스를 가져오기 시작하고 작업이 완료되면
`callback(error, sources)`가 호출됩니다.

`sources`는 `Source`객체의 배열이며, 각 `Source`는 캡쳐될 수 있는 스크린과 각기
윈도우를 표현합니다. 그리고 다음과 같은 속성을 가지고 있습니다:

* `id` String - 윈도우 또는 스크린의 식별자로써 [`navigator.webkitGetUserMedia`]를
  호출할 때 `chromeMediaSourceId` 속성에 사용될 수 있습니다. 식별자의 형식은
  `window:XX` 또는 `screen:XX` 이며 `XX` 부분은 무작위로 생성된 숫자입니다.
* `name` String - `Entire Screen` 또는 `Screen <index>`로 이름지어질 스크린
  소스이며, 이는 윈도우 제목에 일치하는 윈도우 소스의 이름이 됩니다.
* `thumbnail` [NativeImage](native-image.md) - 섬네일 이미지입니다. **참고:**
  `desktopCapturer.getSources`로 전달된 `options` 객체의 `thumnbailSize` 속성과
  같이 이 섬네일의 사이즈가 완전히 같을 것이라고 보장하지 않습니다. 실질적인 크기는
  스크린과 윈도우의 비율에 따라 달라질 수 있습니다.

[`navigator.webkitGetUserMedia`]: https://developer.mozilla.org/en/docs/Web/API/Navigator/getUserMedia
