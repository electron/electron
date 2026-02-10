Fix use-after-free in WebContents async permission callbacks.

Electron no longer crashes when a BrowserWindow is closed while a fullscreen,
pointer lock, or keyboard lock permission request is still pending. The fix
ensures callbacks safely no-op if the associated WebContents has already been
destroyed.
