'use strict'

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')
const { runInThisContext } = require('vm')

// Check whether pattern matches.
// https://developer.chrome.com/extensions/match_patterns
const matchesPattern = function (pattern) {
  if (pattern === '<all_urls>') return true
  const regexp = new RegExp(`^${pattern.replace(/\*/g, '.*')}$`)
  const url = `${location.protocol}//${location.host}${location.pathname}`
  return url.match(regexp)
}

// Run the code with chrome API integrated.
const runContentScript = function (extensionId, url, code) {
  const context = {}
  require('@electron/internal/renderer/chrome-api').injectTo(extensionId, false, context)
  const wrapper = `((chrome) => {\n  ${code}\n  })`
  const compiledWrapper = runInThisContext(wrapper, {
    filename: url,
    lineOffset: 1,
    displayErrors: true
  })
  return compiledWrapper.call(this, context.chrome)
}

const runAllContentScript = function (scripts, extensionId) {
  for (const { url, code } of scripts) {
    runContentScript.call(window, extensionId, url, code)
  }
}

const runStylesheet = function (url, code) {
  const wrapper = `((code) => {
    function init() {
      const styleElement = document.createElement('style');
      styleElement.textContent = code;
      document.head.append(styleElement);
    }
    document.addEventListener('DOMContentLoaded', init);
  })`
  const compiledWrapper = runInThisContext(wrapper, {
    filename: url,
    lineOffset: 1,
    displayErrors: true
  })
  return compiledWrapper.call(this, code)
}

const runAllStylesheet = function (css) {
  for (const { url, code } of css) {
    runStylesheet.call(window, url, code)
  }
}

// Run injected scripts.
// https://developer.chrome.com/extensions/content_scripts
const injectContentScript = function (extensionId, script) {
  if (!script.matches.some(matchesPattern)) return

  if (script.js) {
    const fire = runAllContentScript.bind(window, script.js, extensionId)
    if (script.runAt === 'document_start') {
      process.once('document-start', fire)
    } else if (script.runAt === 'document_end') {
      process.once('document-end', fire)
    } else {
      document.addEventListener('DOMContentLoaded', fire)
    }
  }

  if (script.css) {
    const fire = runAllStylesheet.bind(window, script.css)
    if (script.runAt === 'document_start') {
      process.once('document-start', fire)
    } else if (script.runAt === 'document_end') {
      process.once('document-end', fire)
    } else {
      document.addEventListener('DOMContentLoaded', fire)
    }
  }
}

// Handle the request of chrome.tabs.executeJavaScript.
ipcRendererInternal.on('CHROME_TABS_EXECUTESCRIPT', function (event, senderWebContentsId, requestId, extensionId, url, code) {
  const result = runContentScript.call(window, extensionId, url, code)
  ipcRendererInternal.sendToAll(senderWebContentsId, `CHROME_TABS_EXECUTESCRIPT_RESULT_${requestId}`, result)
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
