# 오프스크린 렌더링

오프스크린 렌더링은 비트맵에 브라우저 윈도우의 컨텐츠를 얻게 합니다. 그래서
아무곳에서나 렌더링 될 수 있습니다. 예를 들어 3D 에서 텍스처위에 렌더링 될 수
있습니다. Electron 의 오프스크린 렌더링은 [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef)
프로젝트와 비슷한 접근방식을 사용합니다.

두 방식의 렌더링을 사용할 수 있고 효율적으로 하기 위해 변경된 영역만 `'paint'`
이벤트에 전달됩니다. 렌더링을 중지하거나, 계속하거나, 프레임 속도를 변경할 수
있습니다. 명시된 프레임 속도는 상한선입니다. 웹페이지에 아무일도 발생하지
않으면 프레임이 생성되지 않습니다. 프레임 속도 최고값은 60입니다. 그 이상은
이점은 없고, 성능 저하만 발생합니다.

## 두가지 렌더링 방식

### GPU 가속

GPU 가속 렌더링은 컴포지션에 GPU 가 사용되는 것을 의미합니다. 프레임이 GPU 에서
복사되기 때문에 더 많은 성능이 필요합니다. 그래서 이 방식이 좀 더 느립니다.
이 방식의 장점은 WebGL 과 3D CSS 애니메이션 지원입니다.

### 소프트웨어 출력 장치

이 방식은 CPU에서 렌더링을 위해 소프트웨어 출력 장치를 사용하여 프레임 생성이
더 빠릅니다. 그래서 이 방식을 GPU 가속보다 선호합니다.

이 방식을 사용하려면 [`app.disableHardwareAcceleration()`][disablehardwareacceleration]
API 를 호출하여 GPU 가속을 비활성화 하여야합니다.

## 사용법

``` javascript
const {app, BrowserWindow} = require('electron')

app.disableHardwareAcceleration()

let win
app.once('ready', () => {
  win = new BrowserWindow({
    webPreferences: {
      offscreen: true
    }
  })
  win.loadURL('http://github.com')
  win.webContents.on('paint', (event, dirty, image) => {
    // updateBitmap(dirty, image.getBitmap())
  })
  win.webContents.setFrameRate(30)
})
```

[disablehardwareacceleration]: ../api/app.md#appdisablehardwareacceleration
