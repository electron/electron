module.exports = process.atomBinding 'shell'

if process.platform is 'win32' and process.type is 'renderer'
  module.exports.showItemInFolder = require('remote').process.atomBinding('shell').showItemInFolder
