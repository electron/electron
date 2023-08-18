/* global chrome */

chrome.runtime.onMessage.addListener((request, _sender, sendResponse) => {
  sendResponse(request);
});

window.addEventListener('message', () => {
  chrome.runtime.sendMessage({ method: 'query' }, response => {
    console.log(JSON.stringify(response));
  });
}, false);
