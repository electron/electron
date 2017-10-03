const {app} = require('electron')

console.log('started')  // ping parent

const shouldExit = app.makeSingleInstance(() => {
  process.nextTick(() => app.exit(0))
})

if (shouldExit) {
  app.exit(1)
}
