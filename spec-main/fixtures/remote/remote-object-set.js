const { BrowserWindow } = require('electron');

class Foo {
  set bar (value) { // eslint-disable-line accessor-pairs
    if (!(value instanceof BrowserWindow)) {
      throw new Error('setting error');
    }
  }
}

module.exports = new Foo();
