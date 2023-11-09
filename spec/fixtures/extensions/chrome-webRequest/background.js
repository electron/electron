/* global chrome */

chrome.webRequest.onBeforeRequest.addListener(
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  (details) => {
    return { cancel: true };
  },
  { urls: ['*://127.0.0.1:*'] },
  ['blocking']
);
