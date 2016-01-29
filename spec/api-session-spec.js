const assert = require('assert');
const http = require('http');
const path = require('path');
const fs = require('fs');

const ipcRenderer = require('electron').ipcRenderer;
const remote = require('electron').remote;

const ipcMain = remote.ipcMain;
const session = remote.session;
const BrowserWindow = remote.BrowserWindow;

describe('session module', function() {
  var fixtures, url, w;
  this.timeout(10000);
  fixtures = path.resolve(__dirname, 'fixtures');
  w = null;
  url = "http://127.0.0.1";
  beforeEach(function() {
    return w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    });
  });
  afterEach(function() {
    return w.destroy();
  });

  describe('session.cookies', function() {
    it('should get cookies', function(done) {
      var server;
      server = http.createServer(function(req, res) {
        res.setHeader('Set-Cookie', ['0=0']);
        res.end('finished');
        return server.close();
      });
      return server.listen(0, '127.0.0.1', function() {
        var port;
        port = server.address().port;
        w.loadURL(url + ":" + port);
        return w.webContents.on('did-finish-load', function() {
          return w.webContents.session.cookies.get({
            url: url
          }, function(error, list) {
            var cookie, i, len;
            if (error) {
              return done(error);
            }
            for (i = 0, len = list.length; i < len; i++) {
              cookie = list[i];
              if (cookie.name === '0') {
                if (cookie.value === '0') {
                  return done();
                } else {
                  return done("cookie value is " + cookie.value + " while expecting 0");
                }
              }
            }
            return done('Can not find cookie');
          });
        });
      });
    });
    it('should over-write the existent cookie', function(done) {
      return session.defaultSession.cookies.set({
        url: url,
        name: '1',
        value: '1'
      }, function(error) {
        if (error) {
          return done(error);
        }
        return session.defaultSession.cookies.get({
          url: url
        }, function(error, list) {
          var cookie, i, len;
          if (error) {
            return done(error);
          }
          for (i = 0, len = list.length; i < len; i++) {
            cookie = list[i];
            if (cookie.name === '1') {
              if (cookie.value === '1') {
                return done();
              } else {
                return done("cookie value is " + cookie.value + " while expecting 1");
              }
            }
          }
          return done('Can not find cookie');
        });
      });
    });
    it('should remove cookies', function(done) {
      return session.defaultSession.cookies.set({
        url: url,
        name: '2',
        value: '2'
      }, function(error) {
        if (error) {
          return done(error);
        }
        return session.defaultSession.cookies.remove(url, '2', function() {
          return session.defaultSession.cookies.get({
            url: url
          }, function(error, list) {
            var cookie, i, len;
            if (error) {
              return done(error);
            }
            for (i = 0, len = list.length; i < len; i++) {
              cookie = list[i];
              if (cookie.name === '2') {
                return done('Cookie not deleted');
              }
            }
            return done();
          });
        });
      });
    });
  });

  describe('session.clearStorageData(options)', function() {
    fixtures = path.resolve(__dirname, 'fixtures');
    return it('clears localstorage data', function(done) {
      ipcMain.on('count', function(event, count) {
        ipcMain.removeAllListeners('count');
        assert(!count);
        return done();
      });
      w.loadURL('file://' + path.join(fixtures, 'api', 'localstorage.html'));
      return w.webContents.on('did-finish-load', function() {
        var options;
        options = {
          origin: "file://",
          storages: ['localstorage'],
          quotas: ['persistent']
        };
        return w.webContents.session.clearStorageData(options, function() {
          return w.webContents.send('getcount');
        });
      });
    });
  });
  return describe('DownloadItem', function() {
    var assertDownload, contentDisposition, downloadFilePath, downloadServer, mockPDF;
    mockPDF = new Buffer(1024 * 1024 * 5);
    contentDisposition = 'inline; filename="mock.pdf"';
    downloadFilePath = path.join(fixtures, 'mock.pdf');
    downloadServer = http.createServer(function(req, res) {
      res.writeHead(200, {
        'Content-Length': mockPDF.length,
        'Content-Type': 'application/pdf',
        'Content-Disposition': contentDisposition
      });
      res.end(mockPDF);
      return downloadServer.close();
    });
    assertDownload = function(event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename, port) {
      assert.equal(state, 'completed');
      assert.equal(filename, 'mock.pdf');
      assert.equal(url, "http://127.0.0.1:" + port + "/");
      assert.equal(mimeType, 'application/pdf');
      assert.equal(receivedBytes, mockPDF.length);
      assert.equal(totalBytes, mockPDF.length);
      assert.equal(disposition, contentDisposition);
      assert(fs.existsSync(downloadFilePath));
      return fs.unlinkSync(downloadFilePath);
    };
    it('can download using BrowserWindow.loadURL', function(done) {
      return downloadServer.listen(0, '127.0.0.1', function() {
        var port;
        port = downloadServer.address().port;
        ipcRenderer.sendSync('set-download-option', false);
        w.loadURL(url + ":" + port);
        return ipcRenderer.once('download-done', function(event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename) {
          assertDownload(event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename, port);
          return done();
        });
      });
    });
    it('can download using WebView.downloadURL', function(done) {
      return downloadServer.listen(0, '127.0.0.1', function() {
        var port, webview;
        port = downloadServer.address().port;
        ipcRenderer.sendSync('set-download-option', false);
        webview = new WebView;
        webview.src = "file://" + fixtures + "/api/blank.html";
        webview.addEventListener('did-finish-load', function() {
          return webview.downloadURL(url + ":" + port + "/");
        });
        ipcRenderer.once('download-done', function(event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename) {
          assertDownload(event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename, port);
          document.body.removeChild(webview);
          return done();
        });
        return document.body.appendChild(webview);
      });
    });
    return it('can cancel download', function(done) {
      return downloadServer.listen(0, '127.0.0.1', function() {
        var port;
        port = downloadServer.address().port;
        ipcRenderer.sendSync('set-download-option', true);
        w.loadURL(url + ":" + port + "/");
        return ipcRenderer.once('download-done', function(event, state, url, mimeType, receivedBytes, totalBytes, disposition, filename) {
          assert.equal(state, 'cancelled');
          assert.equal(filename, 'mock.pdf');
          assert.equal(mimeType, 'application/pdf');
          assert.equal(receivedBytes, 0);
          assert.equal(totalBytes, mockPDF.length);
          assert.equal(disposition, contentDisposition);
          return done();
        });
      });
    });
  });
});
