// Verifies that objects contained in custom snapshot are accessible in Electron.

const {app} = require('electron')

app.once('ready', () => {
  try {
    const testValue = f() // eslint-disable-line no-undef
    if (testValue === 86) {
      console.log('ok test snapshot successfully loaded.')
      app.exit(0)
    } else {
      console.log('not ok test snapshot could not be successfully loaded.')
      app.exit(1)
    }
    return
  } catch (ex) {
    console.log('Error running custom snapshot', ex)
    app.exit(1)
  }
})
