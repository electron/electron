setImmediate(function () {
  try {
    const { remote } = require('electron')
    console.log(JSON.stringify(typeof remote))
  } catch (e) {
    console.log(e.message)
  }
})
