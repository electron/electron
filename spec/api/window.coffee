assert = require 'assert'
fs = require 'fs'
path = require 'path'
remote = require 'remote'
BrowserWindow = remote.require 'browser-window'

fixtures = path.resolve __dirname, '..', 'fixtures'

describe 'window module', ->
  describe 'BrowserWindow.close()', ->
    it 'should emit unload handler', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'loading-state-changed', (event, isLoading) ->
        if (!isLoading)
          w.close()
      w.on 'destroyed', ->
        test = path.join(fixtures, 'api', 'test')
        content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal String(content), 'unload'
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'unload.html')

  describe 'window.close()', ->
    it 'should emit unload handler', (done) ->
      w = new BrowserWindow(show: false)
      w.on 'destroyed', ->
        test = path.join(fixtures, 'api', 'close')
        content = fs.readFileSync(test)
        fs.unlinkSync(test)
        assert.equal String(content), 'close'
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'close.html')
