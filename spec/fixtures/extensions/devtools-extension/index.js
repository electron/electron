/* global chrome */
chrome.devtools.inspectedWindow.eval('require("electron").ipcRenderer.send("winning")', (result, exc) => {
  console.log(result, exc);
});
