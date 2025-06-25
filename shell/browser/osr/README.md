# Offscreen Rendering

## Shared Texture Mode

This section provides a brief summary about how an offscreen frame is generated and how to handle it in native code. This only applies to the GPU-accelerated mode with shared texture, when `webPreferences.offscreen.useSharedTexture` is set to `true`.

### Life of an Offscreen Frame

> This is written at the time of Chromium 134 / Electron 35. The code may change in the future. The following description may not completely reflect the procedure, but it generally describes the process.

#### Initialization

1. Electron JS creates a `BrowserWindow` with `webPreferences.offscreen` set to `true`.
2. Electron C++ creates `OffScreenRenderWidgetHostView`, a subclass of `RenderWidgetHostViewBase`.
3. It instantiates an `OffScreenVideoConsumer`, passing itself as a reference as `view_`.
4. The `OffScreenVideoConsumer` calls `view_->CreateVideoCapturer()`, which makes Chromium code use `HostFrameSinkManager` to communicate with `FrameSinkManagerImpl` (in the Renderer Process) to create a `ClientFrameSinkVideoCapturer` and a `FrameSinkVideoCapturerImpl` (in the Renderer Process). It stores `ClientFrameSinkVideoCapturer` in `video_capturer_`.
5. The `OffScreenVideoConsumer` registers the capture callback to `OffScreenRenderWidgetHostView::OnPaint`.
6. It sets the target FPS, size constraints for capture, and calls `video_capturer_->Start` with the parameter `viz::mojom::BufferFormatPreference::kPreferGpuMemoryBuffer` to enable shared texture mode and start capturing.
7. The `FrameSinkVideoCapturerImpl` accepts `kPreferGpuMemoryBuffer` and creates a `GpuMemoryBufferVideoFramePool` to copy the captured frame. The capacity is `kFramePoolCapacity`, currently `10`, meaning it can capture at most `10` frames if the consumer doesn't consume them in time. It is stored in `frame_pool_`.
8. The `GpuMemoryBufferVideoFramePool` creates a `RenderableGpuMemoryBufferVideoFramePool` using `GmbVideoFramePoolContext` as the context provider, responsible for creating `GpuMemoryBuffer` (or `MappableSharedImage`, as the Chromium team is removing the concept of `GpuMemoryBuffer` and may replace it with `MappableSI` in the future).
9. The `GmbVideoFramePoolContext` initializes itself in both the Renderer Process and GPU Process.

#### Capturing

1. The `FrameSinkVideoCapturerImpl` starts a capture when it receives an `OnFrameDamaged` event or is explicitly requested to refresh the frame. All event sources are evaluated by `VideoCaptureOracle` to see if the capture frequency meets the limit.
2. If a frame is determined to be captured, `FrameSinkVideoCapturerImpl` calls `frame_pool_->ReserveVideoFrame()` to make the pool allocate a frame. The `GmbVideoFramePoolContext` then communicates with the GPU Process to create an actual platform-dependent texture (e.g., `ID3D11Texture2D` or `IOSurface`) that supports being shared across processes.
3. The GPU Process wraps it into a `GpuMemoryBuffer` and sends it back to the Renderer Process, and the pool stores it for further usage.
4. The `FrameSinkVideoCapturerImpl` then uses this allocated (or reused) frame to create a `CopyOutputRequest` and calls `resolved_target_->RequestCopyOfOutput` to copy the frame to the target texture. The `resolved_target_` is a `CapturableFrameSink` that was previously resolved when calling `CreateVideoCapturer` using `OffScreenRenderWidgetHostView`.
5. The GPU Process receives the request and renders the frame to the target texture using the requested format (e.g., `RGBA`). It then sends a completed event to the Renderer Process `FrameSinkVideoCapturerImpl`.
6. The `FrameSinkVideoCapturerImpl` receives the completed event, provides feedback to the `VideoCaptureOracle`, and then calls `frame_pool_->CloneHandleForDelivery` with the captured frame to get a serializable handle to the frame (`HANDLE` or `IOSurfaceRef`). On Windows, it calls `DuplicateHandle` to create a new handle.
7. It then creates a `VideoFrameInfo` with the frame info and the handle and calls `consumer_->OnFrameCaptured` to deliver the frame to the consumer.

#### Consuming

1. `OffScreenVideoConsumer::OnFrameCaptured` is called when the frame is captured. It creates an Electron C++ struct `OffscreenSharedTextureValue` to extract the required info and handle from the callback. It then creates an `OffscreenReleaserHolder` to take ownership of the handle and the mojom remote releaser to prevent releasing.
2. It calls the `callback_` with the `OffscreenSharedTextureValue`, which goes to `OffScreenRenderWidgetHostView::OnPaint`. When shared texture mode is enabled, it directly redirects the callback to an `OnPaintCallback` target set during the initialization of `OffScreenRenderWidgetHostView`, currently set by `OffScreenWebContentsView`, whose `callback_` is also set during initialization. Finally, it goes to `WebContents::OnPaint`.
3. The `WebContents::OnPaint` uses `gin_converter` (`osr_converter.cc`) to convert the `OffscreenSharedTextureValue` to a `v8::Object`. It converts most of the value to a corresponding `v8::Value`, and the handle is converted to a `Buffer`. It also creates a `release` function to destroy the releaser and free the frame we previously took ownership of. The frame can now be reused for further capturing. Finally, it creates a release monitor to detect if the `release` function is called before the garbage collector destroys the JS object; if not, it prints a warning.
4. The data is then emitted to the `paint` event of `webContents`. You can now grab the data and pass it to your native code for further processing. You can pass the `textureInfo` to other processes using Electron IPC, but you can only `release` it in the main process. Do not keep it for long, or it will drain the buffer pool.

