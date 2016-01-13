var DEPRECATED_EVENTS, WEB_VIEW_EVENTS, dispatchEvent, ipcRenderer, ref, requestId, webFrame,
  slice = [].slice;

ref = require('electron'), ipcRenderer = ref.ipcRenderer, webFrame = ref.webFrame;

requestId = 0;

WEB_VIEW_EVENTS = {
  'load-commit': ['url', 'isMainFrame'],
  'did-finish-load': [],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL'],
  'did-frame-finish-load': ['isMainFrame'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'did-get-response-details': ['status', 'newURL', 'originalURL', 'httpResponseCode', 'requestMethod', 'referrer', 'headers'],
  'did-get-redirect-request': ['oldURL', 'newURL', 'isMainFrame'],
  'dom-ready': [],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'devtools-opened': [],
  'devtools-closed': [],
  'devtools-focused': [],
  'new-window': ['url', 'frameName', 'disposition', 'options'],
  'will-navigate': ['url'],
  'did-navigate': ['url'],
  'did-navigate-in-page': ['url'],
  'close': [],
  'crashed': [],
  'gpu-crashed': [],
  'plugin-crashed': ['name', 'version'],
  'media-started-playing': [],
  'media-paused': [],
  'did-change-theme-color': ['themeColor'],
  'destroyed': [],
  'page-title-updated': ['title', 'explicitSet'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'found-in-page': ['result']
};

DEPRECATED_EVENTS = {
  'page-title-updated': 'page-title-set'
};

dispatchEvent = function() {
  var args, domEvent, eventKey, eventName, f, i, j, len, ref1, webView;
  webView = arguments[0], eventName = arguments[1], eventKey = arguments[2], args = 4 <= arguments.length ? slice.call(arguments, 3) : [];
  if (DEPRECATED_EVENTS[eventName] != null) {
    dispatchEvent.apply(null, [webView, DEPRECATED_EVENTS[eventName], eventKey].concat(slice.call(args)));
  }
  domEvent = new Event(eventName);
  ref1 = WEB_VIEW_EVENTS[eventKey];
  for (i = j = 0, len = ref1.length; j < len; i = ++j) {
    f = ref1[i];
    domEvent[f] = args[i];
  }
  webView.dispatchEvent(domEvent);
  if (eventName === 'load-commit') {
    return webView.onLoadCommit(domEvent);
  }
};

module.exports = {
  registerEvents: function(webView, viewInstanceId) {
    ipcRenderer.on("ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-" + viewInstanceId, function() {
      var args, event, eventName;
      event = arguments[0], eventName = arguments[1], args = 3 <= arguments.length ? slice.call(arguments, 2) : [];
      return dispatchEvent.apply(null, [webView, eventName, eventName].concat(slice.call(args)));
    });
    ipcRenderer.on("ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-" + viewInstanceId, function() {
      var args, channel, domEvent, event;
      event = arguments[0], channel = arguments[1], args = 3 <= arguments.length ? slice.call(arguments, 2) : [];
      domEvent = new Event('ipc-message');
      domEvent.channel = channel;
      domEvent.args = slice.call(args);
      return webView.dispatchEvent(domEvent);
    });
    return ipcRenderer.on("ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-" + viewInstanceId, function() {
      var args, domEvent, event, f, i, j, len, ref1;
      event = arguments[0], args = 2 <= arguments.length ? slice.call(arguments, 1) : [];
      domEvent = new Event('size-changed');
      ref1 = ['oldWidth', 'oldHeight', 'newWidth', 'newHeight'];
      for (i = j = 0, len = ref1.length; j < len; i = ++j) {
        f = ref1[i];
        domEvent[f] = args[i];
      }
      return webView.onSizeChanged(domEvent);
    });
  },
  deregisterEvents: function(viewInstanceId) {
    ipcRenderer.removeAllListeners("ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-" + viewInstanceId);
    ipcRenderer.removeAllListeners("ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-" + viewInstanceId);
    return ipcRenderer.removeAllListeners("ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-" + viewInstanceId);
  },
  createGuest: function(params, callback) {
    requestId++;
    ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', params, requestId);
    return ipcRenderer.once("ATOM_SHELL_RESPONSE_" + requestId, callback);
  },
  attachGuest: function(elementInstanceId, guestInstanceId, params) {
    ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', elementInstanceId, guestInstanceId, params);
    return webFrame.attachGuest(elementInstanceId);
  },
  destroyGuest: function(guestInstanceId) {
    return ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', guestInstanceId);
  },
  setSize: function(guestInstanceId, params) {
    return ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', guestInstanceId, params);
  },
  setAllowTransparency: function(guestInstanceId, allowtransparency) {
    return ipcRenderer.send('ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', guestInstanceId, allowtransparency);
  }
};
