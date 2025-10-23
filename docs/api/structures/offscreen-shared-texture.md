# OffscreenSharedTexture Object

* `textureInfo` Object - The shared texture info.
  * `widgetType` string - The widget type of the texture. Can be `popup` or `frame`.
  * `pixelFormat` string - The pixel format of the texture.
    * `rgba` - The texture format is 8-bit unorm RGBA.
    * `bgra` - The texture format is 8-bit unorm BGRA.
    * `rgbaf16` - The texture format is 16-bit float RGBA.
  * `codedSize` [Size](size.md) - The full dimensions of the video frame.
  * `colorSpace` [ColorSpace](color-space.md) - The color space of the video frame.
  * `visibleRect` [Rectangle](rectangle.md) - A subsection of [0, 0, codedSize.width, codedSize.height]. In OSR case, it is expected to have the full section area.
  * `contentRect` [Rectangle](rectangle.md) - The region of the video frame that capturer would like to populate. In OSR case, it is the same with `dirtyRect` that needs to be painted.
  * `timestamp` number - The time in microseconds since the capture start.
  * `metadata` Object - Extra metadata. See comments in src\media\base\video_frame_metadata.h for accurate details.
    * `captureUpdateRect` [Rectangle](rectangle.md) (optional) - Updated area of frame, can be considered as the `dirty` area.
    * `regionCaptureRect` [Rectangle](rectangle.md) (optional) - May reflect the frame's contents origin if region capture is used internally.
    * `sourceSize` [Rectangle](rectangle.md) (optional) - Full size of the source frame.
    * `frameCount` number (optional) - The increasing count of captured frame. May contain gaps if frames are dropped between two consecutively received frames.
  * `handle` [SharedTextureHandle](shared-texture-handle.md) - The shared texture handle data.
* `release` Function - Release the resources. The `texture` cannot be directly passed to another process, users need to maintain texture lifecycles in
  main process, but it is safe to pass the `textureInfo` to another process. Only a limited number of textures can exist at the same time, so it's important that you call `texture.release()` as soon as you're done with the texture.
