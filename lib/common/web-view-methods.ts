export const navigationHistorySyncMethods = new Set([
  'canGoBack',
  'canGoForward',
  'canGoToOffset',
  'clearHistory',
  'goBack',
  'goForward',
  'goToIndex',
  'goToOffset'
]);

// Public-facing API methods.
export const syncMethods = new Set([
  'getURL',
  'getTitle',
  'isLoading',
  'isLoadingMainFrame',
  'isWaitingForResponse',
  'stop',
  'reload',
  'reloadIgnoringCache',
  'isCrashed',
  'setUserAgent',
  'getUserAgent',
  'openDevTools',
  'closeDevTools',
  'isDevToolsOpened',
  'isDevToolsFocused',
  'inspectElement',
  'setAudioMuted',
  'isAudioMuted',
  'isCurrentlyAudible',
  'undo',
  'redo',
  'cut',
  'copy',
  'centerSelection',
  'paste',
  'pasteAndMatchStyle',
  'delete',
  'selectAll',
  'unselect',
  'scrollToTop',
  'scrollToBottom',
  'adjustSelection',
  'replace',
  'replaceMisspelling',
  'findInPage',
  'stopFindInPage',
  'downloadURL',
  'inspectSharedWorker',
  'inspectServiceWorker',
  'showDefinitionForSelection',
  'getZoomFactor',
  'getZoomLevel',
  'setZoomFactor',
  'setZoomLevel',
  ...navigationHistorySyncMethods
]);

export const properties = new Set([
  'audioMuted',
  'userAgent',
  'zoomLevel',
  'zoomFactor',
  'frameRate'
]);

export const asyncMethods = new Set([
  'capturePage',
  'loadURL',
  'executeJavaScript',
  'insertCSS',
  'insertText',
  'removeInsertedCSS',
  'send',
  'sendToFrame',
  'sendInputEvent',
  'setLayoutZoomLevelLimits',
  'setVisualZoomLevelLimits',
  'print',
  'printToPDF'
]);
