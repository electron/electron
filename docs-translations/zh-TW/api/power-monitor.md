# power-monitor

`power-monitor` 模組用來監看電源狀態的改變。你只能在主行程 (main process) 裡面使用。
你應該要等到 `ready` 在 `app` 模組裡的事件被觸發 (emit)，再使用這個模組。

舉例來說:

```javascript
var app = require('app');

app.on('ready', function() {
  require('power-monitor').on('suspend', function() {
    console.log('The system is going to sleep');
  });
});
```

## 事件 (Events)

`power-monitor` 模組會觸發 (emits) 以下幾個事件:

### 事件: 'suspend'

當系統進入 睡眠 (suspend) 時觸發。

### 事件: 'resume'

當系統 resume  時觸發。

### 事件: 'on-ac'

當系統改變使用交流電源 (AC) 時觸發。

### 事件: 'on-battery'

當系統改變使用電池店員時觸發。
