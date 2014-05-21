module.exports = process.atomBinding 'shell'

if process.platform is 'win32' and process.__atom_type is 'renderer'
  module.exports.showItemInFolder = require('remote').process.atomBinding('shell').showItemInFolder
