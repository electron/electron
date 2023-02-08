/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  const { method, args = [] } = message;
  const tabId = sender.tab.id;

  switch (method) {
    case 'sendMessage': {
      const [message] = args;
      chrome.tabs.sendMessage(tabId, { message, tabId }, undefined, sendResponse);
      break;
    }

    case 'executeScript': {
      const [code] = args;
      chrome.tabs.executeScript(tabId, { code }, ([result]) => sendResponse(result));
      break;
    }

    case 'connectTab': {
      const [name] = args;
      const port = chrome.tabs.connect(tabId, { name });
      port.postMessage('howdy');
      break;
    }

    case 'update': {
      const [tabId, props] = args;
      chrome.tabs.update(tabId, props, sendResponse);
    }
  }
  // Respond asynchronously
  return true;
});
