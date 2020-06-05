/* global chrome */

chrome.webRequest.onBeforeSendHeaders.addListener(
  (details) => {
    if (details.requestHeaders) {
      details.requestHeaders.foo = 'bar';
    }
    return { cancel: false, requestHeaders: details.requestHeaders };
  },
  { urls: ['<all_urls>'] },
  ['blocking']
);
