assert        = require 'assert'
child_process = require 'child_process'
fs            = require 'fs'
path          = require 'path'

describe 'asar package', ->
  fixtures = path.join __dirname, 'fixtures'

  describe 'node api', ->
    describe 'fs.readFileSync', ->
      it 'does not leak fd', ->
        for i in [1..10000]
          fs.readFileSync(path.join(process.resourcesPath, 'atom.asar', 'renderer', 'api', 'lib', 'ipc.js'))

      it 'reads a normal file', ->
        file1 = path.join fixtures, 'asar', 'a.asar', 'file1'
        assert.equal fs.readFileSync(file1).toString().trim(), 'file1'
        file2 = path.join fixtures, 'asar', 'a.asar', 'file2'
        assert.equal fs.readFileSync(file2).toString().trim(), 'file2'
        file3 = path.join fixtures, 'asar', 'a.asar', 'file3'
        assert.equal fs.readFileSync(file3).toString().trim(), 'file3'

      it 'reads from a empty file', ->
        file = path.join fixtures, 'asar', 'empty.asar', 'file1'
        buffer = fs.readFileSync(file)
        assert.equal buffer.length, 0
        assert.equal buffer.toString(), ''

      it 'reads a linked file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'link1'
        assert.equal fs.readFileSync(p).toString().trim(), 'file1'

      it 'reads a file from linked directory', ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'file1'
        assert.equal fs.readFileSync(p).toString().trim(), 'file1'
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        assert.equal fs.readFileSync(p).toString().trim(), 'file1'

      it 'throws ENOENT error when can not find file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        throws = -> fs.readFileSync p
        assert.throws throws, /ENOENT/

      it 'passes ENOENT error to callback when can not find file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        async = false
        fs.readFile p, (e) ->
          assert async
          assert /ENOENT/.test e
        async = true

      it 'reads a normal file with unpacked files', ->
        p = path.join fixtures, 'asar', 'unpack.asar', 'a.txt'
        assert.equal fs.readFileSync(p).toString().trim(), 'a'

    describe 'fs.readFile', ->
      it 'reads a normal file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content).trim(), 'file1'
          done()

      it 'reads from a empty file', (done) ->
        p = path.join fixtures, 'asar', 'empty.asar', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content), ''
          done()

      it 'reads a linked file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content).trim(), 'file1'
          done()

      it 'reads a file from linked directory', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content).trim(), 'file1'
          done()

      it 'throws ENOENT error when can not find file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        fs.readFile p, (err, content) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'fs.lstatSync', ->
      it 'handles path with trailing slash correctly', ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        fs.lstatSync p
        fs.lstatSync p + '/'

      it 'returns information of root', ->
        p = path.join fixtures, 'asar', 'a.asar'
        stats = fs.lstatSync p
        assert.equal stats.isFile(), false
        assert.equal stats.isDirectory(), true
        assert.equal stats.isSymbolicLink(), false
        assert.equal stats.size, 0

      it 'returns information of a normal file', ->
        for file in ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')]
          p = path.join fixtures, 'asar', 'a.asar', file
          stats = fs.lstatSync p
          assert.equal stats.isFile(), true
          assert.equal stats.isDirectory(), false
          assert.equal stats.isSymbolicLink(), false
          assert.equal stats.size, 6

      it 'returns information of a normal directory', ->
        for file in ['dir1', 'dir2', 'dir3']
          p = path.join fixtures, 'asar', 'a.asar', file
          stats = fs.lstatSync p
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), true
          assert.equal stats.isSymbolicLink(), false
          assert.equal stats.size, 0

      it 'returns information of a linked file', ->
        for file in ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')]
          p = path.join fixtures, 'asar', 'a.asar', file
          stats = fs.lstatSync p
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), false
          assert.equal stats.isSymbolicLink(), true
          assert.equal stats.size, 0

      it 'returns information of a linked directory', ->
        for file in ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')]
          p = path.join fixtures, 'asar', 'a.asar', file
          stats = fs.lstatSync p
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), false
          assert.equal stats.isSymbolicLink(), true
          assert.equal stats.size, 0

      it 'throws ENOENT error when can not find file', ->
        for file in ['file4', 'file5', path.join('dir1', 'file4')]
          p = path.join fixtures, 'asar', 'a.asar', file
          throws = -> fs.lstatSync p
          assert.throws throws, /ENOENT/

    describe 'fs.lstat', ->
      it 'handles path with trailing slash correctly', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        fs.lstat p + '/', done

      it 'returns information of root', (done) ->
        p = path.join fixtures, 'asar', 'a.asar'
        stats = fs.lstat p, (err, stats) ->
          assert.equal err, null
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), true
          assert.equal stats.isSymbolicLink(), false
          assert.equal stats.size, 0
          done()

      it 'returns information of a normal file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'file1'
        stats = fs.lstat p, (err, stats) ->
          assert.equal err, null
          assert.equal stats.isFile(), true
          assert.equal stats.isDirectory(), false
          assert.equal stats.isSymbolicLink(), false
          assert.equal stats.size, 6
          done()

      it 'returns information of a normal directory', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'dir1'
        stats = fs.lstat p, (err, stats) ->
          assert.equal err, null
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), true
          assert.equal stats.isSymbolicLink(), false
          assert.equal stats.size, 0
          done()

      it 'returns information of a linked file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link1'
        stats = fs.lstat p, (err, stats) ->
          assert.equal err, null
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), false
          assert.equal stats.isSymbolicLink(), true
          assert.equal stats.size, 0
          done()

      it 'returns information of a linked directory', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2'
        stats = fs.lstat p, (err, stats) ->
          assert.equal err, null
          assert.equal stats.isFile(), false
          assert.equal stats.isDirectory(), false
          assert.equal stats.isSymbolicLink(), true
          assert.equal stats.size, 0
          done()

      it 'throws ENOENT error when can not find file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'file4'
        stats = fs.lstat p, (err, stats) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'fs.realpathSync', ->
      it 'returns real path root', ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = 'a.asar'
        r = fs.realpathSync path.join(parent, p)
        assert.equal r, path.join(parent, p)

      it 'returns real path of a normal file', ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'file1'
        r = fs.realpathSync path.join(parent, p)
        assert.equal r, path.join(parent, p)

      it 'returns real path of a normal directory', ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'dir1'
        r = fs.realpathSync path.join(parent, p)
        assert.equal r, path.join(parent, p)

      it 'returns real path of a linked file', ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'link2', 'link1'
        r = fs.realpathSync path.join(parent, p)
        assert.equal r, path.join(parent, 'a.asar', 'file1')

      it 'returns real path of a linked directory', ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'link2', 'link2'
        r = fs.realpathSync path.join(parent, p)
        assert.equal r, path.join(parent, 'a.asar', 'dir1')

      it 'throws ENOENT error when can not find file', ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'not-exist'
        throws = -> fs.realpathSync path.join(parent, p)
        assert.throws throws, /ENOENT/

    describe 'fs.realpath', ->
      it 'returns real path root', (done) ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = 'a.asar'
        fs.realpath path.join(parent, p), (err, r) ->
          assert.equal err, null
          assert.equal r, path.join(parent, p)
          done()

      it 'returns real path of a normal file', (done) ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'file1'
        fs.realpath path.join(parent, p), (err, r) ->
          assert.equal err, null
          assert.equal r, path.join(parent, p)
          done()

      it 'returns real path of a normal directory', (done) ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'dir1'
        fs.realpath path.join(parent, p), (err, r) ->
          assert.equal err, null
          assert.equal r, path.join(parent, p)
          done()

      it 'returns real path of a linked file', (done) ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'link2', 'link1'
        fs.realpath path.join(parent, p), (err, r) ->
          assert.equal err, null
          assert.equal r, path.join(parent, 'a.asar', 'file1')
          done()

      it 'returns real path of a linked directory', (done) ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'link2', 'link2'
        fs.realpath path.join(parent, p), (err, r) ->
          assert.equal err, null
          assert.equal r, path.join(parent, 'a.asar', 'dir1')
          done()

      it 'throws ENOENT error when can not find file', (done) ->
        parent = fs.realpathSync path.join(fixtures, 'asar')
        p = path.join 'a.asar', 'not-exist'
        fs.realpath path.join(parent, p), (err, stats) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'fs.readdirSync', ->
      it 'reads dirs from root', ->
        p = path.join fixtures, 'asar', 'a.asar'
        dirs = fs.readdirSync p
        assert.deepEqual dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']

      it 'reads dirs from a normal dir', ->
        p = path.join fixtures, 'asar', 'a.asar', 'dir1'
        dirs = fs.readdirSync p
        assert.deepEqual dirs, ['file1', 'file2', 'file3', 'link1', 'link2']

      it 'reads dirs from a linked dir', ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2'
        dirs = fs.readdirSync p
        assert.deepEqual dirs, ['file1', 'file2', 'file3', 'link1', 'link2']

      it 'throws ENOENT error when can not find file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        throws = -> fs.readdirSync p
        assert.throws throws, /ENOENT/

    describe 'fs.readdir', ->
      it 'reads dirs from root', (done) ->
        p = path.join fixtures, 'asar', 'a.asar'
        dirs = fs.readdir p, (err, dirs) ->
          assert.equal err, null
          assert.deepEqual dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']
          done()

      it 'reads dirs from a normal dir', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'dir1'
        dirs = fs.readdir p, (err, dirs) ->
          assert.equal err, null
          assert.deepEqual dirs, ['file1', 'file2', 'file3', 'link1', 'link2']
          done()

      it 'reads dirs from a linked dir', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2'
        dirs = fs.readdir p, (err, dirs) ->
          assert.equal err, null
          assert.deepEqual dirs, ['file1', 'file2', 'file3', 'link1', 'link2']
          done()

      it 'throws ENOENT error when can not find file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        fs.readdir p, (err, stats) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'fs.openSync', ->
      it 'opens a normal/linked/under-linked-directory file', ->
        for file in ['file1', 'link1', path.join('link2', 'file1')]
          p = path.join fixtures, 'asar', 'a.asar', file
          fd = fs.openSync p, 'r'
          buffer = new Buffer(6)
          fs.readSync fd, buffer, 0, 6, 0
          assert.equal String(buffer).trim(), 'file1'
          fs.closeSync fd

      it 'throws ENOENT error when can not find file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        throws = -> fs.openSync p
        assert.throws throws, /ENOENT/

    describe 'fs.open', ->
      it 'opens a normal file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'file1'
        fs.open p, 'r', (err, fd) ->
          assert.equal err, null
          buffer = new Buffer(6)
          fs.read fd, buffer, 0, 6, 0, (err) ->
            assert.equal err, null
            assert.equal String(buffer).trim(), 'file1'
            fs.close fd, done

      it 'throws ENOENT error when can not find file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        fs.open p, 'r', (err, stats) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'child_process.fork', ->
      child_process = require 'child_process'

      it 'opens a normal js file', (done) ->
        child = child_process.fork path.join(fixtures, 'asar', 'a.asar', 'ping.js')
        child.on 'message', (msg) ->
          assert.equal msg, 'message'
          done()
        child.send 'message'

      it 'supports asar in the forked js', (done) ->
        file = path.join fixtures, 'asar', 'a.asar', 'file1'
        child = child_process.fork path.join(fixtures, 'module', 'asar.js')
        child.on 'message', (content) ->
          assert.equal content, fs.readFileSync(file).toString()
          done()
        child.send file

    describe 'internalModuleReadFile', ->
      internalModuleReadFile = process.binding('fs').internalModuleReadFile

      it 'read a normal file', ->
        file1 = path.join fixtures, 'asar', 'a.asar', 'file1'
        assert.equal internalModuleReadFile(file1).toString().trim(), 'file1'
        file2 = path.join fixtures, 'asar', 'a.asar', 'file2'
        assert.equal internalModuleReadFile(file2).toString().trim(), 'file2'
        file3 = path.join fixtures, 'asar', 'a.asar', 'file3'
        assert.equal internalModuleReadFile(file3).toString().trim(), 'file3'

      it 'reads a normal file with unpacked files', ->
        p = path.join fixtures, 'asar', 'unpack.asar', 'a.txt'
        assert.equal internalModuleReadFile(p).toString().trim(), 'a'

  describe 'asar protocol', ->
    url = require 'url'
    remote = require 'remote'
    ipc = remote.require 'ipc'
    BrowserWindow = remote.require 'browser-window'

    it 'can request a file in package', (done) ->
      p = path.resolve fixtures, 'asar', 'a.asar', 'file1'
      $.get "file://#{p}", (data) ->
        assert.equal data.trim(), 'file1'
        done()

    it 'can request a file in package with unpacked files', (done) ->
      p = path.resolve fixtures, 'asar', 'unpack.asar', 'a.txt'
      $.get "file://#{p}", (data) ->
        assert.equal data.trim(), 'a'
        done()

    it 'can request a linked file in package', (done) ->
      p = path.resolve fixtures, 'asar', 'a.asar', 'link2', 'link1'
      $.get "file://#{p}", (data) ->
        assert.equal data.trim(), 'file1'
        done()

    it 'can request a file in filesystem', (done) ->
      p = path.resolve fixtures, 'asar', 'file'
      $.get "file://#{p}", (data) ->
        assert.equal data.trim(), 'file'
        done()

    it 'gets 404 when file is not found', (done) ->
      p = path.resolve fixtures, 'asar', 'a.asar', 'no-exist'
      $.ajax
        url: "file://#{p}"
        error: (err) ->
          assert.equal err.status, 404
          done()

    it 'sets __dirname correctly', (done) ->
      after ->
        w.destroy()
        ipc.removeAllListeners 'dirname'

      w = new BrowserWindow(show: false, width: 400, height: 400)
      p = path.resolve fixtures, 'asar', 'web.asar', 'index.html'
      u = url.format protocol: 'file', slashed: true, pathname: p
      ipc.once 'dirname', (event, dirname) ->
        assert.equal dirname, path.dirname(p)
        done()
      w.loadUrl u

    it 'loads script tag in html', (done) ->
      after ->
        w.destroy()
        ipc.removeAllListeners 'ping'

      w = new BrowserWindow(show: false, width: 400, height: 400)
      p = path.resolve fixtures, 'asar', 'script.asar', 'index.html'
      u = url.format protocol: 'file', slashed: true, pathname: p
      w.loadUrl u
      ipc.once 'ping', (event, message) ->
        assert.equal message, 'pong'
        done()

  describe 'original-fs module', ->
    originalFs = require 'original-fs'

    it 'treats .asar as file', ->
      file = path.join fixtures, 'asar', 'a.asar'
      stats = originalFs.statSync file
      assert stats.isFile()

    it 'is available in forked scripts', (done) ->
      child = child_process.fork path.join(fixtures, 'module', 'original-fs.js')
      child.on 'message', (msg) ->
        assert.equal msg, 'object'
        done()
      child.send 'message'

  describe 'graceful-fs module', ->
    gfs = require 'graceful-fs'

    it 'recognize asar archvies', ->
      p = path.join fixtures, 'asar', 'a.asar', 'link1'
      assert.equal gfs.readFileSync(p).toString().trim(), 'file1'

    it 'does not touch global fs object', ->
      assert.notEqual fs.readdir, gfs.readdir

  describe 'native-image', ->
    it 'reads image from asar archive', ->
      p = path.join fixtures, 'asar', 'logo.asar', 'logo.png'
      logo = require('native-image').createFromPath p
      assert.deepEqual logo.getSize(), {width: 55, height: 55}

    it 'reads image from asar archive with unpacked files', ->
      p = path.join fixtures, 'asar', 'unpack.asar', 'atom.png'
      logo = require('native-image').createFromPath p
      assert.deepEqual logo.getSize(), {width: 1024, height: 1024}
