module.exports =
  if process.platform in ['linux', 'win32'] and process.type is 'renderer'
    # On Linux we could not access screen in renderer process.
    require('remote').require 'screen'
  else
    process.atomBinding 'screen'
