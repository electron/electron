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

      it 'throws ENOENT error when can not find a file', ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        throws = -> fs.readFileSync p
        assert.throws throws, /ENOENT/

    describe 'fs.readFile', ->
      it 'reads a normal file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal String(content), 'file1\n'
          done()

      it 'reads a linked file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link1'
        fs.readFile p, (err, content) ->
          assert.equal String(content), 'file1\n'
          done()

      it 'reads a file from linked directory', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'link2', 'link2', 'file1'
        fs.readFile p, (err, content) ->
          assert.equal String(content), 'file1\n'
          done()

      it 'throws ENOENT error when can not find a file', (done) ->
        p = path.join fixtures, 'asar', 'a.asar', 'not-exist'
        fs.readFile p, (err, content) ->
          assert.equal err.code, 'ENOENT'
          done()

    describe 'fs.lstatSync', ->
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

      it 'throws ENOENT error when can not find a file', ->
        for file in ['file4', 'file5', path.join('dir1', 'file4')]
          p = path.join fixtures, 'asar', 'a.asar', file
          throws = -> fs.lstatSync p
          assert.throws throws, /ENOENT/
