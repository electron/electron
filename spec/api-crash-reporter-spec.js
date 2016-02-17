const assert = require('assert');
const http = require('http');
const multiparty = require('multiparty');
const path = require('path');
const url = require('url');

const remote = require('electron').remote;
const app = remote.require('electron').app;
const crashReporter = remote.require('electron').crashReporter;
const BrowserWindow = remote.require('electron').BrowserWindow;

describe('crash-reporter module', function() {
  var fixtures, isCI, w;
  fixtures = path.resolve(__dirname, 'fixtures');
  w = null;
  beforeEach(function() {
    return w = new BrowserWindow({
      show: false
    });
  });
  afterEach(function() {
    return w.destroy();
  });
  if (process.mas) {
    return;
  }
  isCI = remote.getGlobal('isCi');
  if (isCI) {
    return;
  }
  it('should send minidump when renderer crashes', function(done) {
    var called, port, server;
    this.timeout(120000);
    called = false;
    server = http.createServer(function(req, res) {
      var form;
      server.close();
      form = new multiparty.Form();
      return form.parse(req, function(error, fields) {
        if (called) {
          return;
        }
        called = true;
        assert.equal(fields['prod'], 'Electron');
        assert.equal(fields['ver'], process.versions['electron']);
        assert.equal(fields['process_type'], 'renderer');
        assert.equal(fields['platform'], process.platform);
        assert.equal(fields['extra1'], 'extra1');
        assert.equal(fields['extra2'], 'extra2');
        assert.equal(fields['_productName'], 'Zombies');
        assert.equal(fields['_companyName'], 'Umbrella Corporation');
        assert.equal(fields['_version'], app.getVersion());
        res.end('abc-123-def');
        return done();
      });
    });
    port = remote.process.port;
    return server.listen(port, '127.0.0.1', function() {
      port = server.address().port;
      remote.process.port = port;
      const crashUrl = url.format({
        protocol: 'file',
        pathname: path.join(fixtures, 'api', 'crash.html'),
        search: "?port=" + port
      });
      if (process.platform === 'darwin') {
        crashReporter.start({
          companyName: 'Umbrella Corporation',
          submitURL: "http://127.0.0.1:" + port
        });
      }
      return w.loadURL(crashUrl);
    });
  });
  return describe(".start(options)", function() {
    return it('requires that the companyName and submitURL options be specified', function() {
      assert.throws(function() {
        return crashReporter.start({
          companyName: 'Missing submitURL'
        });
      });
      return assert.throws(function() {
        return crashReporter.start({
          submitURL: 'Missing companyName'
        });
      });
    });
  });
});
