# powerMonitor

> Monitor power state changes.

Process: [Main](../tutorial/quick-start.md#main-process)

You cannot require or use this module until the `ready` event of the `app`
module is emitted.

For example:

```javascript
const electron = require('electron')
const {app} = electron

app.on('ready', () => {
  electron.powerMonitor.on('suspend', () => {
    console.log('The system is going to sleep')
  })
})
```

## Events

The `powerMonitor` module emits the following events:

### Event: 'suspend'

Emitted when the system is suspending.

### Event: 'resume'

Emitted when system is resuming.

### Event: 'on-ac' _Windows_

Emitted when the system changes to AC power.

### Event: 'on-battery' _Windows_

Emitted when system changes to battery power.
