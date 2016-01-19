const assert = require('assert');
const child_process = require('child_process');
const fs = require('fs');
const path = require('path');

const nativeImage = require('electron').nativeImage;
const remote = require('electron').remote;

const ipcMain = remote.require('electron').ipcMain;
const BrowserWindow = remote.require('electron').BrowserWindow;

describe('asar package', function() {
  var fixtures;
  fixtures = path.join(__dirname, 'fixtures');
  describe('node api', function() {
    describe('fs.readFileSync', function() {
      it('does not leak fd', function() {
        var readCalls = 1;
        while(readCalls <= 10000) {
          fs.readFileSync(path.join(process.resourcesPath, 'atom.asar', 'renderer', 'api', 'lib', 'ipc.js'));
          readCalls++;
        }
      });
      it('reads a normal file', function() {
        var file1, file2, file3;
        file1 = path.join(fixtures, 'asar', 'a.asar', 'file1');
        assert.equal(fs.readFileSync(file1).toString().trim(), 'file1');
        file2 = path.join(fixtures, 'asar', 'a.asar', 'file2');
        assert.equal(fs.readFileSync(file2).toString().trim(), 'file2');
        file3 = path.join(fixtures, 'asar', 'a.asar', 'file3');
        return assert.equal(fs.readFileSync(file3).toString().trim(), 'file3');
      });
      it('reads from a empty file', function() {
        var buffer, file;
        file = path.join(fixtures, 'asar', 'empty.asar', 'file1');
        buffer = fs.readFileSync(file);
        assert.equal(buffer.length, 0);
        return assert.equal(buffer.toString(), '');
      });
      it('reads a linked file', function() {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link1');
        return assert.equal(fs.readFileSync(p).toString().trim(), 'file1');
      });
      it('reads a file from linked directory', function() {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'file1');
        assert.equal(fs.readFileSync(p).toString().trim(), 'file1');
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1');
        return assert.equal(fs.readFileSync(p).toString().trim(), 'file1');
      });
      it('throws ENOENT error when can not find file', function() {
        var p, throws;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        throws = function() {
          return fs.readFileSync(p);
        };
        return assert.throws(throws, /ENOENT/);
      });
      it('passes ENOENT error to callback when can not find file', function() {
        var async, p;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        async = false;
        fs.readFile(p, function(e) {
          assert(async);
          return assert(/ENOENT/.test(e));
        });
        return async = true;
      });
      return it('reads a normal file with unpacked files', function() {
        var p;
        p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt');
        return assert.equal(fs.readFileSync(p).toString().trim(), 'a');
      });
    });
    describe('fs.readFile', function() {
      it('reads a normal file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'file1');
        return fs.readFile(p, function(err, content) {
          assert.equal(err, null);
          assert.equal(String(content).trim(), 'file1');
          return done();
        });
      });
      it('reads from a empty file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'empty.asar', 'file1');
        return fs.readFile(p, function(err, content) {
          assert.equal(err, null);
          assert.equal(String(content), '');
          return done();
        });
      });
      it('reads a linked file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link1');
        return fs.readFile(p, function(err, content) {
          assert.equal(err, null);
          assert.equal(String(content).trim(), 'file1');
          return done();
        });
      });
      it('reads a file from linked directory', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1');
        return fs.readFile(p, function(err, content) {
          assert.equal(err, null);
          assert.equal(String(content).trim(), 'file1');
          return done();
        });
      });
      return it('throws ENOENT error when can not find file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        return fs.readFile(p, function(err) {
          assert.equal(err.code, 'ENOENT');
          return done();
        });
      });
    });
    describe('fs.lstatSync', function() {
      it('handles path with trailing slash correctly', function() {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1');
        fs.lstatSync(p);
        return fs.lstatSync(p + '/');
      });
      it('returns information of root', function() {
        var p, stats;
        p = path.join(fixtures, 'asar', 'a.asar');
        stats = fs.lstatSync(p);
        assert.equal(stats.isFile(), false);
        assert.equal(stats.isDirectory(), true);
        assert.equal(stats.isSymbolicLink(), false);
        return assert.equal(stats.size, 0);
      });
      it('returns information of a normal file', function() {
        var file, j, len, p, ref2, results, stats;
        ref2 = ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')];
        results = [];
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j];
          p = path.join(fixtures, 'asar', 'a.asar', file);
          stats = fs.lstatSync(p);
          assert.equal(stats.isFile(), true);
          assert.equal(stats.isDirectory(), false);
          assert.equal(stats.isSymbolicLink(), false);
          results.push(assert.equal(stats.size, 6));
        }
        return results;
      });
      it('returns information of a normal directory', function() {
        var file, j, len, p, ref2, results, stats;
        ref2 = ['dir1', 'dir2', 'dir3'];
        results = [];
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j];
          p = path.join(fixtures, 'asar', 'a.asar', file);
          stats = fs.lstatSync(p);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), true);
          assert.equal(stats.isSymbolicLink(), false);
          results.push(assert.equal(stats.size, 0));
        }
        return results;
      });
      it('returns information of a linked file', function() {
        var file, j, len, p, ref2, results, stats;
        ref2 = ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')];
        results = [];
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j];
          p = path.join(fixtures, 'asar', 'a.asar', file);
          stats = fs.lstatSync(p);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), false);
          assert.equal(stats.isSymbolicLink(), true);
          results.push(assert.equal(stats.size, 0));
        }
        return results;
      });
      it('returns information of a linked directory', function() {
        var file, j, len, p, ref2, results, stats;
        ref2 = ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')];
        results = [];
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j];
          p = path.join(fixtures, 'asar', 'a.asar', file);
          stats = fs.lstatSync(p);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), false);
          assert.equal(stats.isSymbolicLink(), true);
          results.push(assert.equal(stats.size, 0));
        }
        return results;
      });
      return it('throws ENOENT error when can not find file', function() {
        var file, j, len, p, ref2, results, throws;
        ref2 = ['file4', 'file5', path.join('dir1', 'file4')];
        results = [];
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j];
          p = path.join(fixtures, 'asar', 'a.asar', file);
          throws = function() {
            return fs.lstatSync(p);
          };
          results.push(assert.throws(throws, /ENOENT/));
        }
        return results;
      });
    });
    describe('fs.lstat', function() {
      it('handles path with trailing slash correctly', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1');
        return fs.lstat(p + '/', done);
      });
      it('returns information of root', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar');
        fs.lstat(p, function(err, stats) {
          assert.equal(err, null);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), true);
          assert.equal(stats.isSymbolicLink(), false);
          assert.equal(stats.size, 0);
          return done();
        });
      });
      it('returns information of a normal file', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'file1');
        fs.lstat(p, function(err, stats) {
          assert.equal(err, null);
          assert.equal(stats.isFile(), true);
          assert.equal(stats.isDirectory(), false);
          assert.equal(stats.isSymbolicLink(), false);
          assert.equal(stats.size, 6);
          return done();
        });
      });
      it('returns information of a normal directory', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1');
        fs.lstat(p, function(err, stats) {
          assert.equal(err, null);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), true);
          assert.equal(stats.isSymbolicLink(), false);
          assert.equal(stats.size, 0);
          return done();
        });
      });
      it('returns information of a linked file', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link1');
        fs.lstat(p, function(err, stats) {
          assert.equal(err, null);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), false);
          assert.equal(stats.isSymbolicLink(), true);
          assert.equal(stats.size, 0);
          return done();
        });
      });
      it('returns information of a linked directory', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2');
        fs.lstat(p, function(err, stats) {
          assert.equal(err, null);
          assert.equal(stats.isFile(), false);
          assert.equal(stats.isDirectory(), false);
          assert.equal(stats.isSymbolicLink(), true);
          assert.equal(stats.size, 0);
          return done();
        });
      });
      return it('throws ENOENT error when can not find file', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file4');
        fs.lstat(p, function(err) {
          assert.equal(err.code, 'ENOENT');
          return done();
        });
      });
    });
    describe('fs.realpathSync', function() {
      it('returns real path root', function() {
        var p, parent, r;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = 'a.asar';
        r = fs.realpathSync(path.join(parent, p));
        return assert.equal(r, path.join(parent, p));
      });
      it('returns real path of a normal file', function() {
        var p, parent, r;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'file1');
        r = fs.realpathSync(path.join(parent, p));
        return assert.equal(r, path.join(parent, p));
      });
      it('returns real path of a normal directory', function() {
        var p, parent, r;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'dir1');
        r = fs.realpathSync(path.join(parent, p));
        return assert.equal(r, path.join(parent, p));
      });
      it('returns real path of a linked file', function() {
        var p, parent, r;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'link2', 'link1');
        r = fs.realpathSync(path.join(parent, p));
        return assert.equal(r, path.join(parent, 'a.asar', 'file1'));
      });
      it('returns real path of a linked directory', function() {
        var p, parent, r;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'link2', 'link2');
        r = fs.realpathSync(path.join(parent, p));
        return assert.equal(r, path.join(parent, 'a.asar', 'dir1'));
      });
      return it('throws ENOENT error when can not find file', function() {
        var p, parent, throws;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'not-exist');
        throws = function() {
          return fs.realpathSync(path.join(parent, p));
        };
        return assert.throws(throws, /ENOENT/);
      });
    });
    describe('fs.realpath', function() {
      it('returns real path root', function(done) {
        var p, parent;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = 'a.asar';
        return fs.realpath(path.join(parent, p), function(err, r) {
          assert.equal(err, null);
          assert.equal(r, path.join(parent, p));
          return done();
        });
      });
      it('returns real path of a normal file', function(done) {
        var p, parent;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'file1');
        return fs.realpath(path.join(parent, p), function(err, r) {
          assert.equal(err, null);
          assert.equal(r, path.join(parent, p));
          return done();
        });
      });
      it('returns real path of a normal directory', function(done) {
        var p, parent;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'dir1');
        return fs.realpath(path.join(parent, p), function(err, r) {
          assert.equal(err, null);
          assert.equal(r, path.join(parent, p));
          return done();
        });
      });
      it('returns real path of a linked file', function(done) {
        var p, parent;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'link2', 'link1');
        return fs.realpath(path.join(parent, p), function(err, r) {
          assert.equal(err, null);
          assert.equal(r, path.join(parent, 'a.asar', 'file1'));
          return done();
        });
      });
      it('returns real path of a linked directory', function(done) {
        var p, parent;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'link2', 'link2');
        return fs.realpath(path.join(parent, p), function(err, r) {
          assert.equal(err, null);
          assert.equal(r, path.join(parent, 'a.asar', 'dir1'));
          return done();
        });
      });
      return it('throws ENOENT error when can not find file', function(done) {
        var p, parent;
        parent = fs.realpathSync(path.join(fixtures, 'asar'));
        p = path.join('a.asar', 'not-exist');
        return fs.realpath(path.join(parent, p), function(err) {
          assert.equal(err.code, 'ENOENT');
          return done();
        });
      });
    });
    describe('fs.readdirSync', function() {
      it('reads dirs from root', function() {
        var dirs, p;
        p = path.join(fixtures, 'asar', 'a.asar');
        dirs = fs.readdirSync(p);
        return assert.deepEqual(dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });
      it('reads dirs from a normal dir', function() {
        var dirs, p;
        p = path.join(fixtures, 'asar', 'a.asar', 'dir1');
        dirs = fs.readdirSync(p);
        return assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2']);
      });
      it('reads dirs from a linked dir', function() {
        var dirs, p;
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2');
        dirs = fs.readdirSync(p);
        return assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2']);
      });
      return it('throws ENOENT error when can not find file', function() {
        var p, throws;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        throws = function() {
          return fs.readdirSync(p);
        };
        return assert.throws(throws, /ENOENT/);
      });
    });
    describe('fs.readdir', function() {
      it('reads dirs from root', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar');
        fs.readdir(p, function(err, dirs) {
          assert.equal(err, null);
          assert.deepEqual(dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
          return done();
        });
      });
      it('reads dirs from a normal dir', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1');
        fs.readdir(p, function(err, dirs) {
          assert.equal(err, null);
          assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2']);
          return done();
        });
      });
      it('reads dirs from a linked dir', function(done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2');
        fs.readdir(p, function(err, dirs) {
          assert.equal(err, null);
          assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2']);
          return done();
        });
      });
      return it('throws ENOENT error when can not find file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        return fs.readdir(p, function(err) {
          assert.equal(err.code, 'ENOENT');
          return done();
        });
      });
    });
    describe('fs.openSync', function() {
      it('opens a normal/linked/under-linked-directory file', function() {
        var buffer, fd, file, j, len, p, ref2, results;
        ref2 = ['file1', 'link1', path.join('link2', 'file1')];
        results = [];
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j];
          p = path.join(fixtures, 'asar', 'a.asar', file);
          fd = fs.openSync(p, 'r');
          buffer = new Buffer(6);
          fs.readSync(fd, buffer, 0, 6, 0);
          assert.equal(String(buffer).trim(), 'file1');
          results.push(fs.closeSync(fd));
        }
        return results;
      });
      return it('throws ENOENT error when can not find file', function() {
        var p, throws;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        throws = function() {
          return fs.openSync(p);
        };
        return assert.throws(throws, /ENOENT/);
      });
    });
    describe('fs.open', function() {
      it('opens a normal file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'file1');
        return fs.open(p, 'r', function(err, fd) {
          var buffer;
          assert.equal(err, null);
          buffer = new Buffer(6);
          return fs.read(fd, buffer, 0, 6, 0, function(err) {
            assert.equal(err, null);
            assert.equal(String(buffer).trim(), 'file1');
            return fs.close(fd, done);
          });
        });
      });
      return it('throws ENOENT error when can not find file', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        return fs.open(p, 'r', function(err) {
          assert.equal(err.code, 'ENOENT');
          return done();
        });
      });
    });
    describe('fs.mkdir', function() {
      return it('throws error when calling inside asar archive', function(done) {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        return fs.mkdir(p, function(err) {
          assert.equal(err.code, 'ENOTDIR');
          return done();
        });
      });
    });
    describe('fs.mkdirSync', function() {
      return it('throws error when calling inside asar archive', function() {
        var p;
        p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
        return assert.throws((function() {
          return fs.mkdirSync(p);
        }), new RegExp('ENOTDIR'));
      });
    });
    describe('child_process.fork', function() {
      child_process = require('child_process');
      it('opens a normal js file', function(done) {
        var child;
        child = child_process.fork(path.join(fixtures, 'asar', 'a.asar', 'ping.js'));
        child.on('message', function(msg) {
          assert.equal(msg, 'message');
          return done();
        });
        return child.send('message');
      });
      return it('supports asar in the forked js', function(done) {
        var child, file;
        file = path.join(fixtures, 'asar', 'a.asar', 'file1');
        child = child_process.fork(path.join(fixtures, 'module', 'asar.js'));
        child.on('message', function(content) {
          assert.equal(content, fs.readFileSync(file).toString());
          return done();
        });
        return child.send(file);
      });
    });
    describe('child_process.execFile', function() {
      var echo, execFile, execFileSync, ref2;
      if (process.platform !== 'darwin') {
        return;
      }
      ref2 = require('child_process'), execFile = ref2.execFile, execFileSync = ref2.execFileSync;
      echo = path.join(fixtures, 'asar', 'echo.asar', 'echo');
      it('executes binaries', function(done) {
        execFile(echo, ['test'], function(error, stdout) {
          assert.equal(error, null);
          assert.equal(stdout, 'test\n');
          return done();
        });
      });
      return xit('execFileSync executes binaries', function() {
        var output;
        output = execFileSync(echo, ['test']);
        return assert.equal(String(output), 'test\n');
      });
    });
    describe('internalModuleReadFile', function() {
      var internalModuleReadFile;
      internalModuleReadFile = process.binding('fs').internalModuleReadFile;
      it('read a normal file', function() {
        var file1, file2, file3;
        file1 = path.join(fixtures, 'asar', 'a.asar', 'file1');
        assert.equal(internalModuleReadFile(file1).toString().trim(), 'file1');
        file2 = path.join(fixtures, 'asar', 'a.asar', 'file2');
        assert.equal(internalModuleReadFile(file2).toString().trim(), 'file2');
        file3 = path.join(fixtures, 'asar', 'a.asar', 'file3');
        return assert.equal(internalModuleReadFile(file3).toString().trim(), 'file3');
      });
      return it('reads a normal file with unpacked files', function() {
        var p;
        p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt');
        return assert.equal(internalModuleReadFile(p).toString().trim(), 'a');
      });
    });
    return describe('process.noAsar', function() {
      var errorName;
      errorName = process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR';
      beforeEach(function() {
        return process.noAsar = true;
      });
      afterEach(function() {
        return process.noAsar = false;
      });
      it('disables asar support in sync API', function() {
        var dir, file;
        file = path.join(fixtures, 'asar', 'a.asar', 'file1');
        dir = path.join(fixtures, 'asar', 'a.asar', 'dir1');
        assert.throws((function() {
          return fs.readFileSync(file);
        }), new RegExp(errorName));
        assert.throws((function() {
          return fs.lstatSync(file);
        }), new RegExp(errorName));
        assert.throws((function() {
          return fs.realpathSync(file);
        }), new RegExp(errorName));
        return assert.throws((function() {
          return fs.readdirSync(dir);
        }), new RegExp(errorName));
      });
      it('disables asar support in async API', function(done) {
        var dir, file;
        file = path.join(fixtures, 'asar', 'a.asar', 'file1');
        dir = path.join(fixtures, 'asar', 'a.asar', 'dir1');
        return fs.readFile(file, function(error) {
          assert.equal(error.code, errorName);
          return fs.lstat(file, function(error) {
            assert.equal(error.code, errorName);
            return fs.realpath(file, function(error) {
              assert.equal(error.code, errorName);
              return fs.readdir(dir, function(error) {
                assert.equal(error.code, errorName);
                return done();
              });
            });
          });
        });
      });
      return it('treats *.asar as normal file', function() {
        var asar, content1, content2, originalFs;
        originalFs = require('original-fs');
        asar = path.join(fixtures, 'asar', 'a.asar');
        content1 = fs.readFileSync(asar);
        content2 = originalFs.readFileSync(asar);
        assert.equal(content1.compare(content2), 0);
        return assert.throws((function() {
          return fs.readdirSync(asar);
        }), /ENOTDIR/);
      });
    });
  });
  describe('asar protocol', function() {
    var url;
    url = require('url');
    it('can request a file in package', function(done) {
      var p;
      p = path.resolve(fixtures, 'asar', 'a.asar', 'file1');
      return $.get("file://" + p, function(data) {
        assert.equal(data.trim(), 'file1');
        return done();
      });
    });
    it('can request a file in package with unpacked files', function(done) {
      var p;
      p = path.resolve(fixtures, 'asar', 'unpack.asar', 'a.txt');
      return $.get("file://" + p, function(data) {
        assert.equal(data.trim(), 'a');
        return done();
      });
    });
    it('can request a linked file in package', function(done) {
      var p;
      p = path.resolve(fixtures, 'asar', 'a.asar', 'link2', 'link1');
      return $.get("file://" + p, function(data) {
        assert.equal(data.trim(), 'file1');
        return done();
      });
    });
    it('can request a file in filesystem', function(done) {
      var p;
      p = path.resolve(fixtures, 'asar', 'file');
      return $.get("file://" + p, function(data) {
        assert.equal(data.trim(), 'file');
        return done();
      });
    });
    it('gets 404 when file is not found', function(done) {
      var p;
      p = path.resolve(fixtures, 'asar', 'a.asar', 'no-exist');
      return $.ajax({
        url: "file://" + p,
        error: function(err) {
          assert.equal(err.status, 404);
          return done();
        }
      });
    });
    it('sets __dirname correctly', function(done) {
      var p, u, w;
      after(function() {
        w.destroy();
        return ipcMain.removeAllListeners('dirname');
      });
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      });
      p = path.resolve(fixtures, 'asar', 'web.asar', 'index.html');
      u = url.format({
        protocol: 'file',
        slashed: true,
        pathname: p
      });
      ipcMain.once('dirname', function(event, dirname) {
        assert.equal(dirname, path.dirname(p));
        return done();
      });
      return w.loadURL(u);
    });
    return it('loads script tag in html', function(done) {
      var p, u, w;
      after(function() {
        w.destroy();
        return ipcMain.removeAllListeners('ping');
      });
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      });
      p = path.resolve(fixtures, 'asar', 'script.asar', 'index.html');
      u = url.format({
        protocol: 'file',
        slashed: true,
        pathname: p
      });
      w.loadURL(u);
      return ipcMain.once('ping', function(event, message) {
        assert.equal(message, 'pong');
        return done();
      });
    });
  });
  describe('original-fs module', function() {
    var originalFs;
    originalFs = require('original-fs');
    it('treats .asar as file', function() {
      var file, stats;
      file = path.join(fixtures, 'asar', 'a.asar');
      stats = originalFs.statSync(file);
      return assert(stats.isFile());
    });
    return it('is available in forked scripts', function(done) {
      var child;
      child = child_process.fork(path.join(fixtures, 'module', 'original-fs.js'));
      child.on('message', function(msg) {
        assert.equal(msg, 'object');
        return done();
      });
      return child.send('message');
    });
  });
  describe('graceful-fs module', function() {
    var gfs;
    gfs = require('graceful-fs');
    it('recognize asar archvies', function() {
      var p;
      p = path.join(fixtures, 'asar', 'a.asar', 'link1');
      return assert.equal(gfs.readFileSync(p).toString().trim(), 'file1');
    });
    return it('does not touch global fs object', function() {
      return assert.notEqual(fs.readdir, gfs.readdir);
    });
  });
  describe('mkdirp module', function() {
    var mkdirp;
    mkdirp = require('mkdirp');
    return it('throws error when calling inside asar archive', function() {
      var p;
      p = path.join(fixtures, 'asar', 'a.asar', 'not-exist');
      return assert.throws((function() {
        return mkdirp.sync(p);
      }), new RegExp('ENOTDIR'));
    });
  });
  return describe('native-image', function() {
    it('reads image from asar archive', function() {
      var logo, p;
      p = path.join(fixtures, 'asar', 'logo.asar', 'logo.png');
      logo = nativeImage.createFromPath(p);
      return assert.deepEqual(logo.getSize(), {
        width: 55,
        height: 55
      });
    });
    return it('reads image from asar archive with unpacked files', function() {
      var logo, p;
      p = path.join(fixtures, 'asar', 'unpack.asar', 'atom.png');
      logo = nativeImage.createFromPath(p);
      return assert.deepEqual(logo.getSize(), {
        width: 1024,
        height: 1024
      });
    });
  });
});
