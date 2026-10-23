/* global chrome */
const seen = [];
const waiting = [];

chrome.webRequest.onBeforeRequest.addListener(
  (details) => {
    seen.push({ url: details.url, tabId: details.tabId });
    for (const check of waiting.splice(0)) check();
  },
  { urls: ['<all_urls>'] }
);

function waitForRequest(marker) {
  return new Promise((resolve) => {
    const check = () => {
      const request = seen.find((r) => r.url.includes(marker));
      if (request) resolve(request);
      else waiting.push(check);
    };
    check();
  });
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type === 'wait-for-request') {
    waitForRequest(message.marker).then(sendResponse);
  } else if (message.type === 'block-in-tab') {
    chrome.declarativeNetRequest
      .updateSessionRules({
        addRules: [
          {
            id: 1,
            condition: { urlFilter: message.marker, tabIds: [message.tabId] },
            action: { type: 'block' }
          }
        ]
      })
      .then(() => sendResponse(true));
  } else {
    sendResponse(null);
  }
  return true;
});
