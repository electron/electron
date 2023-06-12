/* global chrome */
chrome.runtime.onMessage.addListener((message, sender, reply) => {
  window.receivedMessage = message;
  reply({ message, sender });
});
