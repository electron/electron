# powerMonitor

`power-monitor`模块是用来监听能源区改变的.只能在主进程中使用.在 `app` 模块的 `ready` 事件触发之后就不能使用这个模块了.

例如:

```javascript
app.on('ready', function() {
  require('electron').powerMonitor.on('suspend', function() {
    console.log('The system is going to sleep');
  });
});
```

## 事件

`power-monitor` 模块可以触发下列事件:

### Event: 'suspend'

在系统挂起的时候触发.

### Event: 'resume'

在系统恢复继续工作的时候触发.
Emitted when system is resuming.

### Event: 'on-ac'

在系统使用交流电的时候触发.
Emitted when the system changes to AC power.

### Event: 'on-battery'

在系统使用电池电源的时候触发.
Emitted when system changes to battery power.