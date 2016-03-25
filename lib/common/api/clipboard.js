if (process.platform === 'linux' && process.type === 'renderer') {
  // On Linux we could not access clipboard in renderer process.
  module.exports = require('electron').remote.clipboard
} else {
  module.exports = process.atomBinding('clipboard')
}
