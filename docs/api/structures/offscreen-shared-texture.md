# OffscreenSharedTexture Object

* `textureInfo` Object - The shared texture info.
  * `widgetType` string - The widget type of the texture. Can be `popup` or `frame`.
  * `pixelFormat` string - The pixel format of the texture. Can be `rgba` or `bgra`.
  * `codedSize` [Size](size.md) - The full dimensions of the video frame.
  * `visibleRect` [Rectangle](rectangle.md) - A subsection of [0, 0, codedSize.width(), codedSize.height()]. In OSR case, it is expected to have the full section area.
  * `contentRect` [Rectangle](rectangle.md) - The region of the video frame that capturer would like to populate. In OSR case, it is the same with `dirtyRect` that needs to be painted.
  * `timestamp` number - The time in microseconds since the capture start.
  * `metadata` Object - Extra metadata. See comments in src\media\base\video_frame_metadata.h for accurate details.
    * `captureUpdateRect` [Rectangle](rectangle.md) (optional) - Updated area of frame, can be considered as the `dirty` area.
    * `regionCaptureRect` [Rectangle](rectangle.md) (optional) - May reflect the frame's contents origin if region capture is used internally.
    * `sourceSize` [Rectangle](rectangle.md) (optional) - Full size of the source frame.
    * `frameCount` number (optional) - The increasing count of captured frame. May contain gaps if frames are dropped between two consecutively received frames.
  * `sharedTextureHandle` Buffer _Windows_ _macOS_ - The handle to the shared texture.
  * `planes` Object[] _Linux_ - Each plane's info of the shared texture.
    * `stride` number - The strides and offsets in bytes to be used when accessing the buffers via a memory mapping. One per plane per entry.
    * `offset` number - The strides and offsets in bytes to be used when accessing the buffers via a memory mapping. One per plane per entry.
    * `size` number - Size in bytes of the plane. This is necessary to map the buffers.
    * `fd` number - File descriptor for the underlying memory object (usually dmabuf).
  * `modifier` string _Linux_ - The modifier is retrieved from GBM library and passed to EGL driver.
* `release` Function - Release the resources. The `texture` cannot be directly passed to another process, users need to maintain texture lifecycles in
  main process, but it is safe to pass the `textureInfo` to another process.
