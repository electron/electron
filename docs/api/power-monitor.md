# power-monitor

The `power-monitor` module is used to monitor the power state change. You can
only use it on the main process. You should not use this module until the `ready` 
event of `app` module gets emitted.

An example is:

```javascript
var app = require('app');

app.on('ready', function() {
  require('power-monitor').on('suspend', function() {
    console.log('The system is going to sleep');
  });
});
```

## Event: suspend

Emitted when the system is suspending.

## Event: resume

Emitted when system is resuming.

## Event: on-ac

Emitted when the system changes to AC power.

## Event: on-battery

Emitted when system changes to battery power.
