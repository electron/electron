import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import { runInThisContext } from 'vm'

// Check whether pattern matches.
// https://developer.chrome.com/extensions/match_patterns
const matchesPattern = function (pattern: string) {
  if (pattern === '<all_urls>') return true
  const regexp = new RegExp(`^${pattern.replace(/\*/g, '.*')}$`)
  const url = `${location.protocol}//${location.host}${location.pathname}`
  return url.match(regexp)
}

// Run the code with chrome API integrated.
const runContentScript = function (this: any, extensionId: string, url: string, code: string) {
  const context: { chrome?: any } = {}
  require('@electron/internal/renderer/chrome-api').injectTo(extensionId, false, context)
  const wrapper = `((chrome) => {\n  ${code}\n  })`
  const compiledWrapper = runInThisContext(wrapper, {
    filename: url,
    lineOffset: 1,
    displayErrors: true
  })
  return compiledWrapper.call(this, context.chrome)
}

const runAllContentScript = function (scripts: Array<Electron.InjectionBase>, extensionId: string) {
  for (const { url, code } of scripts) {
    runContentScript.call(window, extensionId, url, code)
  }
}

const runStylesheet = function (this: any, url: string, code: string) {
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

const runAllStylesheet = function (css: Array<Electron.InjectionBase>) {
  for (const { url, code } of css) {
    runStylesheet.call(window, url, code)
  }
}

// Run injected scripts.
// https://developer.chrome.com/extensions/content_scripts
const injectContentScript = function (extensionId: string, script: Electron.ContentScript) {
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
ipcRendererInternal.on('CHROME_TABS_EXECUTESCRIPT', function (
  event: Electron.Event,
  senderWebContentsId: number,
  requestId: number,
  extensionId: string,
  url: string,
  code: string
) {
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
