if process.platform is 'linux' and process.type is 'renderer'
  # On Linux we could not access clipboard in renderer process.
  module.exports = require('electron').remote.clipboard
else
  module.exports = process.atomBinding 'clipboard'
