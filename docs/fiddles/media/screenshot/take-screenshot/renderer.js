const { shell, ipcRenderer } = require('electron/renderer')

const fs = require('node:fs').promises
const os = require('node:os')
const path = require('node:path')

const screenshot = document.getElementById('screen-shot')
const screenshotMsg = document.getElementById('screenshot-path')

screenshot.addEventListener('click', async (event) => {
  screenshotMsg.textContent = 'Gathering screens...'
  const thumbSize = await determineScreenShotSize()
  const options = { types: ['screen'], thumbnailSize: thumbSize }

  const sources = await ipcRenderer.invoke('get-sources', options)
  for (const source of sources) {
    const sourceName = source.name.toLowerCase()
    if (sourceName === 'entire screen' || sourceName === 'screen 1') {
      const screenshotPath = path.join(os.tmpdir(), 'screenshot.png')

      await fs.writeFile(screenshotPath, source.thumbnail.toPNG())
      shell.openExternal(`file://${screenshotPath}`)

      const message = `Saved screenshot to: ${screenshotPath}`
      screenshotMsg.textContent = message
    }
  }
})

async function determineScreenShotSize () {
  const screenSize = await ipcRenderer.invoke('get-screen-size')
  const maxDimension = Math.max(screenSize.width, screenSize.height)
  return {
    width: maxDimension * window.devicePixelRatio,
    height: maxDimension * window.devicePixelRatio
  }
}
