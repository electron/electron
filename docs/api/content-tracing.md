# contentTracing

> Collect tracing data from Chromium's content module for finding performance
bottlenecks and slow operations.

This module does not include a web interface so you need to open
`chrome://tracing/` in a Chrome browser and load the generated file to view the
result.

```javascript
const {contentTracing} = require('electron');

const options = {
  categoryFilter: '*',
  traceOptions: 'record-until-full,enable-sampling'
};

contentTracing.startRecording(options, () => {
  console.log('Tracing started');

  setTimeout(() => {
    contentTracing.stopRecording('', (path) => {
      console.log('Tracing data recorded to ' + path);
    });
  }, 5000);
});
```

## Methods

The `contentTracing` module has the following methods:

### `contentTracing.getCategories(callback)`

* `callback` Function

Get a set of category groups. The category groups can change as new code paths
are reached.

Once all child processes have acknowledged the `getCategories` request the
`callback` is invoked with an array of category groups.

### `contentTracing.startRecording(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

Start recording on all processes.

Recording begins immediately locally and asynchronously on child processes
as soon as they receive the EnableRecording request. The `callback` will be
called once all child processes have acknowledged the `startRecording` request.

`categoryFilter` is a filter to control what category groups should be
traced. A filter can have an optional `-` prefix to exclude category groups
that contain a matching category. Having both included and excluded
category patterns in the same list is not supported.

Examples:

* `test_MyTest*`,
* `test_MyTest*,test_OtherStuff`,
* `"-excluded_category1,-excluded_category2`

`traceOptions` controls what kind of tracing is enabled, it is a comma-delimited
list. Possible options are:

* `record-until-full`
* `record-continuously`
* `trace-to-console`
* `enable-sampling`
* `enable-systrace`

The first 3 options are trace recoding modes and hence mutually exclusive.
If more than one trace recording modes appear in the `traceOptions` string,
the last one takes precedence. If none of the trace recording modes are
specified, recording mode is `record-until-full`.

The trace option will first be reset to the default option (`record_mode` set to
`record-until-full`, `enable_sampling` and `enable_systrace` set to `false`)
before options parsed from `traceOptions` are applied on it.

### `contentTracing.stopRecording(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

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

Get the maximum usage across processes of trace buffer as a percentage of the
full state. When the TraceBufferUsage value is determined the `callback` is
called.

### `contentTracing.setWatchEvent(categoryName, eventName, callback)`

* `categoryName` String
* `eventName` String
* `callback` Function

`callback` will be called every time the given event occurs on any
process.

### `contentTracing.cancelWatchEvent()`

Cancel the watch event. This may lead to a race condition with the watch event
callback if tracing is enabled.
