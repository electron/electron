describe 'webrtc', ->
  describe 'navigator.webkitGetUserMedia', ->
    it 'should call its callbacks', (done) ->
      @timeout 5000
      navigator.webkitGetUserMedia audio: true, video: true,
        -> done()
        -> done()
