/* global chrome */

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  sendResponse(message)
})

const testMap = {
  connect () {
    let success = false
    try {
      chrome.runtime.connect(chrome.runtime.id)
      chrome.runtime.connect(chrome.runtime.id, { name: 'content-script' })
      chrome.runtime.connect({ name: 'content-script' })
      success = true
    } finally {
      console.log(JSON.stringify(success))
    }
  },
  getManifest () {
    const manifest = chrome.runtime.getManifest()
    console.log(JSON.stringify(manifest))
  },
  sendMessage (message) {
    chrome.runtime.sendMessage({ method: 'sendMessage', args: [message] }, response => {
      console.log(JSON.stringify(response))
    })
  },
  executeScript (code) {
    chrome.runtime.sendMessage({ method: 'executeScript', args: [code] }, response => {
      console.log(JSON.stringify(response))
    })
  }
}

const dispatchTest = (event) => {
  const { method, args = [] } = JSON.parse(event.data)
  testMap[method](...args)
}
window.addEventListener('message', dispatchTest, false)
