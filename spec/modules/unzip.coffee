assert = require 'assert'
fs = require 'fs'
path = require 'path'
unzip = require 'unzip'

fixtures = path.resolve __dirname, '..', 'fixtures'

describe 'unzip module', ->
  it 'fires close event', (done) ->
    fs.createReadStream(path.join(fixtures, 'zip', 'a.zip'))
      .pipe(unzip.Parse())
      .on('close', done)
