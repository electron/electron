/* global chrome */
chrome.runtime.sendMessage({ text: 'hello from content script' }, (response) => {
  if (!response || !response.text) return;
  document.title = response.text;
});
