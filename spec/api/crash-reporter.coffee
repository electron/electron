assert = require 'assert'
path = require 'path'
http = require 'http'
remote = require 'remote'
BrowserWindow = remote.require 'browser-window'

fixtures = path.resolve __dirname, '..', 'fixtures'

describe 'crash-reporter module', ->
  it 'should send minidump when renderer crashes', (done) ->
    w = new BrowserWindow(show: false)
    server = http.createServer (req, res) ->
      res.end()
      server.close()
      done()
    server.listen 901007, '127.0.0.1', ->
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'crash.html')
