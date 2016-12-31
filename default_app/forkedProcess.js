const os = require('os')
const path = require('path')
const {spawn} = require('child_process')

let submitURL = 'http://localhost:1127/post'
let productName = 'Child Product'
let companyName = 'Child Company'
let tmpPath = path.join(os.tmpdir(), productName + ' Crashes')

if (process.platform === 'win32') {
  const args = [
    '--reporter-url=' + submitURL,
    '--application-name=' + productName,
    '--crashes-directory=' + tmpPath
  ]
  const env = {
    ELECTRON_INTERNAL_CRASH_SERVICE: 1
  }
  spawn(process.execPath, args, {
    env: env,
    detached: true
  })
}

process.crashReporter.start({
  productName: productName,
  companyName: companyName,
  submitURL: submitURL,
  crashesDirectory: tmpPath,
  extra: {
    randomData1: 'The Holidays are here!',
    randomData2: 'Happy New Year!'
  }
})
process.crash()
