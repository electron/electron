const { createDesktopCapturer } = process._linkedBinding('electron_browser_desktop_capturer');

const deepEqual = (a: ElectronInternal.GetSourcesOptions, b: ElectronInternal.GetSourcesOptions) => JSON.stringify(a) === JSON.stringify(b);

let currentlyRunning: {
  options: ElectronInternal.GetSourcesOptions;
  getSources: Promise<ElectronInternal.GetSourcesResult[]>;
}[] = [];

// |options.types| can't be empty and must be an array
function isValid (options: Electron.SourcesOptions) {
  const types = options ? options.types : undefined;
  return Array.isArray(types);
}

export async function getSources (args: Electron.SourcesOptions) {
  if (!isValid(args)) throw new Error('Invalid options');

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
