/* eslint-disable */
chrome.storage.local.set({ key: 'value' }, () => {
  chrome.storage.local.get(['key'], ({ key }) => {
    const script = document.createElement('script')
    script.textContent = `require('electron').ipcRenderer.send('storage-success', ${JSON.stringify(key)})`
    document.documentElement.appendChild(script)
  })
})
