type Sanitizer = (obj: Record<string, any>) => Record<string, any>;

function makeSanitizer (names: string[]): Sanitizer {
  return (obj: Record<string, any>) => {
    const ret = { ...obj };
    for (const name of names) {
      delete ret[name];
    }
    return ret;
  };
}

export const webViewEvents: Record<string, readonly (string | readonly [string, Sanitizer])[]> = {
  'load-commit': ['url', 'isMainFrame'],
  'did-attach': [],
  'did-finish-load': [],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-frame-finish-load': ['isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'dom-ready': [],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'context-menu': [['params', makeSanitizer(['frame'])]],
  'new-window': ['url', 'frameName', 'disposition', ['options', makeSanitizer(['webContents'])]],
  'devtools-opened': [],
  'devtools-closed': [],
  'devtools-focused': [],
  'will-navigate': ['url'],
  'did-start-navigation': ['url', 'isInPlace', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-redirect-navigation': ['url', 'isInPlace', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-navigate': ['url', 'httpResponseCode', 'httpStatusText'],
  'did-frame-navigate': ['url', 'httpResponseCode', 'httpStatusText', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-navigate-in-page': ['url', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  '-focus-change': ['focus'],
  close: [],
  crashed: [],
  'render-process-gone': ['details'],
  'plugin-crashed': ['name', 'version'],
  destroyed: [],
  'page-title-updated': ['title', 'explicitSet'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'media-started-playing': [],
  'media-paused': [],
  'found-in-page': ['result'],
  'did-change-theme-color': ['themeColor'],
  'update-target-url': ['url']
} as const;
