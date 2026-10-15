import { sharedTexture } from 'electron';
import { webUtils } from 'electron/renderer';

declare const frame: VideoFrame;
declare const sender: Electron.WebFrameMain;

export async function nativeVideoFrameTypes() {
  const id: string = await webUtils.getNativeVideoFrameHandle(frame);
  const lease: Electron.NativeVideoFrameLease = sharedTexture.resolveVideoFrameHandle(sender, id);
  const textureInfo: Electron.SharedTextureImportTextureInfo = lease.textureInfo;
  sharedTexture.importSharedTexture({ textureInfo });
  const released: Promise<void> = lease.release();
  await released;
  webUtils.releaseNativeVideoFrameHandle(id);
}
