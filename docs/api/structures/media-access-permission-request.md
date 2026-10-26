# MediaAccessPermissionRequest Object extends `PermissionRequest`

* `securityOrigin` string (optional) - The security origin of the request.
* `mediaTypes` string[] (optional) - The types of media access being requested - elements can be `video`
  or `audio`. For a `media` permission request these are the camera and microphone respectively; for a
  `display-capture` permission request they are the screen, window or tab video and its audio.
