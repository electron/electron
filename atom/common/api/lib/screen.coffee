module.exports =
  if process.platform is 'linux' and process.type is 'renderer'
    # On Linux we could not access screen in renderer process.
    require('remote').require 'screen'
  else
    process.atomBinding 'screen'
