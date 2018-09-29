const { app } = require('electron')

app.commandLine.appendSwitch('--disable-software-rasterizer')

app.on('ready', () => {
  const infoType = process.argv.pop()
  app.getGPUInfo(infoType).then(
    (gpuInfo) => {
      console.log(JSON.stringify(gpuInfo))
      app.exit(0)
    },
    (error) => {
      console.error(error)
      app.exit(1)
    }
  )
})
