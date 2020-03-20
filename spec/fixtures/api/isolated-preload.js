const { ipcRenderer, webFrame } = require('electron');

window.foo = 3;

webFrame.executeJavaScript('window.preloadExecuteJavaScriptProperty = 1234;');

window.addEventListener('message', (event) => {
  ipcRenderer.send('isolated-world', {
    preloadContext: {
      preloadProperty: typeof window.foo,
      pageProperty: typeof window.hello,
      typeofRequire: typeof require,
      typeofProcess: typeof process,
      typeofArrayPush: typeof Array.prototype.push,
      typeofFunctionApply: typeof Function.prototype.apply,
      typeofPreloadExecuteJavaScriptProperty: typeof window.preloadExecuteJavaScriptProperty
    },
    pageContext: event.data
  });
});
