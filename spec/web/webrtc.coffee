describe 'webrtc', ->
  describe 'navigator.webkitGetUserMedia', ->
    it 'should call its callbacks', (done) ->
      navigator.webkitGetUserMedia audio: true, video: true,
        -> done()
        -> done()
