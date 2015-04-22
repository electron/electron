assert = require 'assert'
app = require('remote').require 'app'

describe 'app module', ->
  describe 'app.getVersion()', ->
    it 'returns the version field of package.json', ->
      assert.equal app.getVersion(), '0.1.0'

  describe 'app.setVersion(version)', ->
    it 'overrides the version', ->
      assert.equal app.getVersion(), '0.1.0'
      app.setVersion 'test-version'
      assert.equal app.getVersion(), 'test-version'
      app.setVersion '0.1.0'

  describe 'app.getName()', ->
    it 'returns the name field of package.json', ->
      assert.equal app.getName(), 'Electron Test'

  describe 'app.setName(name)', ->
    it 'overrides the name', ->
      assert.equal app.getName(), 'Electron Test'
      app.setName 'test-name'
      assert.equal app.getName(), 'test-name'
      app.setName 'Electron Test'
