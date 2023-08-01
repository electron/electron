/* global chrome */

chrome.runtime.onMessage.addListener((message, _sender, sendResponse) => {
  if (message === 'fetch-confirmation') {
    sendResponse({ message: 'Hello from background.js' });
  }
});
