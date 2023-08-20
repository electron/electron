/* global chrome */

const handleRequest = async (request, sender, sendResponse) => {
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
      try {
        const response = await chrome.tabs.update(tabId, params);
        sendResponse(response);
      } catch (error) {
        sendResponse({ error: error.message });
      }
      break;
    }
  }
};

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  handleRequest(request, sender, sendResponse);
  return true;
});
