describe 'http', ->
  describe 'sending request of http protocol urls', ->
    it 'should not crash', ->
      $.get 'https://api.github.com/zen'
