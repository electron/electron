export const enum WEB_VIEW_ATTRIBUTES {
  NAME = 'name',
  PARTITION = 'partition',
  SRC = 'src',
  HTTPREFERRER = 'httpreferrer',
  NODEINTEGRATION = 'nodeintegration',
  NODEINTEGRATIONINSUBFRAMES = 'nodeintegrationinsubframes',
  PLUGINS = 'plugins',
  DISABLEWEBSECURITY = 'disablewebsecurity',
  ALLOWPOPUPS = 'allowpopups',
  PRELOAD = 'preload',
  USERAGENT = 'useragent',
  BLINKFEATURES = 'blinkfeatures',
  DISABLEBLINKFEATURES = 'disableblinkfeatures',
  WEBPREFERENCES = 'webpreferences',
}

export const enum WEB_VIEW_ERROR_MESSAGES {
  // Error messages.
  ALREADY_NAVIGATED = 'The object has already navigated, so its partition cannot be changed.',
  INVALID_PARTITION_ATTRIBUTE = 'Invalid partition attribute.',
  INVALID_PRELOAD_ATTRIBUTE = 'Only "file:" protocol is supported in "preload" attribute.',
  NOT_ATTACHED = 'The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.'
}
