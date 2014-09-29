assert = require 'assert'
fs     = require 'fs'
path   = require 'path'

describe 'asar package', ->
  fixtures = path.join __dirname, 'fixtures'

  describe 'node api', ->
    describe 'fs.readFileSync', ->
      it 'reads a normal file', ->
        file1 = path.join fixtures, 'asar', 'a.asar', 'file1'
        assert.equal fs.readFileSync(file1).toString(), 'file1\n'
        file2 = path.join fixtures, 'asar', 'a.asar', 'file2'
        assert.equal fs.readFileSync(file2).toString(), 'file2\n'
        file3 = path.join fixtures, 'asar', 'a.asar', 'file3'
        assert.equal fs.readFileSync(file3).toString(), 'file3\n'

      it 'reads a linked file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'link1'
        assert.equal fs.readFileSync(p).toString(), 'file1\n'

      it 'reads a file from linked directory', ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'file1'
        assert.equal fs.readFileSync(p).toString(), 'file1\n'
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        assert.equal fs.readFileSync(p).toString(), 'file1\n'

      it 'throws ENOENT error when can not find file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        throws = -> fs.readFileSync p
        assert.throws throws, /ENOENT/

    describe 'fs.readFile', ->
      it 'reads a normal file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content), 'file1\n'
          done()

      it 'reads a linked file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content), 'file1\n'
          done()

      it 'reads a file from linked directory', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal err, null
          assert.equal String(content), 'file1\n'
          done()

      it 'throws ENOENT error when can not find file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        fs.readFile p, (err, content) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'fs.lstatSync', ->
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

    describe 'fs.readdirSync', ->
      it 'reads dirs from root', ->
        p = path.join fixtures, 'asar', 'a.asar'
        dirs = fs.readdirSync p
        assert.deepEqual dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2']

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
          assert.deepEqual dirs, ['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2']
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
        stats = fs.readdir p, (err, stats) ->
          assert.equal err.code, 'ENOENT'
          done()
