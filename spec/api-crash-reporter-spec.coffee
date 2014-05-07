assert     = require 'assert'
path       = require 'path'
http       = require 'http'
url        = require 'url'
remote     = require 'remote'
formidable = require 'formidable'

BrowserWindow = remote.require 'browser-window'

describe 'crash-reporter module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  w = null
  beforeEach -> w = new BrowserWindow(show: false)
  afterEach -> w.destroy()

  it 'should send minidump when renderer crashes', (done) ->
    @timeout 5000
    server = http.createServer (req, res) ->
      form = new formidable.IncomingForm()
      process.throwDeprecation = false
      form.parse req, (error, fields, files) ->
        process.throwDeprecation = true
        assert.equal fields['prod'], 'Atom-Shell'
        assert.equal fields['ver'], process.versions['atom-shell']
        assert.equal fields['process_type'], 'renderer'
        assert.equal fields['platform'], process.platform
        assert.equal fields['extra1'], 'extra1'
        assert.equal fields['extra2'], 'extra2'
        assert.equal fields['_productName'], 'Zombies'
        assert.equal fields['_companyName'], 'Umbrella Corporation'
        assert.equal fields['_version'], require('remote').require('app').getVersion()
        assert files['upload_file_minidump']['name']?

        res.end()
        server.close()
        done()
    server.listen 0, '127.0.0.1', ->
      {port} = server.address()
      url = url.format
        protocol: 'file'
        pathname: path.join fixtures, 'api', 'crash.html'
        search: "?port=#{port}"
      w.loadUrl url
