const { app, net, session } = require('electron')

if (process.env.TEST_DUMP_FILE) {
  app.commandLine.appendSwitch('log-net-log', process.env.TEST_DUMP_FILE)
}

function request () {
  return new Promise((resolve) => {
    const req = net.request(process.env.TEST_REQUEST_URL)
    req.on('response', () => {
      resolve()
    })
    req.end()
  })
}

function stopLogging (netLog) {
  return new Promise((resolve) => {
    netLog.stopLogging((path) => {
      resolve()
    })
  })
}

app.on('ready', async () => {
  const netLog = session.defaultSession.netLog

  // The net log exporter becomes ready only after
  // default path is setup, which is posted as task
  // to a sequenced task runner due to sync IO operations,
  // the task are blocked for some reason,
  // revisit task scheduling after 69 upgrade and fix this workaround.
  setImmediate(async () => {
    if (process.env.TEST_DUMP_FILE_DYNAMIC) {
      netLog.startLogging(process.env.TEST_DUMP_FILE_DYNAMIC)
    }

    await request()

    if (process.env.TEST_MANUAL_STOP) {
      await stopLogging(netLog)
    }

    app.quit()
  })
})
