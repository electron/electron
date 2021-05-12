export const enum WEB_VIEW_CONSTANTS {
  // Attributes.
  ATTRIBUTE_NAME = 'name',
  ATTRIBUTE_PARTITION = 'partition',
  ATTRIBUTE_SRC = 'src',
  ATTRIBUTE_HTTPREFERRER = 'httpreferrer',
  ATTRIBUTE_NODEINTEGRATION = 'nodeintegration',
  ATTRIBUTE_NODEINTEGRATIONINSUBFRAMES = 'nodeintegrationinsubframes',
  ATTRIBUTE_PLUGINS = 'plugins',
  ATTRIBUTE_DISABLEWEBSECURITY = 'disablewebsecurity',
  ATTRIBUTE_ALLOWPOPUPS = 'allowpopups',
  ATTRIBUTE_PRELOAD = 'preload',
  ATTRIBUTE_USERAGENT = 'useragent',
  ATTRIBUTE_BLINKFEATURES = 'blinkfeatures',
  ATTRIBUTE_DISABLEBLINKFEATURES = 'disableblinkfeatures',
  ATTRIBUTE_WEBPREFERENCES = 'webpreferences',

  // Error messages.
  ERROR_MSG_ALREADY_NAVIGATED = 'The object has already navigated, so its partition cannot be changed.',
  ERROR_MSG_INVALID_PARTITION_ATTRIBUTE = 'Invalid partition attribute.',
  ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE = 'Only "file:" protocol is supported in "preload" attribute.',
  ERROR_MSG_NOT_ATTACHED = 'The WebView must be attached to the DOM and the dom-ready event emitted before this method can be called.'
}
