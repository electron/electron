/* eslint-disable */
document.documentElement.textContent = JSON.stringify({
  manifest: chrome.runtime.getManifest(),
  id: chrome.runtime.id,
  url: chrome.runtime.getURL('main.js')
})
