const {app} = require('electron')

process.send({ status: 'launched' })

process.on('uncaughtException', () => {
  app.exit(2)
})

const shouldExit = app.makeSingleInstance(() => {
  process.nextTick(() => app.exit(0))
})

if (shouldExit) {
  app.exit(1)
}
