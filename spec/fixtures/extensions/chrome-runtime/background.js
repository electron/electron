/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, reply) => {
  switch (message) {
    case 'getPlatformInfo':
      chrome.runtime.getPlatformInfo(reply);
      break;
  }

  // Respond asynchronously
  return true;
});
