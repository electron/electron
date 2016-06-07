module.exports = {
  // Attributes.
  ATTRIBUTE_AUTOSIZE: 'autosize',
  ATTRIBUTE_MAXHEIGHT: 'maxheight',
  ATTRIBUTE_MAXWIDTH: 'maxwidth',
  ATTRIBUTE_MINHEIGHT: 'minheight',
  ATTRIBUTE_MINWIDTH: 'minwidth',
  ATTRIBUTE_NAME: 'name',
  ATTRIBUTE_PARTITION: 'partition',
  ATTRIBUTE_SRC: 'src',
  ATTRIBUTE_HTTPREFERRER: 'httpreferrer',
  ATTRIBUTE_NODEINTEGRATION: 'nodeintegration',
  ATTRIBUTE_PLUGINS: 'plugins',
  ATTRIBUTE_DISABLEWEBSECURITY: 'disablewebsecurity',
  ATTRIBUTE_ALLOWPOPUPS: 'allowpopups',
  ATTRIBUTE_PRELOAD: 'preload',
  ATTRIBUTE_USERAGENT: 'useragent',
  ATTRIBUTE_BLINKFEATURES: 'blinkfeatures',
  ATTRIBUTE_DISBLEBLINKFEATURES: 'disableblinkfeatures',

  // Internal attribute.
  ATTRIBUTE_INTERNALINSTANCEID: 'internalinstanceid',

  // Error messages.
  ERROR_MSG_ALREADY_NAVIGATED: 'The object has already navigated, so its partition cannot be changed.',
  ERROR_MSG_CANNOT_INJECT_SCRIPT: '<webview>: ' + 'Script cannot be injected into content until the page has loaded.',
  ERROR_MSG_INVALID_PARTITION_ATTRIBUTE: 'Invalid partition attribute.',
  ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE: 'Only "file:" protocol is supported in "preload" attribute.'
}
