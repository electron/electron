assert     = require 'assert'
path       = require 'path'
http       = require 'http'
url        = require 'url'
remote     = require 'remote'
formidable = require 'formidable'

BrowserWindow = remote.require 'browser-window'

describe 'crash-reporter module', ->
  # We have trouble makeing crash reporter work on Yosemite.
  return if process.platform is 'darwin'

  fixtures = path.resolve __dirname, 'fixtures'

  w = null
  beforeEach -> w = new BrowserWindow(show: false)
  afterEach -> w.destroy()

  # It is not working on 64bit Windows.
  return if process.platform is 'win32' and process.arch is 'x64'

  it 'should send minidump when renderer crashes', (done) ->
    @timeout 60000
    server = http.createServer (req, res) ->
      form = new formidable.IncomingForm()
      process.throwDeprecation = false
      form.parse req, (error, fields, files) ->
        process.throwDeprecation = true
        assert.equal fields['prod'], 'Electron'
        assert.equal fields['ver'], process.versions['electron']
        assert.equal fields['process_type'], 'renderer'
        assert.equal fields['platform'], process.platform
        assert.equal fields['extra1'], 'extra1'
        assert.equal fields['extra2'], 'extra2'
        assert.equal fields['_productName'], 'Zombies'
        assert.equal fields['_companyName'], 'Umbrella Corporation'
        assert.equal fields['_version'], require('remote').require('app').getVersion()
        assert files['upload_file_minidump']['name']?

        res.end('abc-123-def')
        server.close()
        done()
    server.listen 0, '127.0.0.1', ->
      {port} = server.address()
      url = url.format
        protocol: 'file'
        pathname: path.join fixtures, 'api', 'crash.html'
        search: "?port=#{port}"
      w.loadUrl url
