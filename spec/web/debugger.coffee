describe 'debugger', ->
  describe 'heap snapshot', ->
    it 'should not crash', ->
      process.atomBinding('v8_util').takeHeapSnapshot()
