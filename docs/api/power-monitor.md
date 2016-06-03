# powerMonitor

> Monitor power state changes.

You can only use it in the main process. You should not use this module until the `ready`
event of the `app` module is emitted.

For example:

```javascript
app.on('ready', () => {
  require('electron').powerMonitor.on('suspend', () => {
    console.log('The system is going to sleep');
  });
});
```

## Events

The `power-monitor` module emits the following events:

### Event: 'suspend'

Emitted when the system is suspending.

### Event: 'resume'

Emitted when system is resuming.

### Event: 'on-ac' _Windows_

Emitted when the system changes to AC power.

### Event: 'on-battery' _Windows_

Emitted when system changes to battery power.
