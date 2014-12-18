module.exports =
  # Attributes.
  ATTRIBUTE_ALLOWTRANSPARENCY: 'allowtransparency'
  ATTRIBUTE_AUTOSIZE: 'autosize'
  ATTRIBUTE_MAXHEIGHT: 'maxheight'
  ATTRIBUTE_MAXWIDTH: 'maxwidth'
  ATTRIBUTE_MINHEIGHT: 'minheight'
  ATTRIBUTE_MINWIDTH: 'minwidth'
  ATTRIBUTE_NAME: 'name'
  ATTRIBUTE_PARTITION: 'partition'
  ATTRIBUTE_SRC: 'src'
  ATTRIBUTE_HTTPREFERRER: 'httpreferrer'
  ATTRIBUTE_NODEINTEGRATION: 'nodeintegration'
  ATTRIBUTE_PLUGINS: 'plugins'
  ATTRIBUTE_DISABLEWEBSECURITY: 'disablewebsecurity'
  ATTRIBUTE_PRELOAD: 'preload'

  # Internal attribute.
  ATTRIBUTE_INTERNALINSTANCEID: 'internalinstanceid'

  # Error messages.
  ERROR_MSG_ALREADY_NAVIGATED: 'The object has already navigated, so its partition cannot be changed.'
  ERROR_MSG_CANNOT_INJECT_SCRIPT: '<webview>: ' +
      'Script cannot be injected into content until the page has loaded.'
  ERROR_MSG_CONTENTWINDOW_NOT_AVAILABLE: '<webview>: ' +
      'contentWindow is not available at this time. It will become available ' +
      'when the page has finished loading.'
  ERROR_MSG_INVALID_PARTITION_ATTRIBUTE: 'Invalid partition attribute.'
  ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE: 'Only "file:" or "asar:" protocol is supported in "preload" attribute.'
