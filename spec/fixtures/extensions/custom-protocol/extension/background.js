/* global chrome */
chrome.runtime.onMessage.addListener((_message, sender, reply) => {
  reply({
    text: 'MESSAGE RECEIVED',
    senderTabId: sender.tab && sender.tab.id
  });
});
