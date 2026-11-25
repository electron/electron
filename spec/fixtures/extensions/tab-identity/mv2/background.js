/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type !== 'execute-script') return false;
  // No tab ID: the script runs in the current tab.
  chrome.tabs.executeScript({ code: message.code }, (results) => {
    sendResponse(chrome.runtime.lastError ? { error: chrome.runtime.lastError.message } : { results });
  });
  return true;
});
