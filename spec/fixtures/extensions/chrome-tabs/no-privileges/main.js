/* global chrome */

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  sendResponse(request);
});

window.addEventListener('message', () => {
  chrome.runtime.sendMessage({}, response => {
    console.log(JSON.stringify(response));
  });
}, false);
