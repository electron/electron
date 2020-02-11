/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  const { method, args = [] } = message
  const tabId = sender.tab.id

  switch (method) {
    case 'sendMessage': {
      const [message] = args
      chrome.tabs.sendMessage(tabId, { message, tabId }, undefined, sendResponse)
      break
    }

    case 'executeScript': {
      const [code] = args
      chrome.tabs.executeScript(tabId, { code }, ([result]) => sendResponse(result))
      break
    }
  }
  // Respond asynchronously
  return true
})
