/* eslint-disable no-undef */
chrome.runtime.onMessage.addListener((message, sender, reply) => {
  reply({ message, sender })
})
