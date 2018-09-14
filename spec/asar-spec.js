const assert = require('assert')
const ChildProcess = require('child_process')
const { expect } = require('chai')
const fs = require('fs')
const path = require('path')
const util = require('util')
const { closeWindow } = require('./window-helpers')

const nativeImage = require('electron').nativeImage
const remote = require('electron').remote

const ipcMain = remote.require('electron').ipcMain
const BrowserWindow = remote.require('electron').BrowserWindow

describe('asar package', function () {
  var fixtures = path.join(__dirname, 'fixtures')

  describe('node api', function () {
    it('supports paths specified as a Buffer', function () {
      var file = Buffer.from(path.join(fixtures, 'asar', 'a.asar', 'file1'))
      assert.strictEqual(fs.existsSync(file), true)
    })

    describe('fs.readFileSync', function () {
      it('does not leak fd', function () {
        var readCalls = 1
        while (readCalls <= 10000) {
          fs.readFileSync(path.join(process.resourcesPath, 'electron.asar', 'renderer', 'api', 'ipc-renderer.js'))
          readCalls++
        }
      })

      it('reads a normal file', function () {
        var file1 = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.strictEqual(fs.readFileSync(file1).toString().trim(), 'file1')
        var file2 = path.join(fixtures, 'asar', 'a.asar', 'file2')
        assert.strictEqual(fs.readFileSync(file2).toString().trim(), 'file2')
        var file3 = path.join(fixtures, 'asar', 'a.asar', 'file3')
        assert.strictEqual(fs.readFileSync(file3).toString().trim(), 'file3')
      })

      it('reads from a empty file', function () {
        var file = path.join(fixtures, 'asar', 'empty.asar', 'file1')
        var buffer = fs.readFileSync(file)
        assert.strictEqual(buffer.length, 0)
        assert.strictEqual(buffer.toString(), '')
      })

      it('reads a linked file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link1')
        assert.strictEqual(fs.readFileSync(p).toString().trim(), 'file1')
      })

      it('reads a file from linked directory', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'file1')
        assert.strictEqual(fs.readFileSync(p).toString().trim(), 'file1')
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1')
        assert.strictEqual(fs.readFileSync(p).toString().trim(), 'file1')
      })

      it('throws ENOENT error when can not find file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        var throws = function () {
          fs.readFileSync(p)
        }
        assert.throws(throws, /ENOENT/)
      })

      it('passes ENOENT error to callback when can not find file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        var async = false
        fs.readFile(p, function (e) {
          assert(async)
          assert(/ENOENT/.test(e))
        })
        async = true
      })

      it('reads a normal file with unpacked files', function () {
        var p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        assert.strictEqual(fs.readFileSync(p).toString().trim(), 'a')
      })
    })

    describe('fs.readFile', function () {
      it('reads a normal file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        fs.readFile(p, function (err, content) {
          assert.strictEqual(err, null)
          assert.strictEqual(String(content).trim(), 'file1')
          done()
        })
      })

      it('reads from a empty file', function (done) {
        var p = path.join(fixtures, 'asar', 'empty.asar', 'file1')
        fs.readFile(p, function (err, content) {
          assert.strictEqual(err, null)
          assert.strictEqual(String(content), '')
          done()
        })
      })

      it('reads from a empty file with encoding', function (done) {
        var p = path.join(fixtures, 'asar', 'empty.asar', 'file1')
        fs.readFile(p, 'utf8', function (err, content) {
          assert.strictEqual(err, null)
          assert.strictEqual(content, '')
          done()
        })
      })

      it('reads a linked file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link1')
        fs.readFile(p, function (err, content) {
          assert.strictEqual(err, null)
          assert.strictEqual(String(content).trim(), 'file1')
          done()
        })
      })

      it('reads a file from linked directory', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1')
        fs.readFile(p, function (err, content) {
          assert.strictEqual(err, null)
          assert.strictEqual(String(content).trim(), 'file1')
          done()
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.readFile(p, function (err) {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.lstatSync', function () {
      it('handles path with trailing slash correctly', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1')
        fs.lstatSync(p)
        fs.lstatSync(p + '/')
      })

      it('returns information of root', function () {
        var p = path.join(fixtures, 'asar', 'a.asar')
        var stats = fs.lstatSync(p)
        assert.strictEqual(stats.isFile(), false)
        assert.strictEqual(stats.isDirectory(), true)
        assert.strictEqual(stats.isSymbolicLink(), false)
        assert.strictEqual(stats.size, 0)
      })

      it('returns information of a normal file', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.strictEqual(stats.isFile(), true)
          assert.strictEqual(stats.isDirectory(), false)
          assert.strictEqual(stats.isSymbolicLink(), false)
          assert.strictEqual(stats.size, 6)
        }
      })

      it('returns information of a normal directory', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['dir1', 'dir2', 'dir3']
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), true)
          assert.strictEqual(stats.isSymbolicLink(), false)
          assert.strictEqual(stats.size, 0)
        }
      })

      it('returns information of a linked file', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), false)
          assert.strictEqual(stats.isSymbolicLink(), true)
          assert.strictEqual(stats.size, 0)
        }
      })

      it('returns information of a linked directory', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), false)
          assert.strictEqual(stats.isSymbolicLink(), true)
          assert.strictEqual(stats.size, 0)
        }
      })

      it('throws ENOENT error when can not find file', function () {
        var file, j, len, p, ref2, throws
        ref2 = ['file4', 'file5', path.join('dir1', 'file4')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          throws = function () {
            fs.lstatSync(p)
          }
          assert.throws(throws, /ENOENT/)
        }
      })
    })

    describe('fs.lstat', function () {
      it('handles path with trailing slash correctly', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1')
        fs.lstat(p + '/', done)
      })

      it('returns information of root', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar')
        fs.lstat(p, function (err, stats) {
          assert.strictEqual(err, null)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), true)
          assert.strictEqual(stats.isSymbolicLink(), false)
          assert.strictEqual(stats.size, 0)
          done()
        })
      })

      it('returns information of a normal file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'file1')
        fs.lstat(p, function (err, stats) {
          assert.strictEqual(err, null)
          assert.strictEqual(stats.isFile(), true)
          assert.strictEqual(stats.isDirectory(), false)
          assert.strictEqual(stats.isSymbolicLink(), false)
          assert.strictEqual(stats.size, 6)
          done()
        })
      })

      it('returns information of a normal directory', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        fs.lstat(p, function (err, stats) {
          assert.strictEqual(err, null)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), true)
          assert.strictEqual(stats.isSymbolicLink(), false)
          assert.strictEqual(stats.size, 0)
          done()
        })
      })

      it('returns information of a linked file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link1')
        fs.lstat(p, function (err, stats) {
          assert.strictEqual(err, null)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), false)
          assert.strictEqual(stats.isSymbolicLink(), true)
          assert.strictEqual(stats.size, 0)
          done()
        })
      })

      it('returns information of a linked directory', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2')
        fs.lstat(p, function (err, stats) {
          assert.strictEqual(err, null)
          assert.strictEqual(stats.isFile(), false)
          assert.strictEqual(stats.isDirectory(), false)
          assert.strictEqual(stats.isSymbolicLink(), true)
          assert.strictEqual(stats.size, 0)
          done()
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file4')
        fs.lstat(p, function (err) {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.realpathSync', () => {
      it('returns real path root', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = 'a.asar'
        const r = fs.realpathSync(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('returns real path of a normal file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        const r = fs.realpathSync(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('returns real path of a normal directory', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        const r = fs.realpathSync(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('returns real path of a linked file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        const r = fs.realpathSync(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, 'a.asar', 'file1'))
      })

      it('returns real path of a linked directory', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        const r = fs.realpathSync(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, 'a.asar', 'dir1'))
      })

      it('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        const r = fs.realpathSync(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'not-exist')
        const throws = () => fs.realpathSync(path.join(parent, p))
        assert.throws(throws, /ENOENT/)
      })
    })

    describe('fs.realpathSync.native', () => {
      it('returns real path root', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = 'a.asar'
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('returns real path of a normal file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('returns real path of a normal directory', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('returns real path of a linked file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, 'a.asar', 'file1'))
      })

      it('returns real path of a linked directory', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, 'a.asar', 'dir1'))
      })

      it('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.strictEqual(r, path.join(parent, p))
      })

      it('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'not-exist')
        const throws = () => fs.realpathSync.native(path.join(parent, p))
        assert.throws(throws, /ENOENT/)
      })
    })

    describe('fs.realpath', () => {
      it('returns real path root', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = 'a.asar'
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal directory', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a linked file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, 'a.asar', 'file1'))
          done()
        })
      })

      it('returns real path of a linked directory', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, 'a.asar', 'dir1'))
          done()
        })
      })

      it('returns real path of an unpacked file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('throws ENOENT error when can not find file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'not-exist')
        fs.realpath(path.join(parent, p), err => {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.realpath.native', () => {
      it('returns real path root', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = 'a.asar'
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal directory', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a linked file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, 'a.asar', 'file1'))
          done()
        })
      })

      it('returns real path of a linked directory', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, 'a.asar', 'dir1'))
          done()
        })
      })

      it('returns real path of an unpacked file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.strictEqual(err, null)
          assert.strictEqual(r, path.join(parent, p))
          done()
        })
      })

      it('throws ENOENT error when can not find file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'not-exist')
        fs.realpath.native(path.join(parent, p), err => {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.readdirSync', function () {
      it('reads dirs from root', function () {
        var p = path.join(fixtures, 'asar', 'a.asar')
        var dirs = fs.readdirSync(p)
        assert.deepStrictEqual(dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js'])
      })

      it('reads dirs from a normal dir', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        var dirs = fs.readdirSync(p)
        assert.deepStrictEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
      })

      it('reads dirs from a linked dir', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2')
        var dirs = fs.readdirSync(p)
        assert.deepStrictEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
      })

      it('throws ENOENT error when can not find file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        var throws = function () {
          fs.readdirSync(p)
        }
        assert.throws(throws, /ENOENT/)
      })
    })

    describe('fs.readdir', function () {
      it('reads dirs from root', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar')
        fs.readdir(p, function (err, dirs) {
          assert.strictEqual(err, null)
          assert.deepStrictEqual(dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js'])
          done()
        })
      })

      it('reads dirs from a normal dir', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        fs.readdir(p, function (err, dirs) {
          assert.strictEqual(err, null)
          assert.deepStrictEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
          done()
        })
      })
      it('reads dirs from a linked dir', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2')
        fs.readdir(p, function (err, dirs) {
          assert.strictEqual(err, null)
          assert.deepStrictEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
          done()
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.readdir(p, function (err) {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.openSync', function () {
      it('opens a normal/linked/under-linked-directory file', function () {
        var buffer, fd, file, j, len, p, ref2
        ref2 = ['file1', 'link1', path.join('link2', 'file1')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          fd = fs.openSync(p, 'r')
          buffer = Buffer.alloc(6)
          fs.readSync(fd, buffer, 0, 6, 0)
          assert.strictEqual(String(buffer).trim(), 'file1')
          fs.closeSync(fd)
        }
      })

      it('throws ENOENT error when can not find file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        var throws = function () {
          fs.openSync(p)
        }
        assert.throws(throws, /ENOENT/)
      })
    })

    describe('fs.open', function () {
      it('opens a normal file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        fs.open(p, 'r', function (err, fd) {
          assert.strictEqual(err, null)
          var buffer = Buffer.alloc(6)
          fs.read(fd, buffer, 0, 6, 0, function (err) {
            assert.strictEqual(err, null)
            assert.strictEqual(String(buffer).trim(), 'file1')
            fs.close(fd, done)
          })
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.open(p, 'r', function (err) {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.mkdir', function () {
      it('throws error when calling inside asar archive', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.mkdir(p, function (err) {
          assert.strictEqual(err.code, 'ENOTDIR')
          done()
        })
      })
    })

    describe('fs.mkdirSync', function () {
      it('throws error when calling inside asar archive', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        assert.throws(function () {
          fs.mkdirSync(p)
        }, new RegExp('ENOTDIR'))
      })
    })

    describe('fs.exists', function () {
      it('handles an existing file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        // eslint-disable-next-line
        fs.exists(p, function (exists) {
          assert.strictEqual(exists, true)
          done()
        })
      })

      it('handles a non-existent file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        // eslint-disable-next-line
        fs.exists(p, function (exists) {
          assert.strictEqual(exists, false)
          done()
        })
      })

      it('promisified version handles an existing file', (done) => {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        // eslint-disable-next-line
        util.promisify(fs.exists)(p).then(exists => {
          assert.strictEqual(exists, true)
          done()
        })
      })

      it('promisified version handles a non-existent file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        // eslint-disable-next-line
        util.promisify(fs.exists)(p).then(exists => {
          assert.strictEqual(exists, false)
          done()
        })
      })
    })

    describe('fs.existsSync', function () {
      it('handles an existing file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.doesNotThrow(function () {
          assert.strictEqual(fs.existsSync(p), true)
        })
      })

      it('handles a non-existent file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        assert.doesNotThrow(function () {
          assert.strictEqual(fs.existsSync(p), false)
        })
      })
    })

    describe('fs.access', function () {
      it('accesses a normal file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        fs.access(p, function (err) {
          assert(err == null)
          done()
        })
      })

      it('throws an error when called with write mode', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        fs.access(p, fs.constants.R_OK | fs.constants.W_OK, function (err) {
          assert.strictEqual(err.code, 'EACCES')
          done()
        })
      })

      it('throws an error when called on non-existent file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.access(p, function (err) {
          assert.strictEqual(err.code, 'ENOENT')
          done()
        })
      })

      it('allows write mode for unpacked files', function (done) {
        var p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        fs.access(p, fs.constants.R_OK | fs.constants.W_OK, function (err) {
          assert(err == null)
          done()
        })
      })
    })

    describe('fs.accessSync', function () {
      it('accesses a normal file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.doesNotThrow(function () {
          fs.accessSync(p)
        })
      })

      it('throws an error when called with write mode', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.throws(function () {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK)
        }, /EACCES/)
      })

      it('throws an error when called on non-existent file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        assert.throws(function () {
          fs.accessSync(p)
        }, /ENOENT/)
      })

      it('allows write mode for unpacked files', function () {
        var p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        assert.doesNotThrow(function () {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK)
        })
      })
    })

    describe('child_process.fork', function () {
      it('opens a normal js file', function (done) {
        var child = ChildProcess.fork(path.join(fixtures, 'asar', 'a.asar', 'ping.js'))
        child.on('message', function (msg) {
          assert.strictEqual(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('supports asar in the forked js', function (done) {
        var file = path.join(fixtures, 'asar', 'a.asar', 'file1')
        var child = ChildProcess.fork(path.join(fixtures, 'module', 'asar.js'))
        child.on('message', function (content) {
          assert.strictEqual(content, fs.readFileSync(file).toString())
          done()
        })
        child.send(file)
      })
    })

    describe('child_process.exec', function () {
      var echo = path.join(fixtures, 'asar', 'echo.asar', 'echo')

      it('should not try to extract the command if there is a reference to a file inside an .asar', function (done) {
        ChildProcess.exec('echo ' + echo + ' foo bar', function (error, stdout) {
          assert.strictEqual(error, null)
          assert.strictEqual(stdout.toString().replace(/\r/g, ''), echo + ' foo bar\n')
          done()
        })
      })

      it('can be promisified', () => {
        return util.promisify(ChildProcess.exec)('echo ' + echo + ' foo bar').then(({ stdout }) => {
          assert.strictEqual(stdout.toString().replace(/\r/g, ''), echo + ' foo bar\n')
        })
      })
    })

    describe('child_process.execSync', function () {
      var echo = path.join(fixtures, 'asar', 'echo.asar', 'echo')

      it('should not try to extract the command if there is a reference to a file inside an .asar', function (done) {
        var stdout = ChildProcess.execSync('echo ' + echo + ' foo bar')
        assert.strictEqual(stdout.toString().replace(/\r/g, ''), echo + ' foo bar\n')
        done()
      })
    })

    describe('child_process.execFile', function () {
      var echo, execFile, execFileSync
      execFile = ChildProcess.execFile
      execFileSync = ChildProcess.execFileSync
      echo = path.join(fixtures, 'asar', 'echo.asar', 'echo')

      before(function () {
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('executes binaries', function (done) {
        execFile(echo, ['test'], function (error, stdout) {
          assert.strictEqual(error, null)
          assert.strictEqual(stdout, 'test\n')
          done()
        })
      })

      xit('execFileSync executes binaries', function () {
        var output = execFileSync(echo, ['test'])
        assert.strictEqual(String(output), 'test\n')
      })

      it('can be promisified', () => {
        return util.promisify(ChildProcess.execFile)(echo, ['test']).then(({ stdout }) => {
          assert.strictEqual(stdout, 'test\n')
        })
      })
    })

    describe('internalModuleReadJSON', function () {
      var internalModuleReadJSON = process.binding('fs').internalModuleReadJSON

      it('read a normal file', function () {
        var file1 = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.strictEqual(internalModuleReadJSON(file1).toString().trim(), 'file1')
        var file2 = path.join(fixtures, 'asar', 'a.asar', 'file2')
        assert.strictEqual(internalModuleReadJSON(file2).toString().trim(), 'file2')
        var file3 = path.join(fixtures, 'asar', 'a.asar', 'file3')
        assert.strictEqual(internalModuleReadJSON(file3).toString().trim(), 'file3')
      })

      it('reads a normal file with unpacked files', function () {
        var p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        assert.strictEqual(internalModuleReadJSON(p).toString().trim(), 'a')
      })
    })

    describe('util.promisify', function () {
      it('can promisify all fs functions', function () {
        const originalFs = require('original-fs')
        const { hasOwnProperty } = Object.prototype

        for (const [propertyName, originalValue] of Object.entries(originalFs)) {
          // Some properties exist but have a value of `undefined` on some platforms.
          // E.g. `fs.lchmod`, which in only available on MacOS, see
          // https://nodejs.org/docs/latest-v10.x/api/fs.html#fs_fs_lchmod_path_mode_callback
          // Also check for `null`s, `hasOwnProperty()` can't handle them.
          if (typeof originalValue === 'undefined' || originalValue === null) continue

          if (hasOwnProperty.call(originalValue, util.promisify.custom)) {
            expect(fs).to.have.own.property(propertyName)
              .that.has.own.property(util.promisify.custom)
          }
        }
      })
    })

    describe('process.noAsar', function () {
      var errorName = process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR'

      beforeEach(function () {
        process.noAsar = true
      })

      afterEach(function () {
        process.noAsar = false
      })

      it('disables asar support in sync API', function () {
        var file = path.join(fixtures, 'asar', 'a.asar', 'file1')
        var dir = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        assert.throws(function () {
          fs.readFileSync(file)
        }, new RegExp(errorName))
        assert.throws(function () {
          fs.lstatSync(file)
        }, new RegExp(errorName))
        assert.throws(function () {
          fs.realpathSync(file)
        }, new RegExp(errorName))
        assert.throws(function () {
          fs.readdirSync(dir)
        }, new RegExp(errorName))
      })

      it('disables asar support in async API', function (done) {
        var file = path.join(fixtures, 'asar', 'a.asar', 'file1')
        var dir = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        fs.readFile(file, function (error) {
          assert.strictEqual(error.code, errorName)
          fs.lstat(file, function (error) {
            assert.strictEqual(error.code, errorName)
            fs.realpath(file, function (error) {
              assert.strictEqual(error.code, errorName)
              fs.readdir(dir, function (error) {
                assert.strictEqual(error.code, errorName)
                done()
              })
            })
          })
        })
      })

      it('treats *.asar as normal file', function () {
        var originalFs = require('original-fs')
        var asar = path.join(fixtures, 'asar', 'a.asar')
        var content1 = fs.readFileSync(asar)
        var content2 = originalFs.readFileSync(asar)
        assert.strictEqual(content1.compare(content2), 0)
        assert.throws(function () {
          fs.readdirSync(asar)
        }, /ENOTDIR/)
      })

      it('is reset to its original value when execSync throws an error', function () {
        process.noAsar = false
        assert.throws(function () {
          ChildProcess.execSync(path.join(__dirname, 'does-not-exist.txt'))
        })
        assert.strictEqual(process.noAsar, false)
      })
    })

    describe('process.env.ELECTRON_NO_ASAR', function () {
      it('disables asar support in forked processes', function (done) {
        const forked = ChildProcess.fork(path.join(__dirname, 'fixtures', 'module', 'no-asar.js'), [], {
          env: {
            ELECTRON_NO_ASAR: true
          }
        })
        forked.on('message', function (stats) {
          assert.strictEqual(stats.isFile, true)
          assert.strictEqual(stats.size, 778)
          done()
        })
      })

      it('disables asar support in spawned processes', function (done) {
        const spawned = ChildProcess.spawn(process.execPath, [path.join(__dirname, 'fixtures', 'module', 'no-asar.js')], {
          env: {
            ELECTRON_NO_ASAR: true,
            ELECTRON_RUN_AS_NODE: true
          }
        })

        let output = ''
        spawned.stdout.on('data', function (data) {
          output += data
        })
        spawned.stdout.on('close', function () {
          const stats = JSON.parse(output)
          assert.strictEqual(stats.isFile, true)
          assert.strictEqual(stats.size, 778)
          done()
        })
      })
    })
  })

  describe('asar protocol', function () {
    var w = null

    afterEach(function () {
      return closeWindow(w).then(function () { w = null })
    })

    it('can request a file in package', function (done) {
      var p = path.resolve(fixtures, 'asar', 'a.asar', 'file1')
      $.get('file://' + p, function (data) {
        assert.strictEqual(data.trim(), 'file1')
        done()
      })
    })

    it('can request a file in package with unpacked files', function (done) {
      var p = path.resolve(fixtures, 'asar', 'unpack.asar', 'a.txt')
      $.get('file://' + p, function (data) {
        assert.strictEqual(data.trim(), 'a')
        done()
      })
    })

    it('can request a linked file in package', function (done) {
      var p = path.resolve(fixtures, 'asar', 'a.asar', 'link2', 'link1')
      $.get('file://' + p, function (data) {
        assert.strictEqual(data.trim(), 'file1')
        done()
      })
    })

    it('can request a file in filesystem', function (done) {
      var p = path.resolve(fixtures, 'asar', 'file')
      $.get('file://' + p, function (data) {
        assert.strictEqual(data.trim(), 'file')
        done()
      })
    })

    it('gets 404 when file is not found', function (done) {
      var p = path.resolve(fixtures, 'asar', 'a.asar', 'no-exist')
      $.ajax({
        url: 'file://' + p,
        error: function (err) {
          assert.strictEqual(err.status, 404)
          done()
        }
      })
    })

    it('sets __dirname correctly', function (done) {
      after(function () {
        ipcMain.removeAllListeners('dirname')
      })

      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
      var p = path.resolve(fixtures, 'asar', 'web.asar', 'index.html')
      ipcMain.once('dirname', function (event, dirname) {
        assert.strictEqual(dirname, path.dirname(p))
        done()
      })
      w.loadFile(p)
    })

    it('loads script tag in html', function (done) {
      after(function () {
        ipcMain.removeAllListeners('ping')
      })

      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
      var p = path.resolve(fixtures, 'asar', 'script.asar', 'index.html')
      w.loadFile(p)
      ipcMain.once('ping', function (event, message) {
        assert.strictEqual(message, 'pong')
        done()
      })
    })

    it('loads video tag in html', function (done) {
      this.timeout(60000)

      after(function () {
        ipcMain.removeAllListeners('asar-video')
      })

      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
      var p = path.resolve(fixtures, 'asar', 'video.asar', 'index.html')
      w.loadFile(p)
      ipcMain.on('asar-video', function (event, message, error) {
        if (message === 'ended') {
          assert(!error)
          done()
        } else if (message === 'error') {
          done(error)
        }
      })
    })
  })

  describe('original-fs module', function () {
    var originalFs = require('original-fs')

    it('treats .asar as file', function () {
      var file = path.join(fixtures, 'asar', 'a.asar')
      var stats = originalFs.statSync(file)
      assert(stats.isFile())
    })

    it('is available in forked scripts', function (done) {
      var child = ChildProcess.fork(path.join(fixtures, 'module', 'original-fs.js'))
      child.on('message', function (msg) {
        assert.strictEqual(msg, 'object')
        done()
      })
      child.send('message')
    })
  })

  describe('graceful-fs module', function () {
    var gfs = require('graceful-fs')

    it('recognize asar archvies', function () {
      var p = path.join(fixtures, 'asar', 'a.asar', 'link1')
      assert.strictEqual(gfs.readFileSync(p).toString().trim(), 'file1')
    })
    it('does not touch global fs object', function () {
      assert.notStrictEqual(fs.readdir, gfs.readdir)
    })
  })

  describe('mkdirp module', function () {
    var mkdirp = require('mkdirp')

    it('throws error when calling inside asar archive', function () {
      var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
      assert.throws(function () {
        mkdirp.sync(p)
      }, new RegExp('ENOTDIR'))
    })
  })

  describe('native-image', function () {
    it('reads image from asar archive', function () {
      var p = path.join(fixtures, 'asar', 'logo.asar', 'logo.png')
      var logo = nativeImage.createFromPath(p)
      assert.deepStrictEqual(logo.getSize(), {
        width: 55,
        height: 55
      })
    })

    it('reads image from asar archive with unpacked files', function () {
      var p = path.join(fixtures, 'asar', 'unpack.asar', 'atom.png')
      var logo = nativeImage.createFromPath(p)
      assert.deepStrictEqual(logo.getSize(), {
        width: 1024,
        height: 1024
      })
    })
  })
})
