// Must provide either --log-net-log or env TEST_DUMP_FILE_A

const {app, net} = require('electron')

function request() {
  return new Promise((resolve) => {
    const req = net.request(process.env.TEST_URL)
    req.on('response', (resp) => {
      resolve()
    })
    req.end()
  })
}

function stopLogging () {
  return new Promise((resolve) => {
    net.stopLogging(() => {
      resolve()
    })
  })
}

app.on('ready', async () => {
  // Logging should start automatically if env TEST_DUMP_FILE_A not set
  if (process.env.TEST_DUMP_FILE_A)
    net.startLogging(process.env.TEST_DUMP_FILE_A)

  await request()

  // Optional second phase
  if (process.env.TEST_DUMP_FILE_B) {
    await stopLogging()

    if (process.env.TEST_DUMP_FILE_B === '--log-net-log')
      net.startLogging()  // Log to file path specified by --log-net-log
    else
      net.startLogging(process.env.TEST_DUMP_FILE_B)

    await request()
  }

  app.quit()
})
