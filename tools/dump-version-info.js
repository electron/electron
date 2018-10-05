const app = require('electron').app
const fs = require('fs')
const request = require('request')

const TARGET_URL = 'https://atom.io/download/electron/index.json'

function getDate () {
  const today = new Date()
  const year = today.getFullYear()
  let month = today.getMonth() + 1
  if (month <= 9) month = '0' + month
  let day = today.getDate()
  if (day <= 9) day = '0' + day
  return year + '-' + month + '-' + day
}

function getInfoForCurrentVersion () {
  const json = {}
  json.version = process.versions.electron
  json.date = getDate()

  const names = ['node', 'v8', 'uv', 'zlib', 'openssl', 'modules', 'chrome']
  for (const i in names) {
    const name = names[i]
    json[name] = process.versions[name]
  }

  json.files = [
    'darwin-x64',
    'darwin-x64-symbols',
    'linux-ia32',
    'linux-ia32-symbols',
    'linux-x64',
    'linux-x64-symbols',
    'win32-ia32',
    'win32-ia32-symbols',
    'win32-x64',
    'win32-x64-symbols'
  ]

  return json
}

function getIndexJsInServer (callback) {
  request(TARGET_URL, function (e, res, body) {
    if (e) {
      callback(e)
    } else if (res.statusCode !== 200) {
      callback(new Error('Server returned ' + res.statusCode))
    } else {
      callback(null, JSON.parse(body))
    }
  })
}

function findObjectByVersion (all, version) {
  for (const i in all) {
    if (all[i].version === version) return i
  }
  return -1
}

app.on('ready', function () {
  getIndexJsInServer(function (e, all) {
    if (e) {
      console.error(e)
      process.exit(1)
    }

    const current = getInfoForCurrentVersion()
    const found = findObjectByVersion(all, current.version)
    if (found === -1) {
      all.unshift(current)
    } else {
      all[found] = current
    }
    fs.writeFileSync(process.argv[2], JSON.stringify(all))
    process.exit(0)
  })
})
