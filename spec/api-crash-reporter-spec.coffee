assert     = require 'assert'
path       = require 'path'
http       = require 'http'
url        = require 'url'
remote     = require 'remote'
formidable = require 'formidable'

crashReporter = remote.require 'crash-reporter'
BrowserWindow = remote.require 'browser-window'

describe 'crash-reporter module', ->
  fixtures = path.resolve __dirname, 'fixtures'

  w = null
  beforeEach -> w = new BrowserWindow(show: false)
  afterEach -> w.destroy()

  # It is not working for mas build.
  return if process.mas

  # The crash-reporter test is not reliable on CI machine.
  isCI = remote.process.argv[2] == '--ci'
  return if isCI

  it 'should send minidump when renderer crashes', (done) ->
    @timeout 120000
    server = http.createServer (req, res) ->
      server.close()
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
        done()
    # Server port is generated randomly for the first run, it will be reused
    # when page is refreshed.
    port = remote.process.port
    server.listen port, '127.0.0.1', ->
      {port} = server.address()
      remote.process.port = port
      url = url.format
        protocol: 'file'
        pathname: path.join fixtures, 'api', 'crash.html'
        search: "?port=#{port}"
      if process.platform is 'darwin'
        crashReporter.start {'submitUrl': 'http://127.0.0.1:' + port}
      w.loadUrl url
