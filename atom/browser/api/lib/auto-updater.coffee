module.exports =
  if process.platform is 'win32'
    require './auto-updater/auto-updater-win'
  else
    require './auto-updater/auto-updater-native'
