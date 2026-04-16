import { BrowserWindow } from 'electron/main';

export interface TabSourceCaptureResult {
  ok: boolean;
  href: string;
  message?: string;
  origin: string;
  videoTrackCount?: number;
}

export const captureWithTabSourceId = async (
  requestingWindow: BrowserWindow,
  streamId: string
): Promise<TabSourceCaptureResult> => {
  return requestingWindow.webContents.executeJavaScript(`
    navigator.mediaDevices.getUserMedia({
      video: {
        mandatory: {
          chromeMediaSource: 'tab',
          chromeMediaSourceId: ${JSON.stringify(streamId)},
        },
      },
    }).then((stream) => {
      const result = {
        ok: stream instanceof MediaStream,
        href: location.href,
        origin: location.origin,
        videoTrackCount: stream.getVideoTracks().length,
      };
      stream.getTracks().forEach((track) => track.stop());
      return result;
    }, (error) => ({
      ok: false,
      href: location.href,
      message: error.message,
      origin: location.origin,
    }));
  `);
};
