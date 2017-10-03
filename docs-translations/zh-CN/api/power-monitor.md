# powerMonitor

> 监视电源状态更改。

进程： [Main](../glossary.md#main-process)

在 `app` 模块的 `ready` 事件触发之后就不能使用这个模块了。

例如:

```javascript
const electron = require('electron')
const {app} = electron

app.on('ready', () => {
  electron.powerMonitor.on('suspend', () => {
    console.log('The system is going to sleep')
  })
})
```

## 事件

`powerMonitor` 模块可以触发下列事件：

### Event: 'suspend'

在系统挂起的时候触发。

### Event: 'resume'

在系统恢复继续工作的时候触发。

### Event: 'on-ac'

在系统使用交流电的时候触发。

### Event: 'on-battery'

在系统使用电池电源的时候触发。
