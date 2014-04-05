module.exports =
  if process.platform is 'linux'
    # On Linux we could not access clipboard in renderer process.
    require('remote').process.atomBinding 'clipboard'
  else
    process.atomBinding 'clipboard'
