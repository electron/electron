# PermissionRequest Object

* `requestingOrigin` string - The origin you should treat the request as coming from. Note this may be different to `requestingUrl` in the case of child frames, you should use `requestingOrigin` as the "safe" one to check.
* `requestingUrl` string - The last URL the requesting frame loaded.
* `isMainFrame` boolean - Whether the frame making the request is the main frame.
