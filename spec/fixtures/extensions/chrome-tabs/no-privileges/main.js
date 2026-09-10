/* global chrome */

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  sendResponse(request);
});

window.addEventListener(
  'message',
  (event) => {
    let message = {};
    try {
      message = typeof event.data === 'string' ? JSON.parse(event.data) : event.data;
    } catch {
      // Fall through with an empty message.
    }
    chrome.runtime.sendMessage(message, (response) => {
      console.log(JSON.stringify(response));
    });
  },
  false
);
