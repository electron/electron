const { desktopCapturer, shell, ipcRenderer } = require('electron')

const fs = require('fs')
const os = require('os')
const path = require('path')

const screenshot = document.getElementById('screen-shot')
const screenshotMsg = document.getElementById('screenshot-path')

screenshot.addEventListener('click', async (event) => {
  screenshotMsg.textContent = 'Gathering screens...'
  const thumbSize = await determineScreenShotSize()
  const options = { types: ['screen'], thumbnailSize: thumbSize }

  desktopCapturer.getSources(options, (error, sources) => {
    if (error) return console.log(error)

    sources.forEach((source) => {
      const sourceName = source.name.toLowerCase()
      if (sourceName === 'entire screen' || sourceName === 'screen 1') {
        const screenshotPath = path.join(os.tmpdir(), 'screenshot.png')

        fs.writeFile(screenshotPath, source.thumbnail.toPNG(), (error) => {
          if (error) return console.log(error)
          shell.openExternal(`file://${screenshotPath}`)

          const message = `Saved screenshot to: ${screenshotPath}`
          screenshotMsg.textContent = message
        })
      }
    })
  })
})

async function determineScreenShotSize () {
  const screenSize = await ipcRenderer.invoke('get-screen-size')
  const maxDimension = Math.max(screenSize.width, screenSize.height)
  return {
    width: maxDimension * window.devicePixelRatio,
    height: maxDimension * window.devicePixelRatio
  }
}
