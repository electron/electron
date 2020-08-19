import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { deserialize } from '@electron/internal/common/type-utils';

const { nativeImage } = process._linkedBinding('electron_common_native_image');

nativeImage.createThumbnailFromPath = async (path: string, size: Electron.Size) => {
  return deserialize(await ipcRendererInternal.invoke('ELECTRON_NATIVE_IMAGE_CREATE_THUMBNAIL_FROM_PATH', path, size));
};

export default nativeImage;
