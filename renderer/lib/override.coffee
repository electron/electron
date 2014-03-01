# Redirect window.onerror to uncaughtException.
window.onerror = (error) ->
  if global.process.listeners('uncaughtException').length > 0
    global.process.emit 'uncaughtException', error
    true
  else
    false

# Override default window.close, see:
# https://github.com/atom/atom-shell/issues/70
window.close = ->
  require('remote').getCurrentWindow().close()

# Override default window.open.
window.open = (url, name, features) ->
  options = {}
  for feature in features.split ','
    [name, value] = feature.split '='
    options[name] =
      if value is 'yes'
        true
      else if value is 'no'
        false
      else
        value

  options.x ?= options.left
  options.y ?= options.top
  options.title ?= name
  options.width ?= 800
  options.height ?= 600

  BrowserWindow = require('remote').require 'browser-window'
  browser = new BrowserWindow options
  browser.loadUrl url
  browser
