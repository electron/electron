/* global chrome */

const handleRequest = (request, sender, sendResponse) => {
  const { method, args = [] } = request;
  const tabId = sender.tab.id;

  switch (method) {
    case 'getZoom': {
      chrome.tabs.getZoom(tabId).then(sendResponse);
      break;
    }

    case 'setZoom': {
      const [zoom] = args;
      chrome.tabs.setZoom(tabId, zoom).then(async () => {
        const updatedZoom = await chrome.tabs.getZoom(tabId);
        sendResponse(updatedZoom);
      });
      break;
    }

    case 'getZoomSettings': {
      chrome.tabs.getZoomSettings(tabId).then(sendResponse);
      break;
    }

    case 'setZoomSettings': {
      const [settings] = args;
      chrome.tabs.setZoomSettings(tabId, { mode: settings.mode }).then(async () => {
        const zoomSettings = await chrome.tabs.getZoomSettings(tabId);
        sendResponse(zoomSettings);
      });
      break;
    }

    case 'get': {
      chrome.tabs.get(tabId).then(sendResponse);
      break;
    }

    case 'query': {
      const [params] = args;
      chrome.tabs.query(params).then(sendResponse);
      break;
    }

    case 'reload': {
      chrome.tabs.reload(tabId).then(() => {
        sendResponse({ status: 'reloaded' });
      });
      break;
    }

    case 'update': {
      const [params] = args;
      chrome.tabs.update(tabId, params).then(sendResponse);
      break;
    }
  }
};

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  handleRequest(request, sender, sendResponse);
  return true;
});
