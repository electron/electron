const {app} = require('electron')

app.once('ready', () => {
  console.log('started')  // ping parent
})

const shouldExit = app.makeSingleInstance(() => {
  setImmediate(() => app.exit(0))
})

if (shouldExit) {
  app.exit(1)
}
