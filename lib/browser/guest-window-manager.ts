/**
 * Create and minimally track guest windows at the direction of the renderer
 * (via window.open). Here, "guest" roughly means "child" — it's not necessarily
 * emblematic of its process status; both in-process (same-origin
 * nativeWindowOpen) and out-of-process (cross-origin nativeWindowOpen and
 * BrowserWindowProxy) are created here. "Embedder" roughly means "parent."
 */
import { BrowserWindow } from 'electron/main';
import type { BrowserWindowConstructorOptions, Referrer, WebContents, LoadURLOptions } from 'electron/main';
import { parseFeatures } from '@electron/internal/browser/parse-features-string';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

type PostData = LoadURLOptions['postData']
export type WindowOpenArgs = {
  url: string,
  frameName: string,
  features: string,
}

const frameNamesToWindow = new Map<string, BrowserWindow>();
const registerFrameNameToGuestWindow = (name: string, win: BrowserWindow) => frameNamesToWindow.set(name, win);
const unregisterFrameName = (name: string) => frameNamesToWindow.delete(name);
const getGuestWindowByFrameName = (name: string) => frameNamesToWindow.get(name);

/**
 * `openGuestWindow` is called for both implementations of window.open
 * (BrowserWindowProxy and nativeWindowOpen) to create and setup event handling
 * for the new window.
 *
 * Until its removal in 12.0.0, the `new-window` event is fired, allowing the
 * user to preventDefault() on the passed event (which ends up calling
 * DestroyWebContents in the nativeWindowOpen code path).
 */
export function openGuestWindow ({ event, embedder, guest, referrer, disposition, postData, overrideBrowserWindowOptions, windowOpenArgs }: {
  event: { sender: WebContents, defaultPrevented: boolean },
  embedder: WebContents,
  guest?: WebContents,
  referrer: Referrer,
  disposition: string,
  postData?: PostData,
  overrideBrowserWindowOptions?: BrowserWindowConstructorOptions,
  windowOpenArgs: WindowOpenArgs,
}): BrowserWindow | undefined {
  const { url, frameName, features } = windowOpenArgs;
  const { options: browserWindowOptions } = makeBrowserWindowOptions({
    embedder,
    features,
    overrideOptions: overrideBrowserWindowOptions
  });

  const didCancelEvent = emitDeprecatedNewWindowEvent({
    event,
    embedder,
    guest,
    browserWindowOptions,
    windowOpenArgs,
    disposition,
    postData,
    referrer
  });
  if (didCancelEvent) return;

  // To spec, subsequent window.open calls with the same frame name (`target` in
  // spec parlance) will reuse the previous window.
  // https://html.spec.whatwg.org/multipage/window-object.html#apis-for-creating-and-navigating-browsing-contexts-by-name
  const existingWindow = getGuestWindowByFrameName(frameName);
  if (existingWindow) {
    if (existingWindow.isDestroyed() || existingWindow.webContents.isDestroyed()) {
      // FIXME(t57ser): The webContents is destroyed for some reason, unregister the frame name
      unregisterFrameName(frameName);
    } else {
      existingWindow.loadURL(url);
      return existingWindow;
    }
  }

  const window = new BrowserWindow({
    webContents: guest,
    ...browserWindowOptions
  });
  if (!guest) {
    // We should only call `loadURL` if the webContents was constructed by us in
    // the case of BrowserWindowProxy (non-sandboxed, nativeWindowOpen: false),
    // as navigating to the url when creating the window from an existing
    // webContents is not necessary (it will navigate there anyway).
    // This can also happen if we enter this function from OpenURLFromTab, in
    // which case the browser process is responsible for initiating navigation
    // in the new window.
    window.loadURL(url, {
      httpReferrer: referrer,
      ...(postData && {
        postData,
        extraHeaders: formatPostDataHeaders(postData as Electron.UploadRawData[])
      })
    });
  }

  handleWindowLifecycleEvents({ embedder, frameName, guest: window });

  embedder.emit('did-create-window', window, { url, frameName, options: browserWindowOptions, disposition, referrer, postData });

  return window;
}

/**
 * Manage the relationship between embedder window and guest window. When the
 * guest is destroyed, notify the embedder. When the embedder is destroyed, so
 * too is the guest destroyed; this is Electron convention and isn't based in
 * browser behavior.
 */
const handleWindowLifecycleEvents = function ({ embedder, guest, frameName }: {
  embedder: WebContents,
  guest: BrowserWindow,
  frameName: string
}) {
  const closedByEmbedder = function () {
    guest.removeListener('closed', closedByUser);
    guest.destroy();
  };

  const cachedGuestId = guest.webContents.id;
  const closedByUser = function () {
    embedder._sendInternal(`${IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_CLOSED}_${cachedGuestId}`);
    embedder.removeListener('current-render-view-deleted' as any, closedByEmbedder);
  };
  embedder.once('current-render-view-deleted' as any, closedByEmbedder);
  guest.once('closed', closedByUser);

  if (frameName) {
    registerFrameNameToGuestWindow(frameName, guest);
    guest.once('closed', function () {
      unregisterFrameName(frameName);
    });
  }
};

/**
 * Deprecated in favor of `webContents.setWindowOpenHandler` and
 * `did-create-window` in 11.0.0. Will be removed in 12.0.0.
 */
