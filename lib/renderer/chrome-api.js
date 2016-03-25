const url = require('url')
const chrome = window.chrome = window.chrome || {}

chrome.extension = {
  getURL: function (path) {
    return url.format({
      protocol: location.protocol,
      slashes: true,
      hostname: location.hostname,
      pathname: path
    })
  }
}
