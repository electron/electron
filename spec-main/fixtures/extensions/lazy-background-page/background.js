/* eslint-disable no-undef */
chrome.runtime.onMessage.addListener((message, sender, reply) => {
  window.receivedMessage = message;
  reply({ message, sender });
});
