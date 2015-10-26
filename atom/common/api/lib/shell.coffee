module.exports = process.atomBinding 'shell'

if process.platform is 'win32' and process.type is 'renderer'
  module.exports.showItemInFolder = (item) ->
    require('remote').require('shell').showItemInFolder item
