assert = require 'assert'
fs = require 'fs'

describe 'contexts', ->
  describe 'setTimeout in fs callback', ->
    it 'does not crash', (done) ->
      fs.readFile __filename, ->
        setTimeout done, 0

  describe 'throw error in node context', ->
    it 'get caught', (done) ->
      error = new Error('boo!')
      lsts = process.listeners 'uncaughtException'
      process.removeAllListeners 'uncaughtException'
      process.on 'uncaughtException', (err) ->
        process.removeAllListeners 'uncaughtException'
        for lst in lsts
          process.on 'uncaughtException', lst
        done()
      fs.readFile __filename, ->
        throw error
