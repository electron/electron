const { app } = require('electron')

app.commandLine.appendSwitch('--disable-software-rasterizer')

app.on('ready', async () => {
  const gpuInfo = await app.getGPUInfo(process.argv.pop())
  console.log(JSON.stringify(gpuInfo))
  app.exit(0)
})
