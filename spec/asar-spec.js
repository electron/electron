const assert = require('assert')
const ChildProcess = require('child_process')
const fs = require('fs')
const path = require('path')
const temp = require('temp').track()
const util = require('util')
const {closeWindow} = require('./window-helpers')

const nativeImage = require('electron').nativeImage
const remote = require('electron').remote

const ipcMain = remote.require('electron').ipcMain
const BrowserWindow = remote.require('electron').BrowserWindow

describe('asar package', function () {
  var fixtures = path.join(__dirname, 'fixtures')

  describe('node api', function () {
    it('supports paths specified as a Buffer', function () {
      var file = Buffer.from(path.join(fixtures, 'asar', 'a.asar', 'file1'))
      assert.equal(fs.existsSync(file), true)
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
        assert.equal(fs.readFileSync(file1).toString().trim(), 'file1')
        var file2 = path.join(fixtures, 'asar', 'a.asar', 'file2')
        assert.equal(fs.readFileSync(file2).toString().trim(), 'file2')
        var file3 = path.join(fixtures, 'asar', 'a.asar', 'file3')
        assert.equal(fs.readFileSync(file3).toString().trim(), 'file3')
      })

      it('reads from a empty file', function () {
        var file = path.join(fixtures, 'asar', 'empty.asar', 'file1')
        var buffer = fs.readFileSync(file)
        assert.equal(buffer.length, 0)
        assert.equal(buffer.toString(), '')
      })

      it('reads a linked file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link1')
        assert.equal(fs.readFileSync(p).toString().trim(), 'file1')
      })

      it('reads a file from linked directory', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'file1')
        assert.equal(fs.readFileSync(p).toString().trim(), 'file1')
        p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1')
        assert.equal(fs.readFileSync(p).toString().trim(), 'file1')
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
        assert.equal(fs.readFileSync(p).toString().trim(), 'a')
      })
    })

    describe('fs.readFile', function () {
      it('reads a normal file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        fs.readFile(p, function (err, content) {
          assert.equal(err, null)
          assert.equal(String(content).trim(), 'file1')
          done()
        })
      })

      it('reads from a empty file', function (done) {
        var p = path.join(fixtures, 'asar', 'empty.asar', 'file1')
        fs.readFile(p, function (err, content) {
          assert.equal(err, null)
          assert.equal(String(content), '')
          done()
        })
      })

      it('reads from a empty file with encoding', function (done) {
        var p = path.join(fixtures, 'asar', 'empty.asar', 'file1')
        fs.readFile(p, 'utf8', function (err, content) {
          assert.equal(err, null)
          assert.equal(content, '')
          done()
        })
      })

      it('reads a linked file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link1')
        fs.readFile(p, function (err, content) {
          assert.equal(err, null)
          assert.equal(String(content).trim(), 'file1')
          done()
        })
      })

      it('reads a file from linked directory', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1')
        fs.readFile(p, function (err, content) {
          assert.equal(err, null)
          assert.equal(String(content).trim(), 'file1')
          done()
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.readFile(p, function (err) {
          assert.equal(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.copyFile', function () {
      it('copies a normal file', function (done) {
        const p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        const dest = temp.path()
        fs.copyFile(p, dest, function (err) {
          assert.strictEqual(err, null)
          assert(fs.readFileSync(p).equals(fs.readFileSync(dest)))
          done()
        })
      })

      it('copies a unpacked file', function (done) {
        const p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        const dest = temp.path()
        fs.copyFile(p, dest, function (err) {
          assert.strictEqual(err, null)
          assert(fs.readFileSync(p).equals(fs.readFileSync(dest)))
          done()
        })
      })
    })

    describe('fs.copyFileSync', function () {
      it('copies a normal file', function () {
        const p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        const dest = temp.path()
        fs.copyFileSync(p, dest)
        assert(fs.readFileSync(p).equals(fs.readFileSync(dest)))
      })

      it('copies a unpacked file', function () {
        const p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        const dest = temp.path()
        fs.copyFileSync(p, dest)
        assert(fs.readFileSync(p).equals(fs.readFileSync(dest)))
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
        assert.equal(stats.isFile(), false)
        assert.equal(stats.isDirectory(), true)
        assert.equal(stats.isSymbolicLink(), false)
        assert.equal(stats.size, 0)
      })

      it('returns information of a normal file', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.equal(stats.isFile(), true)
          assert.equal(stats.isDirectory(), false)
          assert.equal(stats.isSymbolicLink(), false)
          assert.equal(stats.size, 6)
        }
      })

      it('returns information of a normal directory', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['dir1', 'dir2', 'dir3']
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), true)
          assert.equal(stats.isSymbolicLink(), false)
          assert.equal(stats.size, 0)
        }
      })

      it('returns information of a linked file', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), false)
          assert.equal(stats.isSymbolicLink(), true)
          assert.equal(stats.size, 0)
        }
      })

      it('returns information of a linked directory', function () {
        var file, j, len, p, ref2, stats
        ref2 = ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')]
        for (j = 0, len = ref2.length; j < len; j++) {
          file = ref2[j]
          p = path.join(fixtures, 'asar', 'a.asar', file)
          stats = fs.lstatSync(p)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), false)
          assert.equal(stats.isSymbolicLink(), true)
          assert.equal(stats.size, 0)
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
          assert.equal(err, null)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), true)
          assert.equal(stats.isSymbolicLink(), false)
          assert.equal(stats.size, 0)
          done()
        })
      })

      it('returns information of a normal file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'file1')
        fs.lstat(p, function (err, stats) {
          assert.equal(err, null)
          assert.equal(stats.isFile(), true)
          assert.equal(stats.isDirectory(), false)
          assert.equal(stats.isSymbolicLink(), false)
          assert.equal(stats.size, 6)
          done()
        })
      })

      it('returns information of a normal directory', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        fs.lstat(p, function (err, stats) {
          assert.equal(err, null)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), true)
          assert.equal(stats.isSymbolicLink(), false)
          assert.equal(stats.size, 0)
          done()
        })
      })

      it('returns information of a linked file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link1')
        fs.lstat(p, function (err, stats) {
          assert.equal(err, null)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), false)
          assert.equal(stats.isSymbolicLink(), true)
          assert.equal(stats.size, 0)
          done()
        })
      })

      it('returns information of a linked directory', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2')
        fs.lstat(p, function (err, stats) {
          assert.equal(err, null)
          assert.equal(stats.isFile(), false)
          assert.equal(stats.isDirectory(), false)
          assert.equal(stats.isSymbolicLink(), true)
          assert.equal(stats.size, 0)
          done()
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file4')
        fs.lstat(p, function (err) {
          assert.equal(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.realpathSync', () => {
      it('returns real path root', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = 'a.asar'
        const r = fs.realpathSync(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
      })

      it('returns real path of a normal file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        const r = fs.realpathSync(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
      })

      it('returns real path of a normal directory', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        const r = fs.realpathSync(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
      })

      it('returns real path of a linked file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        const r = fs.realpathSync(path.join(parent, p))
        assert.equal(r, path.join(parent, 'a.asar', 'file1'))
      })

      it('returns real path of a linked directory', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        const r = fs.realpathSync(path.join(parent, p))
        assert.equal(r, path.join(parent, 'a.asar', 'dir1'))
      })

      it('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        const r = fs.realpathSync(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
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
        assert.equal(r, path.join(parent, p))
      })

      it('returns real path of a normal file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
      })

      it('returns real path of a normal directory', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
      })

      it('returns real path of a linked file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.equal(r, path.join(parent, 'a.asar', 'file1'))
      })

      it('returns real path of a linked directory', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.equal(r, path.join(parent, 'a.asar', 'dir1'))
      })

      it('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        const r = fs.realpathSync.native(path.join(parent, p))
        assert.equal(r, path.join(parent, p))
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
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal directory', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a linked file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, 'a.asar', 'file1'))
          done()
        })
      })

      it('returns real path of a linked directory', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, 'a.asar', 'dir1'))
          done()
        })
      })

      it('returns real path of an unpacked file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        fs.realpath(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('throws ENOENT error when can not find file', done => {
        const parent = fs.realpathSync(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'not-exist')
        fs.realpath(path.join(parent, p), err => {
          assert.equal(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.realpath.native', () => {
      it('returns real path root', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = 'a.asar'
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'file1')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a normal directory', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'dir1')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('returns real path of a linked file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link1')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, 'a.asar', 'file1'))
          done()
        })
      })

      it('returns real path of a linked directory', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'link2', 'link2')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, 'a.asar', 'dir1'))
          done()
        })
      })

      it('returns real path of an unpacked file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('unpack.asar', 'a.txt')
        fs.realpath.native(path.join(parent, p), (err, r) => {
          assert.equal(err, null)
          assert.equal(r, path.join(parent, p))
          done()
        })
      })

      it('throws ENOENT error when can not find file', done => {
        const parent = fs.realpathSync.native(path.join(fixtures, 'asar'))
        const p = path.join('a.asar', 'not-exist')
        fs.realpath.native(path.join(parent, p), err => {
          assert.equal(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.readdirSync', function () {
      it('reads dirs from root', function () {
        var p = path.join(fixtures, 'asar', 'a.asar')
        var dirs = fs.readdirSync(p)
        assert.deepEqual(dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js'])
      })

      it('reads dirs from a normal dir', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        var dirs = fs.readdirSync(p)
        assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
      })

      it('reads dirs from a linked dir', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2')
        var dirs = fs.readdirSync(p)
        assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
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
          assert.equal(err, null)
          assert.deepEqual(dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js'])
          done()
        })
      })

      it('reads dirs from a normal dir', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'dir1')
        fs.readdir(p, function (err, dirs) {
          assert.equal(err, null)
          assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
          done()
        })
      })
      it('reads dirs from a linked dir', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'link2', 'link2')
        fs.readdir(p, function (err, dirs) {
          assert.equal(err, null)
          assert.deepEqual(dirs, ['file1', 'file2', 'file3', 'link1', 'link2'])
          done()
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.readdir(p, function (err) {
          assert.equal(err.code, 'ENOENT')
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
          assert.equal(String(buffer).trim(), 'file1')
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
          assert.equal(err, null)
          var buffer = Buffer.alloc(6)
          fs.read(fd, buffer, 0, 6, 0, function (err) {
            assert.equal(err, null)
            assert.equal(String(buffer).trim(), 'file1')
            fs.close(fd, done)
          })
        })
      })

      it('throws ENOENT error when can not find file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.open(p, 'r', function (err) {
          assert.equal(err.code, 'ENOENT')
          done()
        })
      })
    })

    describe('fs.mkdir', function () {
      it('throws error when calling inside asar archive', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.mkdir(p, function (err) {
          assert.equal(err.code, 'ENOTDIR')
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
          assert.equal(exists, true)
          done()
        })
      })

      it('handles a non-existent file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        // eslint-disable-next-line
        fs.exists(p, function (exists) {
          assert.equal(exists, false)
          done()
        })
      })

      it('promisified version handles an existing file', (done) => {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        // eslint-disable-next-line
        util.promisify(fs.exists)(p).then(exists => {
          assert.equal(exists, true)
          done()
        })
      })

      it('promisified version handles a non-existent file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        // eslint-disable-next-line
        util.promisify(fs.exists)(p).then(exists => {
          assert.equal(exists, false)
          done()
        })
      })
    })

    describe('fs.existsSync', function () {
      it('handles an existing file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.doesNotThrow(function () {
          assert.equal(fs.existsSync(p), true)
        })
      })

      it('handles a non-existent file', function () {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        assert.doesNotThrow(function () {
          assert.equal(fs.existsSync(p), false)
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
          assert.equal(err.code, 'EACCES')
          done()
        })
      })

      it('throws an error when called on non-existent file', function (done) {
        var p = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
        fs.access(p, function (err) {
          assert.equal(err.code, 'ENOENT')
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
          assert.equal(msg, 'message')
          done()
        })
        child.send('message')
      })

      it('supports asar in the forked js', function (done) {
        var file = path.join(fixtures, 'asar', 'a.asar', 'file1')
        var child = ChildProcess.fork(path.join(fixtures, 'module', 'asar.js'))
        child.on('message', function (content) {
          assert.equal(content, fs.readFileSync(file).toString())
          done()
        })
        child.send(file)
      })
    })

    describe('child_process.exec', function () {
      var echo = path.join(fixtures, 'asar', 'echo.asar', 'echo')

      it('should not try to extract the command if there is a reference to a file inside an .asar', function (done) {
        ChildProcess.exec('echo ' + echo + ' foo bar', function (error, stdout) {
          assert.equal(error, null)
          assert.equal(stdout.toString().replace(/\r/g, ''), echo + ' foo bar\n')
          done()
        })
      })

      it('can be promisified', () => {
        return util.promisify(ChildProcess.exec)('echo ' + echo + ' foo bar').then(({ stdout }) => {
          assert.equal(stdout.toString().replace(/\r/g, ''), echo + ' foo bar\n')
        })
      })
    })

    describe('child_process.execSync', function () {
      var echo = path.join(fixtures, 'asar', 'echo.asar', 'echo')

      it('should not try to extract the command if there is a reference to a file inside an .asar', function (done) {
        var stdout = ChildProcess.execSync('echo ' + echo + ' foo bar')
        assert.equal(stdout.toString().replace(/\r/g, ''), echo + ' foo bar\n')
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
          assert.equal(error, null)
          assert.equal(stdout, 'test\n')
          done()
        })
      })

      xit('execFileSync executes binaries', function () {
        var output = execFileSync(echo, ['test'])
        assert.equal(String(output), 'test\n')
      })

      it('can be promisified', () => {
        return util.promisify(ChildProcess.execFile)(echo, ['test']).then(({ stdout }) => {
          assert.equal(stdout, 'test\n')
        })
      })
    })

    describe('internalModuleReadJSON', function () {
      var internalModuleReadJSON = process.binding('fs').internalModuleReadJSON

      it('read a normal file', function () {
        var file1 = path.join(fixtures, 'asar', 'a.asar', 'file1')
        assert.equal(internalModuleReadJSON(file1).toString().trim(), 'file1')
        var file2 = path.join(fixtures, 'asar', 'a.asar', 'file2')
        assert.equal(internalModuleReadJSON(file2).toString().trim(), 'file2')
        var file3 = path.join(fixtures, 'asar', 'a.asar', 'file3')
        assert.equal(internalModuleReadJSON(file3).toString().trim(), 'file3')
      })

      it('reads a normal file with unpacked files', function () {
        var p = path.join(fixtures, 'asar', 'unpack.asar', 'a.txt')
        assert.equal(internalModuleReadJSON(p).toString().trim(), 'a')
      })
    })

    describe('util.promisify', function () {
      it('can promisify all fs functions', function () {
        const originalFs = require('original-fs')

        for (const key in originalFs) {
          if (originalFs[key][util.promisify.custom] && !fs[key][util.promisify.custom]) {
            assert(false, `fs.${key}[util.promisify.custom] missing`)
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
          assert.equal(error.code, errorName)
          fs.lstat(file, function (error) {
            assert.equal(error.code, errorName)
            fs.realpath(file, function (error) {
              assert.equal(error.code, errorName)
              fs.readdir(dir, function (error) {
                assert.equal(error.code, errorName)
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
        assert.equal(content1.compare(content2), 0)
        assert.throws(function () {
          fs.readdirSync(asar)
        }, /ENOTDIR/)
      })

      it('is reset to its original value when execSync throws an error', function () {
        process.noAsar = false
        assert.throws(function () {
          ChildProcess.execSync(path.join(__dirname, 'does-not-exist.txt'))
        })
        assert.equal(process.noAsar, false)
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
          assert.equal(stats.isFile, true)
          assert.equal(stats.size, 778)
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
          assert.equal(stats.isFile, true)
          assert.equal(stats.size, 778)
          done()
        })
      })
    })
  })

  describe('asar protocol', function () {
    var url = require('url')
    var w = null

    afterEach(function () {
      return closeWindow(w).then(function () { w = null })
    })

    it('can request a file in package', function (done) {
      var p = path.resolve(fixtures, 'asar', 'a.asar', 'file1')
      $.get('file://' + p, function (data) {
        assert.equal(data.trim(), 'file1')
        done()
      })
    })

    it('can request a file in package with unpacked files', function (done) {
      var p = path.resolve(fixtures, 'asar', 'unpack.asar', 'a.txt')
      $.get('file://' + p, function (data) {
        assert.equal(data.trim(), 'a')
        done()
      })
    })

    it('can request a linked file in package', function (done) {
      var p = path.resolve(fixtures, 'asar', 'a.asar', 'link2', 'link1')
      $.get('file://' + p, function (data) {
        assert.equal(data.trim(), 'file1')
        done()
      })
    })

    it('can request a file in filesystem', function (done) {
      var p = path.resolve(fixtures, 'asar', 'file')
      $.get('file://' + p, function (data) {
        assert.equal(data.trim(), 'file')
        done()
      })
    })

    it('gets 404 when file is not found', function (done) {
      var p = path.resolve(fixtures, 'asar', 'a.asar', 'no-exist')
      $.ajax({
        url: 'file://' + p,
        error: function (err) {
          assert.equal(err.status, 404)
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
      var u = url.format({
        protocol: 'file',
        slashed: true,
        pathname: p
      })
      ipcMain.once('dirname', function (event, dirname) {
        assert.equal(dirname, path.dirname(p))
        done()
      })
      w.loadURL(u)
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
      var u = url.format({
        protocol: 'file',
        slashed: true,
        pathname: p
      })
      w.loadURL(u)
      ipcMain.once('ping', function (event, message) {
        assert.equal(message, 'pong')
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
      var u = url.format({
        protocol: 'file',
        slashed: true,
        pathname: p
      })
      w.loadURL(u)
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
        assert.equal(msg, 'object')
        done()
      })
      child.send('message')
    })
  })

  describe('graceful-fs module', function () {
    var gfs = require('graceful-fs')

    it('recognize asar archvies', function () {
      var p = path.join(fixtures, 'asar', 'a.asar', 'link1')
      assert.equal(gfs.readFileSync(p).toString().trim(), 'file1')
    })
    it('does not touch global fs object', function () {
      assert.notEqual(fs.readdir, gfs.readdir)
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
      assert.deepEqual(logo.getSize(), {
        width: 55,
        height: 55
      })
    })

    it('reads image from asar archive with unpacked files', function () {
      var p = path.join(fixtures, 'asar', 'unpack.asar', 'atom.png')
      var logo = nativeImage.createFromPath(p)
      assert.deepEqual(logo.getSize(), {
        width: 1024,
        height: 1024
      })
    })
  })
})
