# contentTracing

> Collect tracing data from Chromium's content module for finding performance
bottlenecks and slow operations.

Process: [Main](../glossary.md#main-process)

This module does not include a web interface so you need to open
`chrome://tracing/` in a Chrome browser and load the generated file to view the
result.

**Note:** You should not use this module until the `ready` event of the app
module is emitted.


```javascript
const {app, contentTracing} = require('electron')

app.on('ready', () => {
  const options = {
    categoryFilter: '*',
    traceOptions: 'record-until-full,enable-sampling'
  }

  contentTracing.startRecording(options, () => {
    console.log('Tracing started')

    setTimeout(() => {
      contentTracing.stopRecording('', (path) => {
        console.log('Tracing data recorded to ' + path)
      })
    }, 5000)
  })
})
```

## Methods

The `contentTracing` module has the following methods:

### `contentTracing.getCategories(callback)`

* `callback` Function
  * `categories` String[]

Get a set of category groups. The category groups can change as new code paths
are reached.

Once all child processes have acknowledged the `getCategories` request the
`callback` is invoked with an array of category groups.

### `contentTracing.startRecording(options, callback)`

* `options` ([TraceCategoriesAndOptions](structures/trace-categories-and-options.md) | [TraceConfig](structures/trace-config.md))
* `callback` Function

Start recording on all processes.

Recording begins immediately locally and asynchronously on child processes
as soon as they receive the EnableRecording request. The `callback` will be
called once all child processes have acknowledged the `startRecording` request.

### `contentTracing.stopRecording(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function
  * `resultFilePath` String

Stop recording on all processes.

Child processes typically cache trace data and only rarely flush and send
trace data back to the main process. This helps to minimize the runtime overhead
of tracing since sending trace data over IPC can be an expensive operation. So,
to end tracing, we must asynchronously ask all child processes to flush any
pending trace data.

Once all child processes have acknowledged the `stopRecording` request,
`callback` will be called with a file that contains the traced data.

Trace data will be written into `resultFilePath` if it is not empty or into a
temporary file. The actual file path will be passed to `callback` if it's not
`null`.

### `contentTracing.startMonitoring(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

Start monitoring on all processes.

Monitoring begins immediately locally and asynchronously on child processes as
soon as they receive the `startMonitoring` request.

Once all child processes have acknowledged the `startMonitoring` request the
`callback` will be called.

### `contentTracing.stopMonitoring(callback)`

* `callback` Function

Stop monitoring on all processes.

Once all child processes have acknowledged the `stopMonitoring` request the
`callback` is called.

### `contentTracing.captureMonitoringSnapshot(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function
  * `resultFilePath` String

Get the current monitoring traced data.

Child processes typically cache trace data and only rarely flush and send
trace data back to the main process. This is because it may be an expensive
operation to send the trace data over IPC and we would like to avoid unneeded
runtime overhead from tracing. So, to end tracing, we must asynchronously ask
all child processes to flush any pending trace data.

Once all child processes have acknowledged the `captureMonitoringSnapshot`
request the `callback` will be called with a file that contains the traced data.


### `contentTracing.getTraceBufferUsage(callback)`

* `callback` Function
  * `value` Number
  * `percentage` Number

Get the maximum usage across processes of trace buffer as a percentage of the
full state. When the TraceBufferUsage value is determined the `callback` is
called.
