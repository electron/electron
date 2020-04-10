/* global chrome */

chrome.webRequest.onBeforeRequest.addListener(
  (details) => {
    return {cancel: false};
  },
  {urls: ["<all_urls>"]},
  ["blocking"]
);