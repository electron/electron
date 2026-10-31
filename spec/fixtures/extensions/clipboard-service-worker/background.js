/* global chrome */
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  navigator.clipboard.writeText(message).then(
    () => sendResponse({ written: true }),
    (error) => sendResponse({ written: false, error: String(error) })
  );
  return true;
});
