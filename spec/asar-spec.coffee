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
