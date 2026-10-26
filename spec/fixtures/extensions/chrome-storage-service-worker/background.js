/* global chrome */
const waiting = [];

chrome.storage.onChanged.addListener((changes, areaName) => {
  for (const resolve of waiting.splice(0)) resolve({ changes, areaName });
});

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message !== 'set-and-wait-for-change') return;
  const changed = new Promise((resolve) => waiting.push(resolve));
  chrome.storage.local
    .set({ key: 'value' })
    .then(() => changed)
    .then(sendResponse);
  return true;
});
