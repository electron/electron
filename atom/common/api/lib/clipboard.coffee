if process.platform is 'linux' and process.type is 'renderer'
  {remote} = require 'electron'
  # On Linux we could not access clipboard in renderer process.
  module.exports = remote.getBuiltin 'clipboard'
else
  module.exports = process.atomBinding 'clipboard'
