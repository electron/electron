const { app, net, netLog } = require('electron')

app.commandLine.appendSwitch('log-net-log', process.env.TEST_DUMP_FILE)

function request () {
  return new Promise((resolve) => {
    const req = net.request(process.env.TEST_REQUEST_URL)
    req.on('response', () => {
      resolve()
    })
    req.end()
  })
}

function stopLogging () {
  return new Promise((resolve) => {
    netLog.stopLogging(() => {
      resolve()
    })
  })
}

app.on('ready', async () => {
  if (process.env.TEST_DUMP_FILE_DYNAMIC) {
    netLog.startLogging(process.env.TEST_DUMP_FILE_DYNAMIC)
  }

  await request()

  if (process.env.TEST_MANUAL_STOP) {
    await stopLogging()
  }

  app.quit()
})
