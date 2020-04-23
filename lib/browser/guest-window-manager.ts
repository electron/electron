import {
  PostData,
  BrowserWindow,
  WebContentsInternal,
  Referrer,
  LoadURLOptions,
  BrowserWindowConstructorOptionsInternal
} from 'electron';

const hasProp = {}.hasOwnProperty;
const frameToGuest = new Map();

// Security options that child windows will always inherit from parent windows
const inheritedWebPreferences = new Map([
  ['contextIsolation', true],
  ['javascript', false],
  ['nativeWindowOpen', true],
  ['nodeIntegration', false],
  ['enableRemoteModule', false],
  ['sandbox', true],
  ['webviewTag', false],
  ['nodeIntegrationInSubFrames', false]
]);

// Copy attribute of |parent| to |child| if it is not defined in |child|.
const mergeOptions = function (child: any, parent: any, visited?: Set<any>) {
  // Check for circular reference.
  if (visited == null) visited = new Set();
  if (visited.has(parent)) return;

  visited.add(parent);
  for (const key in parent) {
    if (key === 'type') continue;
    if (!hasProp.call(parent, key)) continue;
    if (key in child && key !== 'webPreferences') continue;

    const value = parent[key];
    if (typeof value === 'object' && !Array.isArray(value)) {
      child[key] = mergeOptions(child[key] || {}, value, visited);
    } else {
      child[key] = value;
    }
  }
  visited.delete(parent);

  return child;
};

// Merge |options| with the |embedder|'s window's options.
const mergeBrowserWindowOptions = function (embedder: any, options: any) {
  if (options.webPreferences == null) {
    options.webPreferences = {};
  }
  if (embedder.browserWindowOptions != null) {
    let parentOptions = embedder.browserWindowOptions;

    // if parent's visibility is available, that overrides 'show' flag (#12125)
    const win = BrowserWindow.fromWebContents(embedder.webContents);
    if (win != null) {
      parentOptions = {
        ...win.getBounds(),
        ...embedder.browserWindowOptions,
        show: win.isVisible()
      };
    }

    // Inherit the original options if it is a BrowserWindow.
    mergeOptions(options, parentOptions);
  } else {
    // Or only inherit webPreferences if it is a webview.
    mergeOptions(options.webPreferences, embedder.getLastWebPreferences());
  }

  // Inherit certain option values from parent window
  const webPreferences = embedder.getLastWebPreferences();
  for (const [name, value] of inheritedWebPreferences) {
    if (webPreferences[name] === value) {
      options.webPreferences[name] = value;
    }
  }

  if (!webPreferences.nativeWindowOpen) {
    // Sets correct openerId here to give correct options to 'new-window' event handler
    options.webPreferences.openerId = embedder.id;
  }

  return options;
};

const MULTIPART_CONTENT_TYPE = 'multipart/form-data';
const URL_ENCODED_CONTENT_TYPE = 'application/x-www-form-urlencoded';
function makeContentTypeHeader ({
  contentType,
  boundary
}: {
  contentType: string
  boundary?: string
}) {
  const header = `content-type: ${contentType};`;
  if (contentType === MULTIPART_CONTENT_TYPE) {
    return `${header} boundary=${boundary}`;
  }
  return header;
}

// Figure out appropriate headers for post data.
const parseContentTypeFormat = function (postData: PostData[]) {
  if (postData.length) {
    // For multipart forms, the first element will start with the boundary
    // notice, which looks something like `------WebKitFormBoundary12345678`
    // Note, this regex would fail when submitting a urlencoded form with an
    // input attribute of name="--theKey", but, uhh, don't do that?
    const postDataFront = postData[0].bytes!.toString();
    const boundary = /^--.*[^-\r\n]/.exec(postDataFront);
    if (boundary) {
      return {
        boundary: boundary[0].substr(2),
        contentType: MULTIPART_CONTENT_TYPE
      };
    }
  }
  // Either the form submission didn't contain any inputs (the postData array
  // was empty), or we couldn't find the boundary and thus we can assume this is
  // a key=value style form.
  return {
    contentType: URL_ENCODED_CONTENT_TYPE
  };
};

