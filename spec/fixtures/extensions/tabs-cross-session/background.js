/* global chrome */

const handleRequest = async (request, sender, sendResponse) => {
  const { method, tabId, args = [] } = request;

  try {
    switch (method) {
      case 'get': {
        const tab = await chrome.tabs.get(tabId);
        sendResponse({ ok: true, result: tab });
        break;
      }
      case 'update': {
        const tab = await chrome.tabs.update(tabId, args[0]);
        sendResponse({ ok: true, result: tab });
        break;
      }
      case 'reload': {
        await chrome.tabs.reload(tabId);
        sendResponse({ ok: true });
        break;
      }
      case 'executeScript': {
        const results = await chrome.scripting.executeScript({
          target: { tabId },
          func: () => document.title
        });
        sendResponse({ ok: true, result: results });
        break;
      }
      case 'sendMessage': {
        const response = await chrome.tabs.sendMessage(tabId, args[0]);
        sendResponse({ ok: true, result: response });
        break;
      }
    }
  } catch (error) {
    sendResponse({ ok: false, error: error.message });
  }
};

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  handleRequest(request, sender, sendResponse);
  return true;
});
