# LOAF Script Attribution Fix Report

## 1. Main Issue Solved
The primary issue was that the Long Animation Frame (LoAF) API in Chromium did not expose script attribution details for non-HTTPS sources. Specifically, trusted local development environments—such as `file://` URLs (used by Electron apps loading local resources) and `http://localhost`—were failing the standard same-origin checks. This resulted in "opaque" attribution, making it impossible for developers to debug performance issues or identify the source of long animation frames in these common Electron scenarios.

## 2. Files Involved
The issue was located in the Chromium Blink renderer source code:
*   **File Path**: `third_party/blink/renderer/core/timing/performance_long_animation_frame_timing.cc`
*   **Component**: Blink Renderer Core (Timing)

## 3. Logic Applied
To resolve this, we needed to extend the security logic to trust specific local contexts that are safe in an Electron environment but restricted by default in broad web contexts.

The new logic explicitly approves script attribution if:
*   **Default**: The standard same-origin check passes (`source_origin->CanAccess(script_origin)`).
*   **New**: The script comes from a `file://` URL. Since Electron apps run from local files, these are considered trusted parts of the application.
*   **New**: The script comes from a "potentially trustworthy" origin, such as `localhost` or `127.0.0.1`. This supports local server development.

## 4. Changes Made
We modified `performance_long_animation_frame_timing.cc` to implement the logic above.

### Code Added
A helper function was introduced to encapsulate the permissive logic:

```cpp
namespace {

bool ShouldIncludeScriptAttribution(const SecurityOrigin* source_origin,
                                    const SecurityOrigin* script_origin) {
  // 1. Standard same-origin check
  if (source_origin->CanAccess(script_origin)) {
    return true;
  }

  // 2. Allow file:// URLs (Electron-specific trusted local resources)
  if (script_origin->Protocol() == "file") {
    return true;
  }

  // 3. Allow localhost/trustworthy origins (Local development)
  if (script_origin->IsPotentiallyTrustworthy()) {
    return true;
  }

  return false;
}

}  // namespace
```

### Code Modified
The loop iterating through scripts was updated to use this new check instead of the strict `CanAccess` method:

```cpp
// Before
if (security_origin->CanAccess(script->GetSecurityOrigin())) { ... }

// After
if (ShouldIncludeScriptAttribution(security_origin, script->GetSecurityOrigin())) { ... }
```

## 5. How We Solved It
The solution was implemented via the Electron patching system:
1.  **Analysis**: We identified that the strict `CanAccess` check in Blink was the blocker.
2.  **Patching**: Instead of forking Chromium entirely, we created a precise git patch file.
3.  **Application**: This patch file (`feat_expose_loaf_script_attribution_for_non_https.patch`) was placed in `patches/chromium/`. Theoretically, the Electron build system applies this patch to the Chromium source code before compiling, injecting our custom logic into the final binary.
