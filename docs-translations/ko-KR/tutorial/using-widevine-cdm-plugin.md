# Widevine CDM 플러그인 사용하기

Electron에선 Chrome 브라우저에서 취득해온 Widevine CDM 플러그인을 사용할 수 있습니다.

## 플러그인 취득

Electron은 라이센스상의 문제로 Widevine CDM 플러그인을 직접 제공하지 않습니다.
따라서 플러그인을 얻으려면 먼저 사용할 Electron 빌드의 아키텍쳐와 버전에 맞춰 공식
Chrome 브라우저를 설치해야 합니다.

**참고:** Chrome 브라우저의 메이저 버전은 Electron에서 사용하는 Chrome 버전과
같습니다, 만약 그렇지 않다면 `navigator.plugins`가 로드됐더라도 정상적으로 작동하지
않습니다.

### Windows & OS X

Chrome 브라우저에서 `chrome://components/`를 열고 `WidevineCdm`을 찾은 후 확실히
최신버전인지 확인합니다. 여기까지 하면 모든 플러그인 바이너리를
`APP_DATA/Google/Chrome/WidevineCDM/VERSION/_platform_specific/PLATFORM_ARCH/`
디렉터리에서 찾을 수 있습니다.

`APP_DATA`는 어플리케이션 데이터를 저장하고 있는 시스템 경로입니다. Windows에선
`%LOCALAPPDATA%`로 접근할 수 있고 OS X에선 `~/Library/Application Support`로
접근할 수 있습니다. `VERSION`은 `1.4.8.866` 같은 Widevine CDM 플러그인의 버전
문자열입니다. `PLATFORM`은 플랫폼을 뜻하며 `mac` 또는 `win`이 될 수 있으며 `ARCH`는
아키텍쳐를 뜻하고 `x86` 또는 `x64`가 될 수 있습니다.

Windows에선 `widevinecdm.dll` 와 `widevinecdmadapter.dll` 같은 바이너리를
요구하며 OS X에선 `libwidevinecdm.dylib`와 `widevinecdmadapter.plugin` 바이너리를
요구합니다. 원하는 곳에 이들을 복사해 놓을 수 있습니다. 하지만 반드시 바이너리는 같은
위치에 두어야 합니다.

### Linux

Linux에선 플러그인 바이너리들이 Chrome 브라우저와 함께 제공됩니다.
`/opt/google/chrome` 경로에서 찾을 수 있으며, 파일 이름은 `libwidevinecdm.so`와
`libwidevinecdmadapter.so`입니다.

## 플러그인 사용

플러그인 파일을 가져온 후, Electron의 `--widevine-cdm-path` 커맨드 라인 스위치에
`widevinecdmadapter`의 위치를 전달하고 플러그인의 버전을 `--widevine-cdm-version`
스위치에 전달해야 합니다.

**참고:** `widevinecdmadapter` 바이너리가 Electron으로 전달되어도, `widevinecdm`
바이너리는 옆에 같이 두어야 합니다.

커맨드 라인 스위치들은 `app` 모듈의 `ready` 이벤트가 발생하기 전에 전달되어야 합니다.
그리고 이 플러그인을 사용하는 페이지는 플러그인(속성)이 활성화되어있어야 합니다.

예시 코드:

```javascript
// `widevinecdmadapter`의 파일 이름을 이곳에 전달해야 합니다. 파일 이름은
// * OS X에선 `widevinecdmadapter.plugin`로 지정합니다,
// * Linux에선 `libwidevinecdmadapter.so`로 지정합니다,
// * Windows에선 `widevinecdmadapter.dll`로 지정합니다.
app.commandLine.appendSwitch('widevine-cdm-path', '/path/to/widevinecdmadapter.plugin');
// 플러그인의 버전은 크롬의 `chrome://plugins` 페이지에서 취득할 수 있습니다.
app.commandLine.appendSwitch('widevine-cdm-version', '1.4.8.866');

let mainWindow = null;
app.on('ready', () => {
  mainWindow = new BrowserWindow({
    webPreferences: {
      // `plugins`은 활성화되어야 합니다.
      plugins: true
    }
  })
});
```

## 플러그인 작동 확인

플러그인 정상적으로 작동하는지 확인하려면 다음과 같은 방법을 사용할 수 있습니다:

* 개발자 도구를 연 후 `navigator.plugins`를 확인하여 Widevine CDM 플러그인이
  포함되었는지 알 수 있습니다.
* https://shaka-player-demo.appspot.com/를 열어, `Widevine`을 사용하는 설정을
  로드합니다.
* http://www.dash-player.com/demo/drm-test-area/를 열어, 페이지에서
  `bitdash uses Widevine in your browser`라고 적혀있는지 확인하고 비디오를 재생합니다.
