# Release Note for Issue #47648

Notes: Fixed memory leak when repeatedly creating and removing same-origin iframes with nodeIntegration enabled.

---

## Full Description

Fixed a critical memory leak that occurred when repeatedly creating and removing same-origin iframes with `nodeIntegration` or `nodeIntegrationInSubFrames` enabled. Previously, Node.js environments were not properly cleaned up when iframes were removed, causing memory usage to grow with each iframe recreation.

**Technical Details:**
- Same-origin iframes share V8 contexts (Chromium behavior)
- Node.js assumes 1:1 relationship between V8 contexts and Node environments
- Added `context_to_environment_` map to track proper ownership
- Modified `DidCreateScriptContext()` to prevent duplicate environment creation
- Modified `WillReleaseScriptContext()` to use mapping for correct cleanup

**Impact:**
- Fixes memory leak in applications that dynamically create/remove iframes with Node integration
- Significantly reduces memory footprint for iframe-heavy architectures
- No API changes or breaking changes

**Issue:** #47648
**Platforms:** Windows, macOS, Linux
