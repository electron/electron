# 온라인/오프라인 이벤트 감지

온라인/오프라인 이벤트는 다음 예시와 같이 렌더러 프로세스에서 표준 HTML5 API를
이용하여 구현할 수 있습니다.

_main.js_

```javascript
const {app, BrowserWindow} = require('electron')

let onlineStatusWindow

app.on('ready', () => {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL(`file://${__dirname}/online-status.html`)
})
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const alertOnlineStatus = () => {
    window.alert(navigator.onLine ? 'online' : 'offline')
  }

  window.addEventListener('online',  alertOnlineStatus)
  window.addEventListener('offline',  alertOnlineStatus)

  alertOnlineStatus()
</script>
</body>
</html>
```

메인 프로세스에서 이 이벤트를 처리할 필요가 있는 경우 이벤트를 메인 프로세스로
보낼 수 있습니다. 메인 프로세스는 `navigator` 객체를 가지고 있지 않기 때문에 이
이벤트를 직접 사용할 수는 없습니다.

대신 다음 예시와 같이 Electron의 inter-process communication(ipc) 유틸리티를
사용하면 이벤트를 메인 프로세스로 전달할 수 있습니다.

_main.js_

```javascript
const {app, BrowserWindow, ipcMain} = require('electron')
let onlineStatusWindow

app.on('ready', () => {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL(`file://${__dirname}/online-status.html`)
})

ipcMain.on('online-status-changed', (event, status) => {
  console.log(status)
})
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const {ipcRenderer} = require('electron')
  const updateOnlineStatus = () => {
    ipcRenderer.send('online-status-changed', navigator.onLine ? 'online' : 'offline')
  }

  window.addEventListener('online',  updateOnlineStatus)
  window.addEventListener('offline',  updateOnlineStatus)

  updateOnlineStatus()
</script>
</body>
</html>
```

**참고:** Electron 이 근거리 통신망 (LAN) 또는 라우터에 연결할 수 없는 경우,
오프라인으로 간주됩니다; 그 외의 경우는 `true` 를 반환합니다. 그래서
`navigator.onLine` 이 `false` 값을 반환하면 Electron 이 오프라인이라고 가정할 수
있습니다. 하지만 `true` 값은 Electron 이 인터넷에 접근할 수 있다고 가정할 수
없습니다. 항상 "연결된" 가상 이더넷 어댑터를 가지고 있는 가상화 소프트웨어
상에서 작동하는 경우 잘못된 반응을 얻을 수 있습니다. 그러므로, Electron 의
인터넷 접근 상태를 확인하려면, 확인하기 위한 추가적인 개발을 해야합니다.
