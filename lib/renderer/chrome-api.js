const url = require('url')
const chrome = window.chrome = window.chrome || {}

chrome.extension = {
  getURL: function (path) {
    return url.format({
      protocol: window.location.protocol,
      slashes: true,
      hostname: window.location.hostname,
      pathname: path
    })
  }
}
