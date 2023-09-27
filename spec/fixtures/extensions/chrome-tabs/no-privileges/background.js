/* global chrome */

chrome.runtime.onMessage.addListener((_request, sender, sendResponse) => {
  chrome.tabs.get(sender.tab.id).then(sendResponse);
  return true;
});
