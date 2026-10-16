# NativeVideoFrameLease Object

* `textureInfo` [SharedTextureImportTextureInfo](shared-texture-import-texture-info.md) - Native texture metadata and the main-process OS handle.
* `release` Function\<Promise\<void\>\> - Releases the native handle and source frame reference. Resolves after the source reference is released in the renderer. Idempotent. Call only after the native consumer has finished reading.

The handle remains valid while this lease is retained and has not been released.
Garbage collection releases it as well, but deterministic native consumers must
call `release()`. No compositor capture or pixel copy is performed.
