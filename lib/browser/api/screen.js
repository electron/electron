const {EventEmitter} = require('events')
const {deprecate} = require('electron')
const {screen, Screen} = process.atomBinding('screen')

// Screen is an EventEmitter.
Object.setPrototypeOf(Screen.prototype, EventEmitter.prototype)
EventEmitter.call(screen)

const nativeFn = screen.getMenuBarHeight
screen.getMenuBarHeight = function () {
  if (!process.noDeprecations) {
    deprecate.warn('screen.getMenuBarHeight', 'screen.getPrimaryDisplay().workArea')
  }
  return nativeFn.call(this)
}

module.exports = screen
