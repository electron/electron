describe 'http', ->
  describe 'sending request of http protocol urls', ->
    it 'should not crash', (done) ->
      $.ajax
        url: 'http://127.0.0.1'
        success: -> done()
        error: -> done()
