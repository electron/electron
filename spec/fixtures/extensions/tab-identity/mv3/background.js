/* global chrome */

// The window ID the spec registers the tabs under whose requests the filtered
// listener must report.
const WATCHED_WINDOW_ID = 7;

const lists = { requests: [], windowRequests: [], senders: [] };
const waiting = [];

function record(list, entry) {
  lists[list].push(entry);
  for (const check of waiting.splice(0)) check();
}

function waitFor(list, marker) {
  return new Promise((resolve) => {
    const check = () => {
      const entry = lists[list].find((e) => e.url.includes(marker));
      if (entry) resolve(entry);
      else waiting.push(check);
    };
    check();
  });
}

chrome.webRequest.onBeforeRequest.addListener(
  (details) => record('requests', { url: details.url, tabId: details.tabId }),
  { urls: ['<all_urls>'] }
);

chrome.webRequest.onBeforeRequest.addListener(
  (details) => record('windowRequests', { url: details.url, tabId: details.tabId }),
  { urls: ['<all_urls>'], windowId: WATCHED_WINDOW_ID }
);

const describeSender = (sender) => ({ url: sender.url, hasTab: sender.tab !== undefined, tab: sender.tab });

const settle = (promise, sendResponse) =>
  promise.then(
    (result) => sendResponse({ result }),
    (error) => sendResponse({ error: error.message })
  );

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  switch (message.type) {
    case 'hello':
      record('senders', describeSender(sender));
      sendResponse(null);
      return false;
    case 'sender':
      sendResponse(describeSender(sender));
      return false;
    case 'wait-for':
      waitFor(message.list, message.marker).then(sendResponse);
      return true;
    case 'list':
      sendResponse(lists[message.list]);
      return false;
    case 'call': {
      const [namespace, method] = message.api.split('.');
      settle(chrome[namespace][method](...message.args), sendResponse);
      return true;
    }
    case 'execute-script':
      settle(
        chrome.scripting.executeScript({ target: { tabId: message.tabId }, func: () => document.title }),
        sendResponse
      );
      return true;
    default:
      sendResponse(null);
      return false;
  }
});
