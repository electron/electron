This page tracks the issues, contributions, and status of running the Electron Test Suite. For more information, see GitHub issue [electron#18550](https://github.com/electron/electron/issues/18550).

# ARM device

## Rock960c

# Run 05/30/2019 

- Electron Version: master build from 05/21/2019

Issue                                                           | Test | Status
----------------------------------------------------------------|------|-------
[Tests failing on Rock960c](https://github.com/electron/electron/issues/18550) | | Open
| Run all tests | 5 fail 163 pass
| | BrowserWindow module window.close() should emit beforeunload handler | fail
| | BrowserWindow module "webPreferences" option nativeWindowOpen option loads native addons correctly after reload | fail
| | BrowserWindow module beforeunload handler returning empty string would prevent close | fail
| | BrowserWindow module "after each" hook: closeTheWindow | fail
| | BrowserWindow module "before each" hook: openTheWindow | fail
 

# Run 06/06/2019

- Electron Verion: v6.0.0-beta.5

Issue                                                           | Test | Status
----------------------------------------------------------------|------|-------
Run all tests | tests 687 pass 665 fail 22 | | 
