var webFrame = requireNative('webFrame');

function setGlobal(path, value) {
  return webFrame.setGlobal(path.split('.'), value)
}

var binding = {
  setSpellCheckProvider: webFrame.setSpellCheckProvider,
  executeJavaScript: webFrame.executeJavaScript,
  setGlobal
}

exports.binding = binding
