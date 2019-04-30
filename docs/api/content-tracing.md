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
const { app, contentTracing } = require('electron')

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

### `contentTracing.getCategories()`

Returns `Promise<String[]>` - resolves with an array of category groups once all child processes have acknowledged the `getCategories` request

Get a set of category groups. The category groups can change as new code paths are reached.

### `contentTracing.startRecording(options)`

* `options` ([TraceCategoriesAndOptions](structures/trace-categories-and-options.md) | [TraceConfig](structures/trace-config.md))

Returns `Promise<void>` - resolved once all child processes have acknowledged the `startRecording` request.

Start recording on all processes.

Recording begins immediately locally and asynchronously on child processes
as soon as they receive the EnableRecording request.

### `contentTracing.stopRecording(resultFilePath)`

* `resultFilePath` String

Returns `Promise<String>` - resolves with a file that contains the traced data once all child processes have acknowledged the `stopRecording` request

Stop recording on all processes.

Child processes typically cache trace data and only rarely flush and send
trace data back to the main process. This helps to minimize the runtime overhead
of tracing since sending trace data over IPC can be an expensive operation. So,
to end tracing, we must asynchronously ask all child processes to flush any
pending trace data.

Trace data will be written into `resultFilePath` if it is not empty or into a
temporary file.

### `contentTracing.getTraceBufferUsage()`

Returns `Promise<Object>` - Resolves with an object containing the `value` and `percentage` of trace buffer maximum usage

Get the maximum usage across processes of trace buffer as a percentage of the
full state.
