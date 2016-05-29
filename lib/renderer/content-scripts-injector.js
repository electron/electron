const {ipcRenderer} = require('electron')
const {runInThisContext} = require('vm')

// Check whether pattern matches.
// https://developer.chrome.com/extensions/match_patterns
const matchesPattern = function (pattern) {
  if (pattern === '<all_urls>') return true

  const regexp = new RegExp('^' + pattern.replace(/\*/g, '.*') + '$')
  return location.href.match(regexp)
}

// Run the code with chrome API integrated.
const runContentScript = function (extensionId, url, code) {
  const context = {}
  require('./chrome-api').injectTo(extensionId, false, context)
  const wrapper = `(function (chrome) {\n  ${code}\n  })`
  const compiledWrapper = runInThisContext(wrapper, {
    filename: url,
    lineOffset: 1,
    displayErrors: true
  })
  return compiledWrapper.call(this, context.chrome)
}

// Run injected scripts.
// https://developer.chrome.com/extensions/content_scripts
const injectContentScript = function (extensionId, script) {
  for (const match of script.matches) {
    if (!matchesPattern(match)) return
  }

  for (const {url, code} of script.js) {
    const fire = runContentScript.bind(window, extensionId, url, code)
    if (script.runAt === 'document_start') {
      process.once('document-start', fire)
    } else if (script.runAt === 'document_end') {
      process.once('document-end', fire)
    } else if (script.runAt === 'document_idle') {
      document.addEventListener('DOMContentLoaded', fire)
    }
  }
}

// Handle the request of chrome.tabs.executeJavaScript.
ipcRenderer.on('CHROME_TABS_EXECUTESCRIPT', function (event, senderWebContentsId, requestId, extensionId, url, code) {
  const result = runContentScript.call(window, extensionId, url, code)
  ipcRenderer.sendToAll(senderWebContentsId, `CHROME_TABS_EXECUTESCRIPT_RESULT_${requestId}`, result)
})

// Read the renderer process preferences.
const preferences = process.getRenderProcessPreferences()
if (preferences) {
  for (const pref of preferences) {
    if (pref.contentScripts) {
      for (const script of pref.contentScripts) {
        injectContentScript(pref.extensionId, script)
      }
    }
  }
}
