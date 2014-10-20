var fs = require('fs')
var path = require('path')
module.exports = fs.readFileSync(path.join(__dirname, 'run.bat'), 'utf-8').slice(1,-1)