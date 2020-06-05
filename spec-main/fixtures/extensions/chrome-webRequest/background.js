/* global chrome */

chrome.webRequest.onBeforeRequest.addListener(
  (details) => {
    return { cancel: false };
  },
  { urls: ['*://127.0.0.1:*'] },
  ['blocking']
);
