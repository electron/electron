assert = require 'assert'
path = require 'path'

{webFrame} = require 'electron'

describe 'webFrame module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  describe 'webFrame.registerURLSchemeAsPrivileged', ->
    it 'supports fetch api', (done) ->
      webFrame.registerURLSchemeAsPrivileged 'file'
      url = "file://#{fixtures}/assets/logo.png"

      fetch(url).then((response) ->
        assert response.ok
        done()
      ).catch (err) ->
        done('unexpected error : ' + err)
