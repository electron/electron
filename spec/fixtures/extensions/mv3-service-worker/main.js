/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  sendResponse(message);
});

window.addEventListener('message', (event) => {
  if (event.data === 'fetch-confirmation') {
    chrome.runtime.sendMessage('fetch-confirmation', response => {
      console.log(JSON.stringify(response));
    });
  }
}, false);
