const { desktopCapturer, ipcRenderer, remote, webFrame } = require('electron')

process.once('loaded', async () => {
  let sources = []
  try {
    sources = await desktopCapturer.getSources({ types: ['window', 'screen'] })
  } catch {
    // ignore
  }

  function doesThrow (code) {
    try {
      code()
      return false
    } catch {
      return true
    }
  }

  let enablesContextIsolation
  try {
    enablesContextIsolation = await webFrame.executeJavaScript(`typeof electron === 'undefined'`)
  } catch {
    // ignore
  }

  ipcRenderer.send('result', {
    enablesSandbox: !!process.sandboxed,
    enablesContextIsolation,
    blocksGetSources: sources.length === 0,
    blocksRemoteRequire: doesThrow(() => remote.require('electron')),
    blocksRemoteGetGlobal: doesThrow(() => remote.process),
    blocksRemoteGetBuiltin: doesThrow(() => remote.app),
    blocksRemoteGetCurrentWindow: doesThrow(() => remote.getCurrentWindow()),
    blocksRemoteGetCurrentWebContents: doesThrow(() => remote.getCurrentWebContents())
  })
})
