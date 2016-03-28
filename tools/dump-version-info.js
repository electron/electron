var app = require('app')
var fs = require('fs')
var request = require('request')

var TARGET_URL = 'https://atom.io/download/atom-shell/index.json'

function getDate () {
  var today = new Date()
  var year = today.getFullYear()
  var month = today.getMonth() + 1
  if (month <= 9) month = '0' + month
  var day = today.getDate()
  if (day <= 9) day = '0' + day
  return year + '-' + month + '-' + day
}

function getInfoForCurrentVersion () {
  var json = {}
  json.version = process.versions['atom-shell']
  json.date = getDate()

  var names = ['node', 'v8', 'uv', 'zlib', 'openssl', 'modules', 'chrome']
  for (var i in names) {
    var name = names[i]
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
  for (var i in all) {
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

    var current = getInfoForCurrentVersion()
    var found = findObjectByVersion(all, current.version)
    if (found === -1) {
      all.unshift(current)
    } else {
      all[found] = current
    }
    fs.writeFileSync(process.argv[2], JSON.stringify(all))
    process.exit(0)
  })
})
