var fs = require('fs')
var path = require('path')

module.exports = path.join(__dirname, fs.readFileSync(path.join(__dirname, 'path.txt'), 'utf-8'))
