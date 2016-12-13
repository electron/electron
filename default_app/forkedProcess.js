const os = require('os')
const path = require('path')

let productName = 'Child Product'
let companyName = 'Child Company'
let tmpPath = path.join(os.tmpdir(), productName + ' Crashes')

process.crashReporter.start({
  productName: productName,
  companyName: companyName,
  submitURL: 'http://localhost:1127/post',
  crashesDirectory: tmpPath,
  extra: {
    randomData1: 'The Holidays are here!',
    randomData2: 'Happy New Year!'
  }
})
process.crash()
