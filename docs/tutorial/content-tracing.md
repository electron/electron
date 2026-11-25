# Content Tracing

Electron's [`contentTracing`](../api/content-tracing.md) module records performance
events across Chromium processes. Use it to investigate work that spans the main
process, renderer processes, and GPU process, such as slow window creation or an
unresponsive interface while the main process handles an IPC message.

For profiling JavaScript in a single renderer, start with the DevTools
[Performance panel](https://developer.chrome.com/docs/devtools/performance/).
A content trace puts events from several processes on the same timeline so you
can investigate what another process was doing during a delay.

## Record a trace

Call `contentTracing.startRecording()` in the main process after `app.whenReady()`
resolves. Await it before starting the operation you want to record, and await
`contentTracing.stopRecording()` before reading the resulting file or quitting
the app. Stopping a recording flushes trace data from the child processes.

In the app from the [first application tutorial](tutorial-2-first-app.md), replace
the `app.whenReady()` handler with the following code. It records window creation,
the initial page load, and five seconds of activity after the page loads.

```js title='main.js'
const { app, BrowserWindow, contentTracing } = require('electron')
const path = require('node:path')
const { setTimeout: delay } = require('node:timers/promises')

app.whenReady().then(async () => {
  await contentTracing.startRecording({
    recording_mode: 'record-until-full',
    included_categories: ['*', 'electron', 'blink.user_timing']
  })

  const win = new BrowserWindow()
  await win.loadFile('index.html')
  await delay(5000)

  const tracePath = path.join(app.getPath('userData'), 'startup.pftrace')
  await contentTracing.stopRecording(tracePath)
  console.log(`Trace saved to ${tracePath}`)
})
```

The file is a [Perfetto protobuf trace](https://perfetto.dev/docs/reference/trace-packet-proto),
not JSON. If you omit the output path, `stopRecording()` writes to a temporary
file and returns its path instead. Only one recording can run at a time.

The `record-until-full` mode stops collecting events when the trace buffer fills.
For a longer reproduction where the interesting event happens near the end, use
`record-continuously` to retain the most recent events instead. See
[`TraceConfig`](../api/structures/trace-config.md) for buffer size and category options.

## Choose categories and mark application work

The `'*'` category includes the categories enabled by default. Categories prefixed
with `disabled-by-default-` must be enabled explicitly. For example, add
`disabled-by-default-v8.cpu_profiler` to collect JavaScript CPU samples.
Use `contentTracing.getCategories()` to discover available category groups.
Once you know which categories are useful, record only those to reduce the
amount of data collected.

Add [User Timing](https://developer.mozilla.org/en-US/docs/Web/API/Performance_API/User_timing)
marks and measures in your renderer to identify application operations in the
trace. Include `blink.user_timing` in the recording categories, as in the example
above.

```js title='renderer.js'
performance.mark('search-start')
// Run the search operation here.
performance.mark('search-end')
performance.measure('search', 'search-start', 'search-end')
```

## Inspect the trace

Open the [Perfetto UI](https://ui.perfetto.dev/) and select **Open trace file**,
then choose the `.pftrace` file. Expand the process and thread tracks around the
operation you recorded. Search for a User Timing measure such as `search` to
locate the relevant interval.

Select an event to inspect its duration and arguments. Compare the renderer's
activity with the main process at the same point in time. For example, a renderer
waiting for an IPC response may be delayed by a long task in the main process.
An empty interval on one thread alone does not identify the cause of a delay.

After making a change, record the same operation with the same categories and
compare the duration of the application measure. The
[Perfetto UI guide](https://perfetto.dev/docs/visualization/perfetto-ui) describes
timeline navigation, track filtering, and event selection.

## Profile native heap allocations

To investigate native memory use, pass a configuration like this to
`startRecording()` instead of the earlier configuration:

```json
{
  "included_categories": ["disabled-by-default-memory-infra"],
  "nativeHeapProfiling": {
    "dumpInterval": 1000
  }
}
```

The experimental `nativeHeapProfiling` option samples native C/C++ allocations.
`dumpInterval` controls how often snapshots of live sampled allocations are
written to the trace. A value of `0`, the default, writes a single snapshot when
recording stops. See
[`NativeHeapProfilerOptions`](../api/structures/native-heap-profiler-options.md)
for the sampling interval and its overhead tradeoff.

Native allocation profiles serve a different purpose from JavaScript heap
snapshots. To inspect JavaScript objects and their retaining paths, use the
DevTools Memory panel or
[`webContents.takeHeapSnapshot()`](../api/web-contents.md#contentstakeheapsnapshotfilepath).
