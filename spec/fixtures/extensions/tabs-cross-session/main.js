/* global chrome */

window.addEventListener('message', (event) => {
  chrome.runtime.sendMessage(JSON.parse(event.data), (response) => {
    console.log(JSON.stringify(response));
  });
}, false);
