var atom = requireNative('atom').GetBinding();
var webFrame = atom.web_frame.webFrame;

function setGlobal(path, value) {
  return webFrame.setGlobal(path.split('.'), value)
}

var binding = {
  setZoomFactor: webFrame.setZoomFactor.bind(webFrame),
  getZoomFactor: webFrame.getZoomFactor.bind(webFrame),
  setZoomLevel: webFrame.setZoomLevel.bind(webFrame),
  getZoomLevel: webFrame.getZoomLevel.bind(webFrame),
  setZoomLevelLimits: webFrame.setZoomLevelLimits.bind(webFrame),
  setSpellCheckProvider: webFrame.setSpellCheckProvider.bind(webFrame),
  registerURLSchemeAsBypassingCSP: webFrame.registerURLSchemeAsBypassingCSP.bind(webFrame),
  registerURLSchemeAsPrivileged: webFrame.registerURLSchemeAsPrivileged.bind(webFrame),
  insertText: webFrame.insertText.bind(webFrame),
  executeJavaScript: webFrame.executeJavaScript.bind(webFrame),
  setGlobal
}

exports.binding = binding
