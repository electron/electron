/* global chrome */
chrome.runtime.sendMessage({ some: 'message' }, (response) => {
  const script = document.createElement('script');
  script.textContent = `require('electron').ipcRenderer.send('bg-page-message-response', ${JSON.stringify(response)})`;
  document.documentElement.appendChild(script);
});
