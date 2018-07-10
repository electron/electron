const {BrowserWindow} = require('electron')

class Foo {
  set bar (value) {
    if (!(value instanceof BrowserWindow)) {
      throw new Error('setting error')
    }
  }
}

module.exports = new Foo()
