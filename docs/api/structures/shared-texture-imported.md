# SharedTextureImported Object

* `textureId` string - The unique identifier of this shared texture imported.
* `getVideoFrame` Function\<[VideoFrame](https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame)\> - Create a `VideoFrame` that use the imported shared texture at current process. You can call `VideoFrame.close()` once you've finished using, the underlying resources will wait for GPU finish internally.
* `release` Function - Release this object's reference of this shared texture imported. The underlying resource will be alive until every reference released.
* `subtle` [SharedTextureImportedSubtle](shared-texture-imported-subtle.md) - Provides subtle APIs to interact with imported shared texture for advanced users.
