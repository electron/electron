url = require 'url'

chrome = window.chrome = window.chrome || {}
chrome.extension =
  getURL: (path) ->
    url.format
      protocol: location.protocol
      slashes: true
      hostname: location.hostname
      pathname: path
