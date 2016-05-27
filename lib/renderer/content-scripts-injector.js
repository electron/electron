const preferences = process.getRenderProcessPreferences()
if (!preferences || preferences.length == 0) return

const {webFrame} = require('electron')

// Check whether pattern matches.
// https://developer.chrome.com/extensions/match_patterns
const matchesPattern = function (pattern) {
  if (pattern === '<all_urls>')
    return true

  const regexp = new RegExp('^' + pattern.replace(/\*/g, '.*') + '$')
  return location.href.match(regexp)
}

// Run injected scripts.
// https://developer.chrome.com/extensions/content_scripts
const injectContentScript = function (script) {
  for (const match of script.matches) {
    if (!matchesPattern(match)) return
  }

  for (const js of script.js) {
    if (script.runAt === 'document_start') {
      webFrame.executeJavaScript(String(js))
    }
  }
}

// Read the renderer process preferences.
for (const pref of preferences) {
  if (pref.contentScripts) {
    pref.contentScripts.forEach(injectContentScript)
  }
}
