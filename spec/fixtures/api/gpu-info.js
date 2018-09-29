const { app } = require('electron')

app.commandLine.appendSwitch('--disable-software-rasterizer')

app.on('ready', () => {
  const infoType = process.argv.pop()
  app.getGPUInfo(infoType).then(
    (gpuInfo) => {
      console.log(JSON.stringify(gpuInfo))
      app.exit(0)
    },
    (err) => {
      console.error(err)
      app.exit(1)
    }
  )
})
