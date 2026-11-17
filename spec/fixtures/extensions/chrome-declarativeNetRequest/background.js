/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type !== 'add-session-rules') return;
  chrome.declarativeNetRequest.updateSessionRules({ addRules: message.rules }).then(
    () => sendResponse(null),
    (error) => sendResponse(error.message)
  );
  return true;
});
