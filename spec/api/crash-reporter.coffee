assert = require 'assert'
path = require 'path'
http = require 'http'
remote = require 'remote'
formidable = require 'formidable'
BrowserWindow = remote.require 'browser-window'

fixtures = path.resolve __dirname, '..', 'fixtures'

describe 'crash-reporter module', ->
  it 'should send minidump when renderer crashes', (done) ->
    w = new BrowserWindow(show: false)
    server = http.createServer (req, res) ->
      form = new formidable.IncomingForm()
      form.parse req, (error, fields, files) ->
        assert.equal fields['atom_shell_version'], process.versions['atom-shell']
        assert.equal fields['process_type'], 'renderer'
        assert.equal files['upload_file_minidump']['name'], 'minidump.dmp'

        w.destroy()
        res.end()
        server.close()
        done()
    server.listen 901007, '127.0.0.1', ->
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'crash.html')
