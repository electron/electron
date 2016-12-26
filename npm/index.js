var fs = require('fs')
var path = require('path')

const pathFile = path.join(__dirname, 'path.txt')

if (fs.existsSync(pathFile)) {
  module.exports = path.join(__dirname, fs.readFileSync(pathFile, 'utf-8'))
} else {
  throw new Error('Electron failed to install correctly, please delete node_modules/electron and try installing again');
}
