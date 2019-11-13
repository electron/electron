'use strict'

const electron = require('electron')
const fs = require('fs')

const eventBinding = process.electronBinding('event')
const clipboard = process.electronBinding('clipboard')
const features = process.electronBinding('features')

const { crashReporterInit } = require('@electron/internal/browser/crash-reporter-init')
const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal')
const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')
const guestViewManager = require('@electron/internal/browser/guest-view-manager')
const typeUtils = require('@electron/internal/common/type-utils')

const emitCustomEvent = function (contents, eventName, ...args) {
  const event = eventBinding.createWithSender(contents)

  electron.app.emit(eventName, event, contents, ...args)
  contents.emit(eventName, event, ...args)

  return event
}

const logStack = function (contents, code, stack) {
  if (stack) {
    console.warn(`WebContents (${contents.id}): ${code}`, stack)
  }
}

// Implements window.close()
ipcMainInternal.on('ELECTRON_BROWSER_WINDOW_CLOSE', function (event) {
  const window = event.sender.getOwnerBrowserWindow()
  if (window) {
    window.close()
  }
  event.returnValue = null
})

ipcMainUtils.handleSync('ELECTRON_CRASH_REPORTER_INIT', function (event, options) {
  return crashReporterInit(options)
})

ipcMainInternal.handle('ELECTRON_BROWSER_GET_LAST_WEB_PREFERENCES', function (event) {
  return event.sender.getLastWebPreferences()
})

// Methods not listed in this set are called directly in the renderer process.
const allowedClipboardMethods = (() => {
  switch (process.platform) {
    case 'darwin':
      return new Set(['readFindText', 'writeFindText'])
    case 'linux':
      return new Set(Object.keys(clipboard))
    default:
      return new Set()
  }
})()

ipcMainUtils.handleSync('ELECTRON_BROWSER_CLIPBOARD', function (event, method, ...args) {
  if (!allowedClipboardMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`)
  }

  return typeUtils.serialize(electron.clipboard[method](...typeUtils.deserialize(args)))
})

if (features.isDesktopCapturerEnabled()) {
  const desktopCapturer = require('@electron/internal/browser/desktop-capturer')

  ipcMainInternal.handle('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', function (event, options, stack) {
    logStack(event.sender, 'desktopCapturer.getSources()', stack)
    const customEvent = emitCustomEvent(event.sender, 'desktop-capturer-get-sources')

    if (customEvent.defaultPrevented) {
      console.error('Blocked desktopCapturer.getSources()')
      return []
    }

    return desktopCapturer.getSources(event, options)
  })
}

const isRemoteModuleEnabled = features.isRemoteModuleEnabled()
  ? require('@electron/internal/browser/remote/server').isRemoteModuleEnabled
  : () => false

const getPreloadScript = async function (preloadPath) {
  let preloadSrc = null
  let preloadError = null
  try {
    preloadSrc = (await fs.promises.readFile(preloadPath)).toString()
  } catch (error) {
    preloadError = error
  }
  return { preloadPath, preloadSrc, preloadError }
}

if (features.isExtensionsEnabled()) {
  ipcMainUtils.handleSync('ELECTRON_GET_CONTENT_SCRIPTS', () => [])
} else {
  const { getContentScripts } = require('@electron/internal/browser/chrome-extension')
  ipcMainUtils.handleSync('ELECTRON_GET_CONTENT_SCRIPTS', () => getContentScripts())
}

ipcMainUtils.handleSync('ELECTRON_BROWSER_SANDBOX_LOAD', async function (event) {
  const preloadPaths = event.sender._getPreloadPaths()

  let contentScripts = []
  if (!features.isExtensionsEnabled()) {
    const { getContentScripts } = require('@electron/internal/browser/chrome-extension')
    contentScripts = getContentScripts()
  }

  return {
    contentScripts,
    preloadScripts: await Promise.all(preloadPaths.map(path => getPreloadScript(path))),
    isRemoteModuleEnabled: isRemoteModuleEnabled(event.sender),
    isWebViewTagEnabled: guestViewManager.isWebViewTagEnabled(event.sender),
    process: {
      arch: process.arch,
      platform: process.platform,
      env: { ...process.env },
      version: process.version,
      versions: process.versions,
      execPath: process.helperExecPath
    }
  }
})

ipcMainInternal.on('ELECTRON_BROWSER_PRELOAD_ERROR', function (event, preloadPath, error) {
  event.sender.emit('preload-error', event, preloadPath, error)
})
