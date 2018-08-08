const {app} = require('electron')

app.once('ready', () => {
  console.log('started')  // ping parent
})

const gotTheLock = app.requestSingleInstanceLock()

app.on('second-instance', () => {
  setImmediate(() => app.exit(0))
})

if (!gotTheLock) {
  app.exit(1)
}
