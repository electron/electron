const { app } = require('electron')

app.on('ready', () => {
  const payload = {
    hasSwitch: app.commandLine.hasSwitch('foobar'),
    getSwitchValue: app.commandLine.getSwitchValue('foobar')
  }

  process.stdout.write(JSON.stringify(payload))
  process.stdout.end()

  setImmediate(() => {
    app.quit()
  })
})
