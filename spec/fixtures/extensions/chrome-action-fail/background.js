/* global chrome */

const handleRequest = async (request, sender, sendResponse) => {
  const { method } = request;
  const tabId = sender.tab.id;

  switch (method) {
    case 'isEnabled': {
      chrome.action.isEnabled(tabId).then(sendResponse);
      break;
    }

    case 'setIcon': {
      chrome.action.setIcon({ tabId, imageData: {} }).then(sendResponse);
      break;
    }

    case 'getBadgeText': {
      chrome.action.getBadgeText({ tabId }).then(sendResponse);
      break;
    }
  }
};

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  handleRequest(request, sender, sendResponse);
  return true;
});
