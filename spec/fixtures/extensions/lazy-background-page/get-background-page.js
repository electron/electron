/* global chrome */
window.completionPromise = new Promise((resolve) => {
  window.completionPromiseResolve = resolve;
});
chrome.runtime.sendMessage({ some: 'message' }, () => {
  window.completionPromiseResolve(chrome.extension.getBackgroundPage().receivedMessage);
});
