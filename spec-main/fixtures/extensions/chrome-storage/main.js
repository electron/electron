console.log(1)
chrome.storage.local.set({key: 'value'}, () => {
console.log(2)
  chrome.storage.local.get(['key'], ({key}) => {
console.log(3)
    const script = document.createElement('script')
    script.textContent = `require('electron').ipcRenderer.send('storage-success', ${JSON.stringify(key)})`
    document.documentElement.appendChild(script)
console.log(4)
  })
})
