# Pepper 플래시 플러그인 사용하기

Electron은 Pepper 플래시 플러그인을 지원합니다.
Pepper 플래시 플러그인을 사용하려면 Pepper 플래시 플러그인의 위치를 지정해야 합니다.

## 플래시 플러그인 준비하기

크롬 브라우저의 `chrome://plugins` 페이지에 접속한 후 `세부정보`에서 플래시 플러그인의 위치와 버전을 찾을 수 있습니다.
Electron에서 플래시 플러그인을 지원하기 위해선 이 두 가지를 복사해 와야 합니다.

## Electron 스위치 추가

플러그인을 사용하려면 Electron 커맨드 라인에 `--ppapi-flash-path` 와 `ppapi-flash-version` 플래그를 app의 ready 이벤트가 호출되기 전에 추가해야 합니다.
그리고 `browser-window`에 `plugins` 스위치도 추가해야합니다.

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

// Report crashes to our server.
require('crash-reporter').start();

var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// 플래시 플러그인의 위치를 설정합니다.
// Windows의 경우, /path/to/pepflashplayer.dll
// Mac의 경우, /path/to/PepperFlashPlayer.plugin
// Linux의 경우, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    'web-preferences': {
      'plugins': true
    }
  });
  mainWindow.loadUrl('file://' + __dirname + '/index.html');
  // Something else
});
```

## `<webview>` 태그를 이용하여 플러그인을 활성화

`plugins` 속성을 `<webview>` 태그에 추가합니다.

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
