import { BrowserWindow } from 'electron/main';
const { createDesktopCapturer, isDisplayMediaSystemPickerAvailable } = process._linkedBinding('electron_browser_desktop_capturer');

const deepEqual = (a: ElectronInternal.GetSourcesOptions, b: ElectronInternal.GetSourcesOptions) => JSON.stringify(a) === JSON.stringify(b);

let currentlyRunning: {
  options: ElectronInternal.GetSourcesOptions;
  getSources: Promise<ElectronInternal.GetSourcesResult[]>;
}[] = [];

// |options.types| can't be empty and must be an array
function isValid (options: Electron.SourcesOptions) {
  return Array.isArray(options?.types);
}

export { isDisplayMediaSystemPickerAvailable };

export async function getNativePickerSource () {
  if (process.platform !== 'darwin') {
    throw new Error('Native system picker option is currently only supported on MacOS');
  }

  if (!isDisplayMediaSystemPickerAvailable) {
    throw new Error(`Native system picker unavailable. 
      Note: This is an experimental API; please check the API documentation for updated restrictions`);
  }

  // Pass in the needed options for a more native experience
  // screen & windows by default, no thumbnails, since the native picker doesn't return them
  const options: Electron.SourcesOptions = {
    types: ['screen', 'window'],
    thumbnailSize: { width: 0, height: 0 },
    fetchWindowIcons: false
  };

  return await getSources(options);
}

export async function getSources (args: Electron.SourcesOptions) {
  if (!isValid(args)) throw new Error('Invalid options');

  const resizableValues = new Map();
  if (process.platform === 'darwin') {
    // Fix for bug in ScreenCaptureKit that modifies a window's styleMask the first time
    // it captures a non-resizable window. We record each non-resizable window's styleMask,
    // and we restore modified styleMasks later, after the screen capture.
    for (const win of BrowserWindow.getAllWindows()) {
      resizableValues.set([win.id], win.resizable);
    }
  }

  const captureWindow = args.types.includes('window');
  const captureScreen = args.types.includes('screen');

  const { thumbnailSize = { width: 150, height: 150 } } = args;
  const { fetchWindowIcons = false } = args;

  const options = {
    captureWindow,
    captureScreen,
    thumbnailSize,
    fetchWindowIcons
  };

  for (const running of currentlyRunning) {
    if (deepEqual(running.options, options)) {
      // If a request is currently running for the same options
      // return that promise
      return running.getSources;
    }
  }

  const getSources = new Promise<ElectronInternal.GetSourcesResult[]>((resolve, reject) => {
    let capturer: ElectronInternal.DesktopCapturer | null = createDesktopCapturer();

    const stopRunning = () => {
      if (capturer) {
        delete capturer._onerror;
        delete capturer._onfinished;
        capturer = null;

        if (process.platform === 'darwin') {
          for (const win of BrowserWindow.getAllWindows()) {
            if (resizableValues.has(win.id)) {
              win.resizable = resizableValues.get(win.id);
            }
          };
        }
      }
      // Remove from currentlyRunning once we resolve or reject
      currentlyRunning = currentlyRunning.filter(running => running.options !== options);
    };

    capturer._onerror = (error: string) => {
      stopRunning();
      reject(error);
    };

    capturer._onfinished = (sources: Electron.DesktopCapturerSource[]) => {
      stopRunning();
      resolve(sources);
    };

    capturer.startHandling(captureWindow, captureScreen, thumbnailSize, fetchWindowIcons);
  });

  currentlyRunning.push({
    options,
    getSources
  });

  return getSources;
}