function emitDeprecatedNewWindowEvent ({ event, embedder, guest, windowOpenArgs, browserWindowOptions, disposition, referrer, postData }: {
  event: { sender: WebContents, defaultPrevented: boolean, newGuest?: BrowserWindow },
  embedder: WebContents,
  guest?: WebContents,
  windowOpenArgs: WindowOpenArgs,
  browserWindowOptions: BrowserWindowConstructorOptions,
  disposition: string,
  referrer: Referrer,
  postData?: PostData,
}): boolean {
  const { url, frameName } = windowOpenArgs;
  const isWebViewWithPopupsDisabled = embedder.getType() === 'webview' && embedder.getLastWebPreferences()!.disablePopups;
  const postBody = postData ? {
    data: postData,
    ...parseContentTypeFormat(postData)
  } : null;

  embedder.emit(
    'new-window',
    event,
    url,
    frameName,
    disposition,
    {
      ...browserWindowOptions,
      webContents: guest
    },
    [], // additionalFeatures
    referrer,
    postBody
  );

  const { newGuest } = event;
  if (isWebViewWithPopupsDisabled) return true;
  if (event.defaultPrevented) {
    if (newGuest) {
      if (guest === newGuest.webContents) {
        // The webContents is not changed, so set defaultPrevented to false to
        // stop the callers of this event from destroying the webContents.
        event.defaultPrevented = false;
      }

      handleWindowLifecycleEvents({
        embedder: event.sender,
        guest: newGuest,
        frameName
      });
    }
    return true;
  }
  return false;
}

// Security options that child windows will always inherit from parent windows
const securityWebPreferences: { [key: string]: boolean } = {
  contextIsolation: true,
  javascript: false,
  nativeWindowOpen: true,
  nodeIntegration: false,
  sandbox: true,
  webviewTag: false,
  nodeIntegrationInSubFrames: false,
  enableWebSQL: false
};

function makeBrowserWindowOptions ({ embedder, features, overrideOptions }: {
  embedder: WebContents,
  features: string,
  overrideOptions?: BrowserWindowConstructorOptions,
}) {
  const { options: parsedOptions, webPreferences: parsedWebPreferences } = parseFeatures(features);

  return {
    options: {
      show: true,
      width: 800,
      height: 600,
      ...parsedOptions,
      ...overrideOptions,
      webPreferences: makeWebPreferences({
        embedder,
        insecureParsedWebPreferences: parsedWebPreferences,
        secureOverrideWebPreferences: overrideOptions && overrideOptions.webPreferences
      })
    } as Electron.BrowserViewConstructorOptions
  };
}

export function makeWebPreferences ({ embedder, secureOverrideWebPreferences = {}, insecureParsedWebPreferences: parsedWebPreferences = {} }: {
  embedder: WebContents,
  insecureParsedWebPreferences?: ReturnType<typeof parseFeatures>['webPreferences'],
  // Note that override preferences are considered elevated, and should only be
  // sourced from the main process, as they override security defaults. If you
  // have unvetted prefs, use parsedWebPreferences.
  secureOverrideWebPreferences?: BrowserWindowConstructorOptions['webPreferences'],
}) {
  const parentWebPreferences = embedder.getLastWebPreferences()!;
  const securityWebPreferencesFromParent = (Object.keys(securityWebPreferences).reduce((map, key) => {
    if (securityWebPreferences[key] === parentWebPreferences[key as keyof Electron.WebPreferences]) {
      (map as any)[key] = parentWebPreferences[key as keyof Electron.WebPreferences];
    }
    return map;
  }, {} as Electron.WebPreferences));
  const openerId = parentWebPreferences.nativeWindowOpen ? null : embedder.id;

  return {
    ...parsedWebPreferences,
    // Note that order is key here, we want to disallow the renderer's
    // ability to change important security options but allow main (via
    // setWindowOpenHandler) to change them.
    ...securityWebPreferencesFromParent,
    ...secureOverrideWebPreferences,
    // Sets correct openerId here to give correct options to 'new-window' event handler
    // TODO: Figure out another way to pass this?
    openerId
  };
}

function formatPostDataHeaders (postData: PostData) {
  if (!postData) return;

  const { contentType, boundary } = parseContentTypeFormat(postData);
  if (boundary != null) { return `content-type: ${contentType}; boundary=${boundary}`; }

  return `content-type: ${contentType}`;
}

const MULTIPART_CONTENT_TYPE = 'multipart/form-data';
const URL_ENCODED_CONTENT_TYPE = 'application/x-www-form-urlencoded';

// Figure out appropriate headers for post data.
export const parseContentTypeFormat = function (postData: Exclude<PostData, undefined>) {
  if (postData.length) {
    if (postData[0].type === 'rawData') {
      // For multipart forms, the first element will start with the boundary
      // notice, which looks something like `------WebKitFormBoundary12345678`
      // Note, this regex would fail when submitting a urlencoded form with an
      // input attribute of name="--theKey", but, uhh, don't do that?
      const postDataFront = postData[0].bytes.toString();
      const boundary = /^--.*[^-\r\n]/.exec(postDataFront);
      if (boundary) {
        return {
          boundary: boundary[0].substr(2),
          contentType: MULTIPART_CONTENT_TYPE
        };
      }
    }
  }
  // Either the form submission didn't contain any inputs (the postData array
  // was empty), or we couldn't find the boundary and thus we can assume this is
  // a key=value style form.
  return {
    contentType: URL_ENCODED_CONTENT_TYPE
  };
};
