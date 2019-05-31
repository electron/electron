This page tracks the issues, contributions, and status of running the Electron Test Suite.

# ARM device

## Rock960c

Issue                                                           | Test | Status
----------------------------------------------------------------|------|-------
[Tests failing on Rock960c](https://github.com/electron/electron/issues/18550) | | Open
| Run all tests | 5 fail 163 pass
| | BrowserWindow module window.close() should emit beforeunload handler | fail
| | BrowserWindow module "webPreferences" option nativeWindowOpen option loads native addons correctly after reload | fail
| | BrowserWindow module beforeunload handler returning empty string would prevent close | fail
| | BrowserWindow module "after each" hook: closeTheWindow | fail
| | BrowserWindow module "before each" hook: openTheWindow | fail
 
