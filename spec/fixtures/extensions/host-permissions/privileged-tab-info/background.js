/* global chrome */

chrome.runtime.onMessage.addListener((_request, _sender, sendResponse) => {
  chrome.tabs.query({}).then(sendResponse);
  return true;
});