### Native Handling

You now have the texture info for the frame. Here's how you should handle it in native code. Suppose you write a node native addon to handle the shared texture.

Retrieve the handle from the `textureInfo.sharedTextureHandle`. You can also read the buffer in JS and use other methods.

```c++
auto textureInfo = args[0];
auto sharedTextureHandle =
    NAPI_GET_PROPERTY_VALUE(textureInfo, "sharedTextureHandle");

size_t handleBufferSize;
uint8_t* handleBufferData;
napi_get_buffer_info(env, sharedTextureHandle,
                      reinterpret_cast<void**>(&handleBufferData),
                      &handleBufferSize);
```

Import the handle to your rendering program.

```c++
// Windows
HANDLE handle = *reinterpret_cast<HANDLE*>(handleBufferData);
Microsoft::WRL::ComPtr<ID3D11Texture2D> shared_texture = nullptr;
HRESULT hr = device1->OpenSharedResource1(handle, IID_PPV_ARGS(&shared_texture)); 

// Extract the texture description
D3D11_TEXTURE2D_DESC desc;
shared_texture->GetDesc(&desc);

// Cache the staging texture if it does not exist or size has changed
if (!cached_staging_texture || cached_width != desc.Width ||
    cached_height != desc.Height) {
  if (cached_staging_texture) {
    cached_staging_texture->Release();
  }

  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.MiscFlags = 0;

  std::cout << "Create staging Texture2D width=" << desc.Width
            << " height=" << desc.Height << std::endl;
  hr = device->CreateTexture2D(&desc, nullptr, &cached_staging_texture);

  cached_width = desc.Width;
  cached_height = desc.Height;
}

// Copy to a intermediate texture
context->CopyResource(cached_staging_texture.Get(), shared_texture.Get());
```

```c++
// macOS
IOSurfaceRef handle = *reinterpret_cast<IOSurfaceRef*>(handleBufferData);

// Assume you have created a GL context.

GLuint io_surface_tex;
glGenTextures(1, &io_surface_tex);
glEnable(GL_TEXTURE_RECTANGLE_ARB);
glBindTexture(GL_TEXTURE_RECTANGLE_ARB, io_surface_tex);

CGLContextObj cgl_context = CGLGetCurrentContext();

GLsizei width = (GLsizei)IOSurfaceGetWidth(io_surface);
GLsizei height = (GLsizei)IOSurfaceGetHeight(io_surface);

CGLTexImageIOSurface2D(cgl_context, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA8, width,
                        height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                        io_surface, 0);

glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

// Copy to a intermediate texture from io_surface_tex
// ...
```

As introduced above, the shared texture is not a single fixed texture that Chromium draws on every frame (CEF before Chromium 103 works that like, please note this significate difference). It is a pool of textures, so every frame Chromium may pass a different texture to you. As soon as you call release, the texture will be reused by Chromium and may cause picture corruption if you keep reading from it. It is also wrong to only open the handle once and directly read from that opened texture on later events. Be very careful if you want to cache the opened texture(s), on Windows, the duplicated handle's value will not be a reliable mapping to the actual underlying texture.

The suggested way is always open the handle on every event, and copy the shared texture to a intermediate texture that you own and release it as soon as possible. You can use the copied texture for further rendering whenever you want. Opening a shared texture should only takes couple microseconds, and you can also use the damaged rect to only copy a portion of the texture to speed up.

You can also refer to these examples:

* [Electron OSR feature test on Windows](https://github.com/electron/electron/blob/e7fa5c709c555bbe248bc98b50a14b5cfa9ea1d9/spec/fixtures/native-addon/osr-gpu/binding_win.cc#L22)
* [CEF open handle on Windows](https://github.com/chromiumembedded/cef/blob/144e01e377249cd614378cb7084a3ed9b06803ca/tests/cefclient/browser/osr_render_handler_win_d3d11.cc#L188)
* [CEF open handle on macOS](https://github.com/chromiumembedded/cef/blob/144e01e377249cd614378cb7084a3ed9b06803ca/tests/cefclient/browser/osr_renderer.cc#L763)
* [CEF copy to texture on macOS](https://github.com/chromiumembedded/cef/blob/144e01e377249cd614378cb7084a3ed9b06803ca/tests/cefclient/browser/osr_renderer.cc#L763)