// Setup a new guest with |embedder|
const setupGuest = function (
  embedder: WebContentsInternal,
  frameName: string,
  guest: BrowserWindow
) {
  // When |embedder| is destroyed we should also destroy attached guest, and if
  // guest is closed by user then we should prevent |embedder| from double
  // closing guest.
  const guestId = guest.webContents.id;
  const closedByEmbedder = function () {
    guest.removeListener('closed', closedByUser);
    guest.destroy();
  };
  const closedByUser = function () {
    embedder._sendInternal(
      'ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_' + guestId
    );
    embedder.removeListener('current-render-view-deleted' as any, closedByEmbedder);
  };
  embedder.once('current-render-view-deleted' as any, closedByEmbedder);
  guest.once('closed', closedByUser);
  if (frameName) {
    frameToGuest.set(frameName, guest);
    (guest as any).frameName = frameName;
    guest.once('closed', function () {
      frameToGuest.delete(frameName);
    });
  }
  return guestId;
};

// Create a new guest created by |embedder| with |options|.
const createGuest = function (
  embedder: WebContentsInternal,
  url: string,
  referrer: Referrer,
  frameName: string,
  options: BrowserWindowConstructorOptionsInternal,
  postData: PostData[] | null
) {
  let guest = frameToGuest.get(frameName);
  if (frameName && guest != null) {
    guest.loadURL(url);
    return guest.webContents.id;
  }

  // Remember the embedder window's id.
  if (options.webPreferences == null) {
    options.webPreferences = {};
  }

  guest = new BrowserWindow(options);
  if (!options.webContents) {
    // We should not call `loadURL` if the window was constructed from an
    // existing webContents (window.open in a sandboxed renderer).
    //
    // Navigating to the url when creating the window from an existing
    // webContents is not necessary (it will navigate there anyway).
    const loadOptions: LoadURLOptions = {
      httpReferrer: referrer
    };
    if (postData != null) {
      loadOptions.postData = postData as any;
      loadOptions.extraHeaders = makeContentTypeHeader(
        parseContentTypeFormat(postData)
      );
    }
    guest.loadURL(url, loadOptions);
  }

  return setupGuest(embedder, frameName, guest);
};

// Routed window.open messages with fully parsed options
export function internalWindowOpen (
  event: ElectronInternal.IpcMainInternalEvent,
  url: string,
  referrer: Referrer,
  frameName: string,
  disposition: string,
  options: BrowserWindowConstructorOptionsInternal,
  additionalFeatures: string[],
  postData: PostData[] | null
) {
  options = mergeBrowserWindowOptions(event.sender, options);
  const postBody = postData
    ? {
      data: postData,
      ...parseContentTypeFormat(postData)
    }
    : null;

  event.sender.emit(
    'new-window',
    event,
    url,
    frameName,
    disposition,
    options,
    additionalFeatures,
    referrer,
    postBody
  );
  const { newGuest } = event as any;
  if (
    (event.sender.getType() === 'webview' &&
      (event.sender as any).getLastWebPreferences().disablePopups) ||
    event.defaultPrevented
  ) {
    if (newGuest != null) {
      if (options.webContents === newGuest.webContents) {
        // the webContents is not changed, so set defaultPrevented to false to
        // stop the callers of this event from destroying the webContents.
        (event as any).defaultPrevented = false;
      }
      event.returnValue = setupGuest(event.sender as WebContentsInternal, frameName, newGuest);
    } else {
      event.returnValue = null;
    }
  } else {
    event.returnValue = createGuest(
      event.sender as WebContentsInternal,
      url,
      referrer,
      frameName,
      options,
      postData
    );
  }
}
