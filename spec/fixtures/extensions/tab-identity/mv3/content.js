/* global chrome */

// Lets the background record how this page is identified as a sender.
chrome.runtime.sendMessage({ type: 'hello' });

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  sendResponse({ echo: message });
});
