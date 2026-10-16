# webUtils

> A utility layer to interact with Web API objects (Files, Blobs, etc.)

Process: [Renderer](../glossary.md#renderer-process)

> [!IMPORTANT]
> If you want to call this API from a renderer process with context isolation enabled,
> place the API call in your preload script and
> [expose](../tutorial/context-isolation.md#after-context-isolation-enabled) it using the
> [`contextBridge`](context-bridge.md) API.

## Methods

The `webUtils` module has the following methods:

### `webUtils.getPathForFile(file)`

* `file` File - A web [File](https://developer.mozilla.org/en-US/docs/Web/API/File) object.

Returns `string` - The file system path that this `File` object points to. In the case where the object passed in is not a `File` object an exception is thrown. In the case where the File object passed in was constructed in JS and is not backed by a file on disk an empty string is returned.

This method superseded the previous augmentation to the `File` object with the `path` property.  An example is included below.

```js @ts-nocheck
// Before (renderer)
const oldPath = document.querySelector('input[type=file]').files[0].path
```

```js @ts-nocheck
// After

// Renderer:

const file = document.querySelector('input[type=file]').files[0]
electronApi.doSomethingWithFile(file)

// Preload script:

const { contextBridge, webUtils } = require('electron')

contextBridge.exposeInMainWorld('electronApi', {
  doSomethingWithFile(file) {
    const path = webUtils.getPathForFile(file)
    // Do something with the path, e.g., send it over IPC to the main process.
    // It's best not to expose the full file path to the web content if possible.
  }
})
```

### `webUtils.getNativeVideoFrameHandle(frame)` _Experimental_

* `frame` VideoFrame - An open GPU frame with exportable native storage.

Returns `Promise<string>` - An opaque, single-use ID owned by the current document.
Pass it to the main process using your application's IPC. The main process calls
`sharedTexture.resolveVideoFrameHandle(event.senderFrame, id)` to obtain a lease.
OS handles are transferred internally; renderer-local pointers and descriptor
numbers are never sent through application IPC.

This method waits for GPU completion without waiting for a compositor `paint`
event. It performs neither a pixel readback nor a GPU pixel copy. Not every GPU
VideoFrame has native storage: unsupported, closed or CPU frames reject rather
than being copied. The current prototype accepts top-left, unscaled sRGB
RGBA/BGRA native frames only. The optional Chromium feature
`CanvasNativeVideoFrameExport` allocates native 2D canvas storage before drawing.
It is disabled by default.

Closing the input VideoFrame is safe after starting this call. A separate native
reference keeps the source alive until the main process releases its lease.
Unresolved IDs expire after 15 seconds; at most four may be pending per document.
Only one acquisition may be in progress per renderer context. A concurrent call
rejects without cancelling the existing acquisition. This API is available in
frame preloads, not Web Workers.

### `webUtils.releaseNativeVideoFrameHandle(id)` _Experimental_

* `id` string - An unresolved ID returned by `getNativeVideoFrameHandle`.

Releases an unresolved export. Calling this after resolution has no effect on the
main-process lease. Repeated release is harmless.
