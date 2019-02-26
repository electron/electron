'use strict'

// Public-facing API methods.
exports.syncMethods = new Set([
  'getURL',
  'loadURL',
  'getTitle',
  'isLoading',
  'isLoadingMainFrame',
  'isWaitingForResponse',
  'stop',
  'reload',
  'reloadIgnoringCache',
  'canGoBack',
  'canGoForward',
  'canGoToOffset',
  'clearHistory',
  'goBack',
  'goForward',
  'goToIndex',
  'goToOffset',
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
  'paste',
  'pasteAndMatchStyle',
  'delete',
  'selectAll',
  'unselect',
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
  'setZoomLevel'
])

exports.asyncCallbackMethods = new Set([
  'insertCSS',
  'insertText',
  'send',
  'sendInputEvent',
  'setLayoutZoomLevelLimits',
  'setVisualZoomLevelLimits',
  'print'
])

exports.asyncPromiseMethods = new Set([
  'capturePage',
  'executeJavaScript',
  'printToPDF'
])
