/* global chrome */

const testMap = {
  getManifest () {
    const manifest = chrome.runtime.getManifest()
    console.log(JSON.stringify(manifest))
  }
}

const dispatchTest = (event) => {
  const testName = event.data
  const test = testMap[testName]
  test()
}
window.addEventListener('message', dispatchTest, false)
