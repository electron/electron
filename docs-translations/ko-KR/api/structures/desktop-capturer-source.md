# DesktopCapturerSource Object

* `id` String - [`navigator.webkitGetUserMedia`] 를 호출할 때
  `chromeMediaSourceId` 제한으로 사용될 수 있는 윈도우 또는 화면의 식별자.
  식별자의 형식은 `window:XX` 또는 `screen:XX` 이 될 것 이며, `XX` 는 무작위로
  생성된 숫자입니다.
* `name` String - 윈도우 소스의 이름이 윈도우 제목과 일치하면, 화면 소스는
  `Entire Screen` 또는 `Screen <index>` 으로 명명될 것 입니다.
* `thumbnail` [NativeImage](../native-image.md) - 섬네일 이미지. **참고:**
  `desktopCapturer.getSources` 에 넘겨진 `options` 에 명시된 `thumbnailSize` 와
  섬네일의 크기가 같음을 보장하지 않습니다. 실제 크기는 화면이나 윈도우의 규모에
  의해 결정됩니다.
