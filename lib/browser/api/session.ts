import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
import { desktopCapturer, net } from 'electron/main';
const { fromPartition, fromPath, Session } = process._linkedBinding('electron_browser_session');
const { isDisplayMediaSystemPickerAvailable } = process._linkedBinding('electron_browser_desktop_capturer');

async function getNativePickerSource (preferredDisplaySurface: string) {
  if (process.platform !== 'darwin') {
    throw new Error('Native system picker option is currently only supported on MacOS');
  }

  if (!isDisplayMediaSystemPickerAvailable) {
    throw new Error(`Native system picker unavailable. 
      Note: This is an experimental API; please check the API documentation for updated restrictions`);
  }

  let types: Electron.SourcesOptions["types"];
  switch (preferredDisplaySurface) {
    case 'no_preference':
      types = ['screen', 'window']
      break;
    case 'monitor':
      types = ['screen']
      break;
    case 'window':
      types = ['window']
      break;
    default:
      types = ['screen', 'window']
  }

  // Pass in the needed options for a more native experience
  // screen & windows by default, no thumbnails, since the native picker doesn't return them
  const options: Electron.SourcesOptions = {
    types,
    thumbnailSize: { width: 0, height: 0 },
    fetchWindowIcons: false
  };

  const mediaStreams = await desktopCapturer.getSources(options);
  return mediaStreams[0];
}

Session.prototype.fetch = function (input: RequestInfo, init?: RequestInit) {
  return fetchWithSession(input, init, this, net.request);
};

Session.prototype.setDisplayMediaRequestHandler = function (handler, opts) {
  if (!handler) return this._setDisplayMediaRequestHandler(handler, opts);

  this._setDisplayMediaRequestHandler(async (request, callback) => {
    if (opts && opts.useSystemPicker && isDisplayMediaSystemPickerAvailable()) {
      return callback({ video: await getNativePickerSource(request.preferredDisplaySurface) });
    }

    return handler(request, callback);
  }, opts);
};

export default {
  fromPartition,
  fromPath,
  get defaultSession () {
    return fromPartition('');
  }
};
