# netLog

> Logging network events for a session.

Process: [Main](../glossary.md#main-process)

```javascript
const { netLog } = require('electron')

app.on('ready', function () {
  netLog.startLogging('/path/to/net-log')
  // After some network events
  netLog.stopLogging(path => {
    console.log('Net-logs written to', path)
  })
})
```

See [`--log-net-log`](chrome-command-line-switches.md#--log-net-logpath) to log network events throughout the app's lifecycle.

**Note:** All methods unless specified can only be used after the `ready` event
of the `app` module gets emitted.

## Methods

### `netLog.startLogging(path)`

* `path` String - File path to record network logs.

Starts recording network events to `path`.

### `netLog.stopLogging([callback])`

* `callback` Function (optional)
  * `path` String - File path to which network logs were recorded.

Stops recording network events. If not called, net logging will automatically end when app quits.

## Properties

### `netLog.currentlyLogging`

A `Boolean` property that indicates whether network logs are recorded.

### `netLog.currentlyLoggingPath`

A `String` property that returns the path to the current log file.
