import { BrowserWindow } from 'electron/main';

const { createDesktopCapturer, isDisplayMediaSystemPickerAvailable } = process._linkedBinding(
  'electron_browser_desktop_capturer'
);

const deepEqual = (a: ElectronInternal.GetSourcesOptions, b: ElectronInternal.GetSourcesOptions) =>
  JSON.stringify(a) === JSON.stringify(b);

let currentlyRunning: {
  options: ElectronInternal.GetSourcesOptions;
  getSources: Promise<ElectronInternal.GetSourcesResult[]>;
}[] = [];

// |options.types| can't be empty and must be an array
function isValid(options: Electron.SourcesOptions) {
  return Array.isArray(options?.types);
}

export { isDisplayMediaSystemPickerAvailable };

export async function getSources(args: Electron.SourcesOptions) {
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

  let resolveGetSources!: (value: ElectronInternal.GetSourcesResult[]) => void;
  let rejectGetSources!: (reason?: any) => void;

  const getSources = new Promise<ElectronInternal.GetSourcesResult[]>((resolve, reject) => {
    resolveGetSources = resolve;
    rejectGetSources = reject;
  });

  // Register in currentlyRunning BEFORE startHandling so that synchronous
  // completion (e.g. when no capturers are created) can properly clean up.
  currentlyRunning.push({ options, getSources });

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
        }
      }
    }
    // Remove from currentlyRunning once we resolve or reject
    currentlyRunning = currentlyRunning.filter((running) => running.options !== options);
  };

  capturer._onerror = (error: string) => {
    stopRunning();
    // eslint-disable-next-line prefer-promise-reject-errors
    rejectGetSources(error);
  };

  capturer._onfinished = (sources: Electron.DesktopCapturerSource[]) => {
    stopRunning();
    resolveGetSources(sources);
  };

  capturer.startHandling(captureWindow, captureScreen, thumbnailSize, fetchWindowIcons);

  return getSources;
}
