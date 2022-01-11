import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { internalContextBridge } from '@electron/internal/renderer/api/context-bridge';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const { contextIsolationEnabled } = internalContextBridge;

export const windowSetup = (isWebView: boolean, isHiddenPage: boolean) => {
  if (!process.sandboxed && !isWebView) {
    // Override default window.close.
    window.close = function () {
      ipcRendererInternal.send(IPC_MESSAGES.BROWSER_WINDOW_CLOSE);
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['close'], window.close);
  }

  // But we do not support prompt().
  window.prompt = function () {
    throw new Error('prompt() is and will not be supported.');
  };
  if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['prompt'], window.prompt);

  if (isWebView) {
    // Webview `document.visibilityState` tracks window visibility (and ignores
    // the actual <webview> element visibility) for backwards compatibility.
    // See discussion in #9178.
    //
    // Note that this results in duplicate visibilitychange events (since
    // Chromium also fires them) and potentially incorrect visibility change.
    // We should reconsider this decision for Electron 2.0.
    let cachedVisibilityState = isHiddenPage ? 'hidden' : 'visible';

    // Subscribe to visibilityState changes.
    ipcRendererInternal.on(IPC_MESSAGES.GUEST_INSTANCE_VISIBILITY_CHANGE, function (_event, visibilityState: VisibilityState) {
      if (cachedVisibilityState !== visibilityState) {
        cachedVisibilityState = visibilityState;
        document.dispatchEvent(new Event('visibilitychange'));
      }
    });

    // Make document.hidden and document.visibilityState return the correct value.
    const getDocumentHidden = () => cachedVisibilityState !== 'visible';
    Object.defineProperty(document, 'hidden', {
      get: getDocumentHidden
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['document', 'hidden'], getDocumentHidden);

    const getDocumentVisibilityState = () => cachedVisibilityState;
    Object.defineProperty(document, 'visibilityState', {
      get: getDocumentVisibilityState
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['document', 'visibilityState'], getDocumentVisibilityState);
  }
};
