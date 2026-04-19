# Internal System Audio Capture API

This document describes the internal-only system audio capture module added in Electron's main process bindings.

## Scope

This API is currently internal and is not part of Electron's public `require('electron')` surface.

It is exposed through a linked binding:

- `process._linkedBinding('electron_browser_system_audio_capture')`

## Entry Points

Main files:

- `shell/browser/media/system_audio_capturer.h`
- `shell/browser/media/system_audio_capturer.cc`
- `shell/browser/media/system_audio_capturer_mac.mm`
- `shell/browser/api/electron_api_system_audio_capture.h`
- `shell/browser/api/electron_api_system_audio_capture.cc`
- `lib/browser/api/system-audio-capture.ts`

Registration and typing updates:

- `shell/common/node_bindings.cc`
- `typings/internal-ambient.d.ts`
- `typings/internal-electron.d.ts`

## Runtime Behavior

- Capture starts from `SystemAudioCapture.startHandling()` and stops with `stopHandling()`.
- The macOS implementation uses ScreenCaptureKit stream audio output callbacks.
- PCM frames are forwarded to JS via `_onframe(pcm, meta)`.
- Errors are forwarded via `_onerror(errorMessage)`.
- Diagnostics are available through `getCaptureDiagnostics()`.

## macOS Requirements

- System audio capture requires macOS 13.0 or newer.
- Screen capture permission is required before starting stream capture.
- If permission is missing, startup returns `false` and diagnostics `lastError` is populated.

## Diagnostics Contract

`getCaptureDiagnostics()` returns:

- `permissionPreflight`
- `usingSystemCapture`
- `lastStartSystemCaptureSucceeded`
- `systemFrameCallbacks`
- `fallbackFrameCallbacks`
- `queuedFrameDepth`
- `queueMaxDepth`
- `queueDroppedFrames`
- `queueUnderrunFrames`
- `lastError`

## Test Coverage

Primary spec:

- `spec/api-system-audio-capture-spec.ts`

Useful related checks:

- `spec/api-desktop-capturer-spec.ts`
- `spec/api-screen-spec.ts`

Example local validation:

```bash
node ./script/spec-runner.js --files spec/api-system-audio-capture-spec.ts
```
