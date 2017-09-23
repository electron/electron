const {app} = require('electron')

console.log('launched')

process.on('uncaughtException', () => {
  app.exit(2)
})

const shouldExit = app.makeSingleInstance(() => {
  process.nextTick(() => app.exit(0))
})

if (shouldExit) {
  app.exit(1)
}
