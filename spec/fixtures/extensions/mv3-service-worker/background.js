/* global chrome */

// Chromium only stores the registration after the install event, and the
// worker can receive messages before then. It activates after it has been
// stored, so replying only once activated tells the test it has been stored.
const activated = self.registration.active
  ? Promise.resolve()
  : new Promise((resolve) => self.addEventListener('activate', resolve));

chrome.runtime.onMessage.addListener((message, _sender, sendResponse) => {
  if (message === 'fetch-confirmation') {
    activated.then(() => sendResponse({ message: 'Hello from background.js' }));
    return true;
  }
});
