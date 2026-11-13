/* global chrome */

// Records every webRequest event without blocking or modifying the request.
const events = [];
const filter = { urls: ['http://127.0.0.1/*', 'ws://127.0.0.1/*'] };

const record = (name) => (details) => {
  events.push({
    name,
    url: details.url,
    type: details.type,
    error: details.error,
    redirectUrl: details.redirectUrl,
    requestHeaders: details.requestHeaders,
    responseHeaders: details.responseHeaders
  });
};

chrome.webRequest.onBeforeRequest.addListener(record('onBeforeRequest'), filter);
chrome.webRequest.onBeforeSendHeaders.addListener(record('onBeforeSendHeaders'), filter);
chrome.webRequest.onSendHeaders.addListener(record('onSendHeaders'), filter, ['requestHeaders']);
chrome.webRequest.onHeadersReceived.addListener(record('onHeadersReceived'), filter, ['responseHeaders']);
chrome.webRequest.onBeforeRedirect.addListener(record('onBeforeRedirect'), filter);
chrome.webRequest.onResponseStarted.addListener(record('onResponseStarted'), filter);
chrome.webRequest.onCompleted.addListener(record('onCompleted'), filter);
chrome.webRequest.onErrorOccurred.addListener(record('onErrorOccurred'), filter);

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type === 'events') {
    sendResponse(events.filter((event) => event.url.includes(message.token)));
  } else if (message.type === 'listen-with-extra-headers') {
    // An 'extraHeaders' listener makes the browser deliver header events for
    // every request from the network service instead of the URL loader.
    chrome.webRequest.onSendHeaders.addListener(record('onSendHeaders:extraHeaders'), filter, [
      'requestHeaders',
      'extraHeaders'
    ]);
    chrome.webRequest.onHeadersReceived.addListener(record('onHeadersReceived:extraHeaders'), filter, [
      'responseHeaders',
      'extraHeaders'
    ]);
    sendResponse(true);
  }
});
