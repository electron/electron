/* global chrome */

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  const { method } = request || {};
  if (method === 'query') {
    chrome.tabs.query({}).then(sendResponse);
  } else {
    chrome.tabs.get(sender.tab.id).then(sendResponse);
  }
  return true;
});
