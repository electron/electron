/**
 * Create and minimally track guest windows at the direction of the renderer
 * (via window.open). Here, "guest" roughly means "child" — it's not necessarily
 * emblematic of its process status; both in-process (same-origin
 * nativeWindowOpen) and out-of-process (cross-origin nativeWindowOpen and
 * BrowserWindowProxy) are created here. "Embedder" roughly means "parent."
 */
import { BrowserWindow } from 'electron/main';
import type { BrowserWindowConstructorOptions, Referrer, WebContents, LoadURLOptions } from 'electron/main';
import { parseFeatures } from '@electron/internal/common/parse-features-string';
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
  const isNativeWindowOpen = !!guest;
  const { options: browserWindowOptions, additionalFeatures } = makeBrowserWindowOptions({
    embedder,
    features,
    frameName,
    overrideOptions: overrideBrowserWindowOptions
  });

  const didCancelEvent = emitDeprecatedNewWindowEvent({
    event,
    embedder,
    guest,
    browserWindowOptions,
    windowOpenArgs,
    additionalFeatures,
    disposition,
    referrer
  });
  if (didCancelEvent) return;

  // To spec, subsequent window.open calls with the same frame name (`target` in
  // spec parlance) will reuse the previous window.
  // https://html.spec.whatwg.org/multipage/window-object.html#apis-for-creating-and-navigating-browsing-contexts-by-name
  const existingWindow = getGuestWindowByFrameName(frameName);
  if (existingWindow) {
    existingWindow.loadURL(url);
    return existingWindow;
  }

  const window = new BrowserWindow({
    webContents: guest,
    ...browserWindowOptions
  });
  if (!isNativeWindowOpen) {
    // We should only call `loadURL` if the webContents was constructed by us in
    // the case of BrowserWindowProxy (non-sandboxed, nativeWindowOpen: false),
    // as navigating to the url when creating the window from an existing
    // webContents is not necessary (it will navigate there anyway).
    window.loadURL(url, {
      httpReferrer: referrer,
      ...(postData && {
        postData,
        extraHeaders: formatPostDataHeaders(postData)
      })
    });
  }

  handleWindowLifecycleEvents({ embedder, frameName, guest: window });

  embedder.emit('did-create-window', window, { url, frameName, options: browserWindowOptions, disposition, additionalFeatures, referrer, postData });

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
function emitDeprecatedNewWindowEvent ({ event, embedder, guest, windowOpenArgs, browserWindowOptions, additionalFeatures, disposition, referrer, postData }: {
  event: { sender: WebContents, defaultPrevented: boolean },
  embedder: WebContents,
  guest?: WebContents,
  windowOpenArgs: WindowOpenArgs,
  browserWindowOptions: BrowserWindowConstructorOptions,
  additionalFeatures: string[]
  disposition: string,
  referrer: Referrer,
  postData?: PostData,
}): boolean {
  const { url, frameName } = windowOpenArgs;
  const isWebViewWithPopupsDisabled = embedder.getType() === 'webview' && (embedder as any).getLastWebPreferences().disablePopups;
  const postBody = postData ? {
    data: postData,
    headers: formatPostDataHeaders(postData)
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
    additionalFeatures,
    referrer,
    postBody
  );

  const { newGuest } = event as any;
  if (isWebViewWithPopupsDisabled) return true;
  if (event.defaultPrevented) {
    if (newGuest) {
      if (guest === newGuest.webContents) {
        // The webContents is not changed, so set defaultPrevented to false to
        // stop the callers of this event from destroying the webContents.
        (event as any).defaultPrevented = false;
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
  enableRemoteModule: false,
  sandbox: true,
  webviewTag: false,
  nodeIntegrationInSubFrames: false,
  enableWebSQL: false
};

function makeBrowserWindowOptions ({ embedder, features, frameName, overrideOptions, useDeprecatedBehaviorForBareValues = true, useDeprecatedBehaviorForOptionInheritance = true }: {
  embedder: WebContents,
  features: string,
  frameName: string,
  overrideOptions?: BrowserWindowConstructorOptions,
  useDeprecatedBehaviorForBareValues?: boolean
  useDeprecatedBehaviorForOptionInheritance?: boolean
}) {
  const { options: parsedOptions, webPreferences: parsedWebPreferences, additionalFeatures } = parseFeatures(features, useDeprecatedBehaviorForBareValues);

  const deprecatedInheritedOptions = getDeprecatedInheritedOptions(embedder);

  return {
    additionalFeatures,
    options: {
      ...(useDeprecatedBehaviorForOptionInheritance && deprecatedInheritedOptions),
      show: true,
      title: frameName,
      width: 800,
      height: 600,
      ...parsedOptions,
      ...overrideOptions,
      webPreferences: makeWebPreferences({ embedder, insecureParsedWebPreferences: parsedWebPreferences, secureOverrideWebPreferences: overrideOptions && overrideOptions.webPreferences, useDeprecatedBehaviorForOptionInheritance: true })
    }
  };
}

export function makeWebPreferences ({ embedder, secureOverrideWebPreferences = {}, insecureParsedWebPreferences: parsedWebPreferences = {}, useDeprecatedBehaviorForOptionInheritance = true }: {
  embedder: WebContents,
  insecureParsedWebPreferences?: ReturnType<typeof parseFeatures>['webPreferences'],
  // Note that override preferences are considered elevated, and should only be
  // sourced from the main process, as they override security defaults. If you
  // have unvetted prefs, use parsedWebPreferences.
  secureOverrideWebPreferences?: BrowserWindowConstructorOptions['webPreferences'],
  useDeprecatedBehaviorForBareValues?: boolean
  useDeprecatedBehaviorForOptionInheritance?: boolean
}) {
  const deprecatedInheritedOptions = getDeprecatedInheritedOptions(embedder);
  const parentWebPreferences = (embedder as any).getLastWebPreferences();
  const securityWebPreferencesFromParent = Object.keys(securityWebPreferences).reduce((map, key) => {
    if (securityWebPreferences[key] === parentWebPreferences[key]) {
      map[key] = parentWebPreferences[key];
    }
    return map;
  }, {} as any);
  const openerId = parentWebPreferences.nativeWindowOpen ? null : embedder.id;

  return {
    ...(useDeprecatedBehaviorForOptionInheritance && deprecatedInheritedOptions ? deprecatedInheritedOptions.webPreferences : null),
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

/**
 * Current Electron behavior is to inherit all options from the parent window.
 * In practical use, this is kind of annoying because consumers have to know
 * about the parent window's preferences in order to unset them and makes child
 * windows even more of an anomaly. In 11.0.0 we will remove this behavior and
 * only critical security preferences will be inherited by default.
 */
function getDeprecatedInheritedOptions (embedder: WebContents) {
  if (!(embedder as any).browserWindowOptions) {
    // If it's a webview, return just the webPreferences.
    return {
      webPreferences: (embedder as any).getLastWebPreferences()
    };
  }

  const { type, show, ...inheritableOptions } = (embedder as any).browserWindowOptions;
  return inheritableOptions;
}

function formatPostDataHeaders (postData: any) {
  if (!postData) return;

  let extraHeaders = 'content-type: application/x-www-form-urlencoded';

  if (postData.length > 0) {
    const postDataFront = postData[0].bytes.toString();
    const boundary = /^--.*[^-\r\n]/.exec(
      postDataFront
    );
    if (boundary != null) {
      extraHeaders = `content-type: multipart/form-data; boundary=${boundary[0].substr(
        2
      )}`;
    }
  }

  return extraHeaders;
}
